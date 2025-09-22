#include "renderer.hpp"

#include <GLEW/glew.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>

namespace snow {
namespace viz {

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

    glewExperimental = GL_TRUE;
    const GLenum glew_status = glewInit();
    if (glew_status != GLEW_OK)
    {
        std::cerr << "[viz] Failed to initialize GLEW: "
                  << reinterpret_cast<const char*>(glewGetErrorString(glew_status)) << '\n';
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

void end_frame()
{
    if (!g_window)
    {
        return;
    }

    glfwSwapBuffers(g_window);
}

} // namespace viz
} // namespace snow

