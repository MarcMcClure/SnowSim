#include "renderer.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>

namespace snow {
namespace viz {

// TODO: use Instanced rendering for arrow to improve performance.
// TODO: use ImGui for gui side bar.

namespace {
    GLFWwindow* g_window = nullptr;
    std::int32_t g_width = 0;
    std::int32_t g_height = 0;

    // GLFW callback used when the window resizes; updates cached dimensions and resets the viewport.
    void frame_buffer_size_callback(GLFWwindow*, int width, int height)
    {
        g_width = width;
        g_height = height;
        glViewport(0, 0, std::max(1, width), std::max(1, height));
    }
}

bool initialize(std::int32_t width, std::int32_t height, const char* title)
{
    if (g_window) // if already intitilized return true.
    {
        return true;
    }

    if (!glfwInit())    // attempt to initialize the glfw lib
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
    glfwSwapInterval(1);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cerr << "[viz] Failed to initialize GLAD\n";
        shutdown();
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, width, height);

    return true;
}

void shutdown()
{
    if (g_window)
    {
        glfwDestroyWindow(g_window);
        g_window = nullptr;
    }
    glfwTerminate();
}

void poll_events()
{
    glfwPollEvents();
}

void process_input()
{
    
}

bool should_close()
{
    return g_window && glfwWindowShouldClose(g_window);
}

void begin_frame()
{
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void end_frame()
{
    glfwSwapBuffers(g_window);
}

bool visualizer_is_closed(){
    return g_window == nullptr;
}


} // namespace viz
} // namespace snow
