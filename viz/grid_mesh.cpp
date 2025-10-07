#include "grid_mesh.hpp"

#include <glad/gl.h>

#include <vector>

namespace snow {
namespace viz {
    
GridMesh2D::~GridMesh2D()
{
    destroy(); // Ensure GPU resources are reclaimed when the mesh is destroyed.
}

/**
 * Builds the static grid geometry and allocates a single-channel texture that will later
 * store the air-mask values. The grid is centered on the origin in the YZ plane.
 * @param rows Number of cells along the vertical (Y) axis.
 * @param cols Number of cells along the depth (Z) axis.
 * @param width Total width of the grid in model space.
 */
bool GridMesh2D::initialize(std::size_t rows, std::size_t cols, float width)
{
    destroy(); // Reset any previous state before building a new grid.

    rows_ = rows;   // Cache row count for future updates and sanity checks.
    cols_ = cols;   // Cache column count likewise.

    if (rows_ == 0 || cols_ == 0)
    {
        return false; // Cannot build a grid without positive dimensions.
    }

    const float cell_size = width / static_cast<float>(cols_);        // Width of a single cell in model space. 
                                                                      // TODO:change prevous line to cell width
    const float half_width = width * 0.5f;                            // Half-width used to center the grid on the origin.
    const float total_height = cell_size * static_cast<float>(rows_); // Total height derived from the number of rows.
    const float half_height = total_height * 0.5f;                    // Half-height for centering vertically.

    std::vector<glm::vec3> positions;
    positions.reserve(rows_ * cols_ * 4); // Four vertices per cell.

    std::vector<glm::vec2> texcoords;
    texcoords.reserve(rows_ * cols_ * 4); // Matching texture coordinate per vertex.

    std::vector<unsigned int> indices;
    indices.reserve(rows_ * cols_ * 6);   // Two triangles per cell requires six indices.

    for (std::size_t row = 0; row < rows_; ++row)
    {
        for (std::size_t col = 0; col < cols_; ++col)
        {
            // Horizontal (left/right) bounds across X
            const float x_left  = -half_width + static_cast<float>(col) * cell_size;
            const float x_right = x_left + cell_size;
            // Vertical (top/bottom) bounds across Y
            const float y_top    = half_height - static_cast<float>(row) * cell_size;
            const float y_bottom = y_top - cell_size;

            const std::size_t base = positions.size();
            positions.emplace_back(x_left,  y_top,    0.0f); // top-left in XY
            positions.emplace_back(x_right, y_top,    0.0f); // top-right
            positions.emplace_back(x_right, y_bottom, 0.0f); // bottom-right
            positions.emplace_back(x_left,  y_bottom, 0.0f); // bottom-left

            const float u_left  = static_cast<float>(col)     / static_cast<float>(cols_);
            const float u_right = static_cast<float>(col + 1) / static_cast<float>(cols_);
            const float v_top   = 1.0f - static_cast<float>(row)     / static_cast<float>(rows_);
            const float v_bottom= 1.0f - static_cast<float>(row + 1) / static_cast<float>(rows_);

            texcoords.emplace_back(u_left,  v_top);    // top-left
            texcoords.emplace_back(u_right, v_top);    // top-right
            texcoords.emplace_back(u_right, v_bottom); // bottom-right
            texcoords.emplace_back(u_left,  v_bottom); // bottom-left

            indices.push_back(static_cast<unsigned int>(base + 0));
            indices.push_back(static_cast<unsigned int>(base + 1));
            indices.push_back(static_cast<unsigned int>(base + 2));
            indices.push_back(static_cast<unsigned int>(base + 0));
            indices.push_back(static_cast<unsigned int>(base + 2));
            indices.push_back(static_cast<unsigned int>(base + 3));
        }
    }

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_positions_);
    glGenBuffers(1, &vbo_texcoords_);
    glGenBuffers(1, &ebo_);
    glGenTextures(1, &texture_id_);

    glBindVertexArray(vao_); // All attribute bindings and the element buffer attach to this VAO.

    glBindBuffer(GL_ARRAY_BUFFER, vbo_positions_);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), positions.data(), GL_STATIC_DRAW); // Upload positions once (they never change).
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), reinterpret_cast<void*>(0));       // Attribute 0 = 3D position.
    glEnableVertexAttribArray(0);                                                                          // Enable attribute 0.

    glBindBuffer(GL_ARRAY_BUFFER, vbo_texcoords_);
    glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(glm::vec2), texcoords.data(), GL_STATIC_DRAW); // Upload texture coordinates once.
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), reinterpret_cast<void*>(0));        // Attribute 1 = texcoord.
    glEnableVertexAttribArray(1);                                                                           // Enable attribute 1.

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW); // Element buffer stays bound to the VAO.

    index_count_ = indices.size();

    glBindVertexArray(0); // Unbind VAO to avoid accidental state leakage.

    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // Use nearest sampling so each cell stays crisp.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Clamp to avoid sampling outside the grid.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Allow tightly packed single-byte rows.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8,
                 static_cast<GLsizei>(cols_), static_cast<GLsizei>(rows_),
                 0, GL_RED, GL_UNSIGNED_BYTE, nullptr); // Allocate storage (data uploaded later).
    glBindTexture(GL_TEXTURE_2D, 0); // Leave texture unbound until render time.

    return true;
}

/** Releases all OpenGL objects owned by the grid mesh. */
void GridMesh2D::destroy()
{
    if (ebo_ != 0)
    {
        glDeleteBuffers(1, &ebo_); // Remove index buffer.
        ebo_ = 0;
    }
    if (vbo_texcoords_ != 0)
    {
        glDeleteBuffers(1, &vbo_texcoords_); // Remove texcoord buffer.
        vbo_texcoords_ = 0;
    }
    if (vbo_positions_ != 0)
    {
        glDeleteBuffers(1, &vbo_positions_); // Remove position buffer.
        vbo_positions_ = 0;
    }
    if (texture_id_ != 0)
    {
        glDeleteTextures(1, &texture_id_); // Remove mask texture.
        texture_id_ = 0;
    }
    if (vao_ != 0)
    {
        glDeleteVertexArrays(1, &vao_); // Remove vertex array object.
        vao_ = 0;
    }

    rows_ = 0;
    cols_ = 0;
    index_count_ = 0;
}

/** Uploads the latest air-mask texel data (0 = ground, 255 = air) to the GPU texture. */
void GridMesh2D::update_mask_texture(const std::vector<std::uint8_t>& mask_values)
{
    if (mask_values.size() != rows_ * cols_ || texture_id_ == 0)
    {
        return; // Ignore mismatched uploads.
    }

    glBindTexture(GL_TEXTURE_2D, texture_id_); // Select the mask texture to update.
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    0, 0,
                    static_cast<GLsizei>(cols_), static_cast<GLsizei>(rows_),
                    GL_RED, GL_UNSIGNED_BYTE, mask_values.data());
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind for cleanliness.
}

/** Binds the VAO and issues a draw call for the entire grid. */
void GridMesh2D::draw() const
{
    if (vao_ == 0 || index_count_ == 0)
    {
        return; // Nothing to draw if the mesh was not initialized.
    }

    glBindVertexArray(vao_); // Restore the attribute + index bindings.
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(index_count_), GL_UNSIGNED_INT, nullptr); // Render every cell.
    glBindVertexArray(0); // Leave a clean OpenGL state for the caller.
}

} // namespace viz
} // namespace snow
