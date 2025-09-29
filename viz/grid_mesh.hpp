#pragma once

#include <cstddef>
#include <vector>
#include <glm/glm/glm.hpp>

namespace snow {
namespace viz {

class GridMesh2D
{
public:
    GridMesh2D() = default;
    ~GridMesh2D();

    GridMesh2D(const GridMesh2D&) = delete;
    GridMesh2D& operator=(const GridMesh2D&) = delete;

    bool initialize(std::size_t rows, std::size_t cols, float width);
    void destroy();

    void update_cell_colors(const std::vector<float>& mask_values);

    void draw() const;

private:
    unsigned int vao_ = 0;
    unsigned int vbo_positions_ = 0;
    unsigned int vbo_normals_ = 0;
    unsigned int vbo_colors_ = 0;
    unsigned int ebo_ = 0;

    std::size_t rows_ = 0;
    std::size_t cols_ = 0;
    std::size_t index_count_ = 0;
};

} // namespace viz
} // namespace snow
