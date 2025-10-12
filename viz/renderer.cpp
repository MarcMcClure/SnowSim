#include "renderer.hpp"

#include "camera.hpp"
#include "cube_mesh.hpp"
#include "grid_mesh.hpp"
#include "arrow_layer.hpp"
#include "shader_program.hpp"
#include "types.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace snow {
namespace viz {

// TODO: use Instanced rendering for arrow to improve performance.
// TODO: use ImGui for gui side bar.

namespace {
    GLFWwindow* g_window = nullptr;
    std::int32_t g_width = 0;
    std::int32_t g_height = 0;

    ShaderProgram g_cube_shader;
    ShaderProgram g_air_mask_shader;
    ShaderProgram g_arrow_shader;
    CubeMesh g_cube_mesh;
    GridMesh2D g_air_mask_mesh;
    ArrowLayer g_arrow_layer;
    Camera g_camera;

    bool g_air_mask_initialized = false;
    std::vector<std::uint8_t> g_air_mask_texture_data;

    bool g_arrow_initialized = false;
    std::vector<ArrowLayer::InstanceData> g_arrow_instances;

    float g_last_frame = 0.0f;
    float g_delta_time = 0.0f;

    bool g_first_mouse = true;
    double g_last_x = 0.0;
    double g_last_y = 0.0;

    const std::string kShaderDir = "resources/shaders/";
    constexpr float simWidthInModelSpace = 100.0f;
    constexpr GLuint kAirMaskTextureUnit = 0;
    // TODO: rename k-prefixed constants to snake_case for consistency.
    // TODO: support rectangular visualization cells (distinct width/height).
    constexpr float kArrowPlaneZ = 0.1f;           // arrows sit 0.1 units in front of the mask
    constexpr float kArrowDensityMax = 2.0e-0f;    // density mapped to red
    constexpr float kArrowReferenceWind = 60.0f;  // wind magnitude that yields half-cell arrow length
    constexpr float kArrowMinLen = 0.1f;  // minimum arrow length as a percentage of the cell width

    void frame_buffer_size_callback(GLFWwindow*, int width, int height)
    {
        g_width = width;
        g_height = height;
        glViewport(0, 0, std::max(1, width), std::max(1, height));
    }

    void mouse_callback(GLFWwindow*, double xpos, double ypos)
    {
        if (g_first_mouse)
        {
            g_last_x = xpos;
            g_last_y = ypos;
            g_first_mouse = false;
        }

        float x_offset = static_cast<float>(xpos - g_last_x);
        float y_offset = static_cast<float>(g_last_y - ypos);

        g_last_x = xpos;
        g_last_y = ypos;

        g_camera.process_mouse_movement(x_offset, y_offset);
    }

    void scroll_callback(GLFWwindow*, double /*xoffset*/, double yoffset)
    {
        g_camera.process_scroll(static_cast<float>(yoffset));
    }

    void update_timing()
    {
        const float current = static_cast<float>(glfwGetTime());
        g_delta_time = current - g_last_frame;
        g_last_frame = current;
    }
}

bool initialize(std::int32_t width, std::int32_t height, const char* title)
{
    if (g_window)  // if already intitilized return true.
    {
        return true;
    }

    if (!glfwInit())     // attempt to initialize the glfw lib
    {
        std::cerr << "[viz] Failed to initialize GLFW\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    g_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!g_window)
    {
        std::cerr << "[viz] Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    g_width = width;
    g_height = height;

    glfwMakeContextCurrent(g_window);
    glfwSetFramebufferSizeCallback(g_window, frame_buffer_size_callback);
    glfwSetCursorPosCallback(g_window, mouse_callback);
    glfwSetScrollCallback(g_window, scroll_callback);
    glfwSwapInterval(1);
    glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cerr << "[viz] Failed to initialize GLAD\n";
        shutdown();
        return false;
    }

    // if (!g_cube_shader.load_from_files(kShaderDir + "basic_color_lit.vert",
    //                                    kShaderDir + "basic_color_lit.frag"))
    // {
    //     shutdown();
    //     return false;
    // }

    // if (!g_cube_mesh.initialize())
    // {
    //     shutdown();
    //     return false;
    // }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, width, height);

    g_first_mouse = true;
    double cursor_x = 0.0;
    double cursor_y = 0.0;
    glfwGetCursorPos(g_window, &cursor_x, &cursor_y);
    g_last_x = cursor_x;
    g_last_y = cursor_y;

    g_last_frame = static_cast<float>(glfwGetTime());

    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);

    return true;
}

/**
 * Prepares the GPU resources required to render the 2D air-mask overlay.
 * @param rows Number of cells along the vertical (Y) axis.
 * @param cols Number of cells along the depth (Z) axis.
 * Must be called after the OpenGL context is ready and before the first render.
 */
void initialize_air_mask_resources(std::size_t rows, std::size_t cols)
{
    if (!g_window)
    {
        return; // No context ? nothing to do.
    }

    if (g_air_mask_initialized)
    {
        return; // Already set up.
    }

    if (!g_air_mask_shader.is_valid())
    {
        if (!g_air_mask_shader.load_from_files(kShaderDir + "air_mask.vert",
                                               kShaderDir + "air_mask.frag"))
        {
            std::cerr << "[viz] Failed to load air-mask shader\n";
            return;
        }
    }

    if (!g_air_mask_mesh.initialize(rows, cols, simWidthInModelSpace))
    {
        std::cerr << "[viz] Failed to initialize air-mask mesh\n";
        return;
    }

    g_air_mask_texture_data.assign(rows * cols, 0); // Allocate staging buffer (one byte per cell).

    g_air_mask_shader.bind();
    g_air_mask_shader.set_uniform("uMaskTexture", static_cast<int>(kAirMaskTextureUnit)); // sampler2D ? texture unit mapping.
    g_air_mask_shader.set_uniform("uAirColor", glm::vec3(0.2f, 0.5f, 0.9f));             // Default air color.
    g_air_mask_shader.set_uniform("uGroundColor", glm::vec3(0.5f, 0.35f, 0.1f));          // Default ground color.
    glUseProgram(0);

    
    g_air_mask_initialized = true;
}

void initialize_arrow_resources(std::size_t rows, std::size_t cols)
{
    if (!g_window)
    {
        return;
    }

    if (g_arrow_initialized)
    {
        return;
    }

    if (!g_arrow_shader.is_valid())
    {
        if (!g_arrow_shader.load_from_files(kShaderDir + "arrow.vert",
                                            kShaderDir + "arrow.frag"))
        {
            std::cerr << "[viz] Failed to load arrow shader\n";
            return;
        }
    }

    if (!g_arrow_layer.initialize(rows, cols))
    {
        std::cerr << "[viz] Failed to initialize arrow layer\n";
        return;
    }

    const float cell_size = simWidthInModelSpace / static_cast<float>(cols);
    const float half_width = simWidthInModelSpace * 0.5f;
    const float total_height = cell_size * static_cast<float>(rows);
    const float half_height = total_height * 0.5f;
    g_arrow_layer.set_grid_metrics(cell_size, half_height, half_width);

    g_arrow_instances.assign(rows * cols, {});

    g_arrow_shader.bind();
    g_arrow_shader.set_uniform("uPlaneZ", kArrowPlaneZ);
    g_arrow_shader.set_uniform("uDensityMax", kArrowDensityMax);
    g_arrow_shader.set_uniform("uArrowHalfWidth", cell_size * 0.5f);
    glUseProgram(0);

    g_arrow_initialized = true;
}

void shutdown()
{
    g_air_mask_mesh.destroy();
    g_air_mask_texture_data.clear();
    g_air_mask_shader.destroy();
    g_air_mask_initialized = false;

    g_arrow_layer.destroy();
    g_arrow_instances.clear();
    g_arrow_shader.destroy();
    g_arrow_initialized = false;

    g_cube_mesh.destroy();
    g_cube_shader.destroy();

    if (g_window)
    {
        glfwDestroyWindow(g_window);
        g_window = nullptr;
    }
    glfwTerminate();
}

void poll_events()
{
    if (!g_window)
    {
        return;
    }
    glfwPollEvents();
}

void process_input()
{
    if (!g_window)
    {
        return;
    }

    update_timing();

    // TODO: Update ESC handling so it releases the cursor but leaves the window running.
    if (glfwGetKey(g_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(g_window, GLFW_TRUE);
    }

    g_camera.update(g_window, g_delta_time);
}

bool should_close()
{
    return g_window && glfwWindowShouldClose(g_window);
}

void begin_frame()
{
    if (!g_window)
    {
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void render_frame(const Params& params, const Fields& fields)
{
    if (!g_window)
    {
        return;
    }
    if (g_air_mask_initialized)
    {
        render_air_mask(params, fields.air_mask);
    }

    if (g_arrow_initialized)
    {
        render_arrows(params, fields);
    }

    // render_cube(params.light_direction,
    //             params.light_color,
    //             params.object_color);
}

/**
 * Streams the current air-mask into a texture and renders the grid overlay in one draw call.
 * @param params Simulation parameters (used for camera matrices).
 * @param air_mask Field containing the per-cell air/ground classification.
 */
void render_air_mask(const Params& params, const Field2D<std::uint8_t>& air_mask)
{
    (void)params; // Camera matrices come from the global camera; params are unused for now.

    const std::size_t cell_count = air_mask.nx * air_mask.ny;
    if (cell_count == 0)
    {
        return;
    }

    if (g_air_mask_texture_data.size() != cell_count)
    {
        g_air_mask_texture_data.resize(cell_count, 0); // Resize staging buffer if the grid dimensions changed.
    }

    for (std::size_t j = 0; j < air_mask.ny; ++j)
    {
        for (std::size_t i = 0; i < air_mask.nx; ++i)
        {
            const bool is_air_cell = air_mask.in_bounds(i, j) && air_mask(i, j) != 0;
            g_air_mask_texture_data[j * air_mask.nx + i] = is_air_cell ? 255u : 0u; // 255 = air, 0 = ground.
        }
    }

    g_air_mask_mesh.update_mask_texture(g_air_mask_texture_data); // Upload new texel values.

    g_air_mask_shader.bind();
    g_air_mask_shader.set_uniform("uModel", glm::mat4(1.0f)); // Keep the grid centered on the origin.
    g_air_mask_shader.set_uniform("uView", view_matrix());      // Camera view.
    g_air_mask_shader.set_uniform("uProjection", projection_matrix()); // Camera projection.

    glActiveTexture(GL_TEXTURE0 + kAirMaskTextureUnit);
    glBindTexture(GL_TEXTURE_2D, g_air_mask_mesh.texture_id()); // Bind mask texture so the shader can sample it.

    g_air_mask_mesh.draw(); // Single draw call covers the entire grid.

    glBindTexture(GL_TEXTURE_2D, 0); // Optional: unbind to keep state tidy.
}

void render_arrows(const Params& params, const Fields& fields)
{
    (void)params; // currently unused; retained for symmetry

    if (!g_arrow_initialized)
    {
        return;
    }

    const std::size_t rows = fields.air_mask.ny;
    const std::size_t cols = fields.air_mask.nx;
    if (rows == 0 || cols == 0)
    {
        return;
    }

    if (g_arrow_instances.size() != rows * cols)
    {
        g_arrow_instances.assign(rows * cols, {});
    }

    const float cell_size = g_arrow_layer.cell_size();
    const float half_width = g_arrow_layer.half_width();
    const float half_height = g_arrow_layer.half_height();

    const float inv_reference_wind = cell_size / kArrowReferenceWind; // magnitude -> length factor

    std::size_t instance_index = 0;
    for (std::size_t j = 0; j < rows; ++j)
    {
        const std::size_t field_row = rows - 1 - j;
        for (std::size_t i = 0; i < cols; ++i, ++instance_index)
        {
            //TODO: might need to flip horozontally here
            ArrowLayer::InstanceData data{};

            const bool is_air = fields.air_mask.in_bounds(i, field_row) && fields.air_mask(i, field_row) != 0;

            const float x_center = -half_width + (static_cast<float>(i) + 0.5f) * cell_size;
            const float y_center = half_height - (static_cast<float>(j) + 0.5f) * cell_size;
            data.center = glm::vec2(x_center, y_center);

            float vx = 0.0f;
            float vy = 0.0f;
            int samples_x = 0;
            int samples_y = 0;
            if (fields.snow_transport_speed_x.in_bounds(i, field_row))
            {
                vx += fields.snow_transport_speed_x(i, field_row);
                ++samples_x;
            }
            if (fields.snow_transport_speed_x.in_bounds(i + 1, field_row))
            {
                vx += fields.snow_transport_speed_x(i + 1, field_row);
                ++samples_x;
            }
            if (fields.snow_transport_speed_y.in_bounds(i, field_row))
            {
                vy += fields.snow_transport_speed_y(i, field_row);
                ++samples_y;
            }
            if (fields.snow_transport_speed_y.in_bounds(i, field_row + 1))
            {
                vy += fields.snow_transport_speed_y(i, field_row + 1);
                ++samples_y;
            }
            if (samples_x > 0)
            {
                vx /= static_cast<float>(samples_x);
            }
            if (samples_y > 0)
            {
                vy /= static_cast<float>(samples_y);
            }

            data.direction = glm::vec2(vx, vy);

            float magnitude = std::sqrt(vx * vx + vy * vy);
            float length = magnitude * inv_reference_wind;
            if (!is_air || magnitude < 1e-5f)
            {
                length = 0.0f;
                data.direction = glm::vec2(1.0f, 0.0f);
            }
            else
            {
                const float minimum_arrow_length = cell_size * kArrowMinLen; // enforce a visible baseline arrow length.
                if (length < minimum_arrow_length)
                {
                    length = minimum_arrow_length;
                }
            }

            data.length = length;
            data.density = (is_air && fields.snow_density.in_bounds(i, field_row)) ? fields.snow_density(i, field_row) : 0.0f;

            g_arrow_instances[instance_index] = data;
        }
    }

    g_arrow_layer.update_instances(g_arrow_instances);

    g_arrow_shader.bind();
    g_arrow_shader.set_uniform("uPlaneZ", kArrowPlaneZ);
    g_arrow_shader.set_uniform("uDensityMax", kArrowDensityMax);
    g_arrow_shader.set_uniform("uArrowHalfWidth", cell_size * 0.5f);
    g_arrow_shader.set_uniform("uView", view_matrix());
    g_arrow_shader.set_uniform("uProjection", projection_matrix());

    g_arrow_layer.draw(rows * cols);
}

void render_cube(const glm::vec3& light_direction,
                 const glm::vec3& light_color,
                 const glm::vec3& object_color)
{
    if (!g_window || !g_cube_shader.is_valid())
    {
        return;
    }

    g_cube_shader.bind();

    const glm::mat4 model(1.0f);
    g_cube_shader.set_uniform("uModel", model);
    g_cube_shader.set_uniform("uView", view_matrix());
    g_cube_shader.set_uniform("uProjection", projection_matrix());
    g_cube_shader.set_uniform("uLightDirection", light_direction);
    g_cube_shader.set_uniform("uLightColor", light_color);
    g_cube_shader.set_uniform("uObjectColor", object_color);
    g_cube_shader.set_uniform("uViewPos", camera_position());
    g_cube_shader.set_uniform("uUseVertexColor", 0);

    g_cube_mesh.draw();
}

void end_frame()
{
    if (!g_window)
    {
        return;
    }

    glfwSwapBuffers(g_window);
}

glm::mat4 view_matrix()
{
    return g_camera.view_matrix();
}

glm::mat4 projection_matrix()
{
    const float aspect = (g_height != 0) ? static_cast<float>(g_width) / static_cast<float>(g_height) : 1.0f;
    return g_camera.projection_matrix(aspect);
}

glm::vec3 camera_position()
{
    return g_camera.position();
}


bool visualizer_is_closed()
{
    return g_window == nullptr;
}

} // namespace viz
} // namespace snow

