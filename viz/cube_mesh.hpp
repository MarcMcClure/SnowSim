#pragma once

namespace snow {
namespace viz {

class CubeMesh
{
public:
    CubeMesh() = default;
    ~CubeMesh();

    CubeMesh(const CubeMesh&) = delete;
    CubeMesh& operator=(const CubeMesh&) = delete;

    bool initialize();
    void destroy();

    void draw() const;

private:
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int ebo_ = 0;
};

} // namespace viz
} // namespace snow

