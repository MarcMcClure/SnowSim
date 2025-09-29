#include "renderer.hpp"

#include "camera.hpp"
#include "cube_mesh.hpp"
#include "grid_mesh.hpp"
#include "shader_program.hpp"
#include "types.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
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

    ShaderProgram g_shader;
    CubeMesh g_cube_mesh;
    GridMesh2D g_air_mask_mesh;
    Camera g_camera;

    bool g_air_mask_ready = false;               // has the GPU grid been created?
    std::vector<float> g_air_mask_buffer;        // CPU staging for per-cell colors (size: ny*nx)

    float g_last_frame = 0.0f;
    float g_delta_time = 0.0f;

    bool g_first_mouse = true;
    double g_last_x = 0.0;
    double g_last_y = 0.0;

    const std::string kShaderDir = "resources/shaders/";
    constexpr float simWidthInModelSpace = 100.0f;

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

    if (!g_shader.load_from_files(kShaderDir + "basic_color_lit.vert",
                                  kShaderDir + "basic_color_lit.frag"))
    {
        shutdown();
        return false;
    }

    //initialize cube mesh
    if (!g_cube_mesh.initialize())
    {
        shutdown();
        return false;
    }

    //initialize air mesh mask
    if (g_air_mask_mesh.initialize(air_mask.ny, air_mask.nx, simWidthInModelSpace))
    {
        shutdown();
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, width, height);

    g_first_mouse = true;
    double cursor_x = 0.0;
    double cursor_y = 0.0;
    glfwGetCursorPos(g_window, &cursor_x, &cursor_y);
    g_last_x = cursor_x;
    g_last_y = cursor_y;

    g_last_frame = static_cast<float>(glfwGetTime());
    
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f); //set abckground color

    return true;
}

void shutdown()
{
    g_air_mask_mesh.destroy();
    g_air_mask_ready = false;
    g_air_mask_buffer.clear();

    g_cube_mesh.destroy();

    g_shader.destroy();

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
    if (!g_window || !g_shader.is_valid())
    {
        return;
    }

    render_air_mask(params, fields.air_mask);

    render_cube(params.light_direction,
                params.light_color,
                params.object_color);
}

void render_air_mask(const Params& params, const Field2D<std::uint8_t>& air_mask)
{
    // The air-mask overlay is a fixed-resolution grid that maps the simulation's
    // Field2D<uint8_t> (1 = air, 0 = ground) to per-vertex colors. We set this up once
    // and, every frame, only stream the color buffer.

    if (!g_window || !g_shader.is_valid())
    {
        return; // guard: renderer not initialized or shader failed to link
    }

    // Translate mask -> colors into the staging buffer. We keep this as scalars (0/1)
    // and let GridMesh2D expand to vec3 in update_cell_colors.
    for (std::size_t j = 0; j < air_mask.ny; ++j)
    {
        for (std::size_t i = 0; i < air_mask.nx; ++i)
        {
            const float value = (air_mask.in_bounds(i, j) && air_mask(i, j) != 0) ? 1.0f : 0.0f;
            g_air_mask_buffer[j * air_mask.nx + i] = value;
        }
    }

    // Upload per-cell colors to the GPU and draw with vertex colors enabled.
    g_air_mask_mesh.update_cell_colors(g_air_mask_buffer);

    g_shader.bind();
    const glm::mat4 model(1.0f);
    g_shader.set_uniform("uModel", model);
    g_shader.set_uniform("uView", view_matrix());
    g_shader.set_uniform("uProjection", projection_matrix());
    g_shader.set_uniform("uLightDirection", params.light_direction);
    g_shader.set_uniform("uLightColor", params.light_color);
    g_shader.set_uniform("uObjectColor", glm::vec3(1.0f));
    g_shader.set_uniform("uViewPos", camera_position());
    g_shader.set_uniform("uUseVertexColor", 1);

    g_air_mask_mesh.draw();

    // Restore default for subsequent draws (e.g., lit cube uses uniform color).
    g_shader.set_uniform("uUseVertexColor", 0);
}

void render_cube(const glm::vec3& light_direction,
                 const glm::vec3& light_color,
                 const glm::vec3& object_color)
{
    if (!g_window || !g_shader.is_valid())
    {
        return;
    }

    g_shader.bind();

    const glm::mat4 model(1.0f);
    g_shader.set_uniform("uModel", model);
    g_shader.set_uniform("uView", view_matrix());
    g_shader.set_uniform("uProjection", projection_matrix());
    g_shader.set_uniform("uLightDirection", light_direction);
    g_shader.set_uniform("uLightColor", light_color);
    g_shader.set_uniform("uObjectColor", object_color);
    g_shader.set_uniform("uViewPos", camera_position());

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
