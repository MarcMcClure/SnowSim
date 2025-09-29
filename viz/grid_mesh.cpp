#include "grid_mesh.hpp"

#include <glad/gl.h>

#include <vector>

namespace snow {
namespace viz {

namespace {
constexpr glm::vec3 kAirColor(0.2f, 0.5f, 0.9f);
constexpr glm::vec3 kGroundColor(0.5f, 0.35f, 0.1f);
}

GridMesh2D::~GridMesh2D()
{
    destroy();
}

bool GridMesh2D::initialize(std::size_t rows, std::size_t cols, float width)
{
    destroy();

    rows_ = rows;
    cols_ = cols;

    if (rows_ == 0 || cols_ == 0)
    {
        return false;
    }

    const float cell_size = width / static_cast<float>(cols_);
    const float half_width = width * 0.5f;
    const float total_height = cell_size * static_cast<float>(rows_);
    const float half_height = total_height * 0.5f;

    std::vector<glm::vec3> positions;
    positions.reserve(rows_ * cols_ * 4);

    std::vector<glm::vec3> normals(rows_ * cols_ * 4, glm::vec3(1.0f, 0.0f, 0.0f));
    std::vector<glm::vec3> colors(rows_ * cols_ * 4, kAirColor);

    std::vector<unsigned int> indices;
    indices.reserve(rows_ * cols_ * 6);

    for (std::size_t row = 0; row < rows_; ++row)
    {
        for (std::size_t col = 0; col < cols_; ++col)
        {
            const float y0 = half_height - static_cast<float>(row) * cell_size;
            const float y1 = y0 - cell_size;
            const float z0 = -half_width + static_cast<float>(col) * cell_size;
            const float z1 = z0 + cell_size;

            const std::size_t base = positions.size();
            positions.emplace_back(0.0f, y0, z0);
            positions.emplace_back(0.0f, y0, z1);
            positions.emplace_back(0.0f, y1, z1);
            positions.emplace_back(0.0f, y1, z0);

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
    glGenBuffers(1, &vbo_normals_);
    glGenBuffers(1, &vbo_colors_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_positions_);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_normals_);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_colors_);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    index_count_ = indices.size();

    glBindVertexArray(0);

    return true;
}

void GridMesh2D::destroy()
{
    if (ebo_ != 0)
    {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }
    if (vbo_colors_ != 0)
    {
        glDeleteBuffers(1, &vbo_colors_);
        vbo_colors_ = 0;
    }
    if (vbo_normals_ != 0)
    {
        glDeleteBuffers(1, &vbo_normals_);
        vbo_normals_ = 0;
    }
    if (vbo_positions_ != 0)
    {
        glDeleteBuffers(1, &vbo_positions_);
        vbo_positions_ = 0;
    }
    if (vao_ != 0)
    {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    rows_ = 0;
    cols_ = 0;
    index_count_ = 0;
}

void GridMesh2D::update_cell_colors(const std::vector<float>& mask_values)
{
    if (mask_values.size() != rows_ * cols_ || vbo_colors_ == 0)
    {
        return;
    }

    std::vector<glm::vec3> colors(rows_ * cols_ * 4);
    for (std::size_t row = 0; row < rows_; ++row)
    {
        for (std::size_t col = 0; col < cols_; ++col)
        {
            const std::size_t cell_index = row * cols_ + col;
            const glm::vec3 color = (mask_values[cell_index] > 0.5f) ? kAirColor : kGroundColor;
            const std::size_t base = cell_index * 4;
            colors[base + 0] = color;
            colors[base + 1] = color;
            colors[base + 2] = color;
            colors[base + 3] = color;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo_colors_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(glm::vec3), colors.data());
}

void GridMesh2D::draw() const
{
    if (vao_ == 0 || index_count_ == 0)
    {
        return;
    }

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(index_count_), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

} // namespace viz
} // namespace snow
