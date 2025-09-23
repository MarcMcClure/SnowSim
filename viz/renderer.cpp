#include "renderer.hpp"

#include "camera.hpp"
#include "cube_mesh.hpp"
#include "shader_program.hpp"
#include "types.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>
#include <string>

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
    Camera g_camera;

    float g_last_frame = 0.0f;
    float g_delta_time = 0.0f;

    bool g_first_mouse = true;
    double g_last_x = 0.0;
    double g_last_y = 0.0;

    const std::string kShaderDir = "resources/shaders/";

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

    if (!g_cube_mesh.initialize())
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

    return true;
}

void shutdown()
{
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

    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void render_frame(const Params& params)
{
    render_cube(params.light_direction,
                params.light_color,
                params.object_color);
}

void end_frame()
{
    if (!g_window)
    {
        return;
    }

    glfwSwapBuffers(g_window);
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
