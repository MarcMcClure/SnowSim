#include "arrow_layer.hpp"

#include <glad/gl.h>

#include <cstddef>
#include <vector>

namespace snow {
namespace viz {

namespace
{
// Simple arrow pointing along +X in local space (length 1, width ~0.4)
const float kArrowVertices[] = {
    // x,    y
    -0.5f,  0.05f,
    0.2f,   0.05f,
    0.2f,   0.20f,
    0.5f,   0.00f,
    0.2f,  -0.20f,
    0.2f,  -0.05f,
    -0.5f, -0.05f
};

const unsigned int kArrowIndices[] = {
    0, 1, 5,
    0, 5, 6,
    1, 2, 4,
    1, 4, 5,
    2, 3, 4
};
} // namespace

ArrowLayer::~ArrowLayer()
{
    destroy();
}

bool ArrowLayer::initialize(std::size_t rows, std::size_t cols)
{
    destroy();

    rows_ = rows;
    cols_ = cols;
    index_count_ = sizeof(kArrowIndices) / sizeof(unsigned int);
    instance_capacity_ = rows * cols;

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(1, &vbo_vertices_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kArrowVertices), kArrowVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &ebo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kArrowIndices), kArrowIndices, GL_STATIC_DRAW);

    glGenBuffers(1, &vbo_instances_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instances_);
    glBufferData(GL_ARRAY_BUFFER, instance_capacity_ * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);

    const GLsizei stride = sizeof(InstanceData);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(InstanceData, center)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(InstanceData, direction)));
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(InstanceData, length)));
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(InstanceData, density)));

    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);

    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

void ArrowLayer::destroy()
{
    if (vbo_instances_ != 0)
    {
        glDeleteBuffers(1, &vbo_instances_);
        vbo_instances_ = 0;
    }
    if (ebo_ != 0)
    {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }
    if (vbo_vertices_ != 0)
    {
        glDeleteBuffers(1, &vbo_vertices_);
        vbo_vertices_ = 0;
    }
    if (vao_ != 0)
    {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    rows_ = cols_ = index_count_ = instance_capacity_ = 0;
    cell_size_ = half_height_ = half_width_ = 0.0f;
}

void ArrowLayer::update_instances(const std::vector<InstanceData>& instances)
{
    if (vao_ == 0 || vbo_instances_ == 0)
    {
        return;
    }

    const std::size_t required_bytes = instances.size() * sizeof(InstanceData);
    const std::size_t capacity_bytes = instance_capacity_ * sizeof(InstanceData);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instances_);
    if (required_bytes > capacity_bytes)
    {
        instance_capacity_ = instances.size();
        glBufferData(GL_ARRAY_BUFFER, required_bytes, instances.data(), GL_DYNAMIC_DRAW);
    }
    else
    {
        glBufferSubData(GL_ARRAY_BUFFER, 0, required_bytes, instances.data());
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ArrowLayer::draw(std::size_t instance_count) const
{
    if (vao_ == 0 || index_count_ == 0 || instance_count == 0)
    {
        return;
    }

    glBindVertexArray(vao_);
    glDrawElementsInstanced(GL_TRIANGLES,
                            static_cast<GLsizei>(index_count_),
                            GL_UNSIGNED_INT,
                            nullptr,
                            static_cast<GLsizei>(instance_count));
    glBindVertexArray(0);
}

} // namespace viz
} // namespace snow
