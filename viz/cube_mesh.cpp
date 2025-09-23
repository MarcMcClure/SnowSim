#include "cube_mesh.hpp"

#include <cstddef>
#include <glad/gl.h>

namespace snow {
namespace viz {

namespace {
struct Vertex
{
    float position[3];
    float normal[3];
};

constexpr Vertex vertices[] = {
    // +X face
    {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
    // -X face
    {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}},
    {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}},
    {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}},
    // +Y face
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f, 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    // -Y face
    {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}},
    // +Z face
    {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{ 0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{ 0.5f,  0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    // -Z face
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
};

constexpr unsigned int indices[] = {
    0, 1, 2, 0, 2, 3,
    4, 5, 6, 4, 6, 7,
    8, 9,10, 8,10,11,
    12,13,14, 12,14,15,
    16,17,18, 16,18,19,
    20,21,22, 20,22,23
};

}

CubeMesh::~CubeMesh()
{
    destroy();
}

bool CubeMesh::initialize()
{
    destroy();

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return true;
}

void CubeMesh::destroy()
{
    if (ebo_ != 0)
    {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }
    if (vbo_ != 0)
    {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_ != 0)
    {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
}

void CubeMesh::draw() const
{
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sizeof(indices) / sizeof(unsigned int)), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

} // namespace viz
} // namespace snow
