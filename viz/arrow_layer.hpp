#pragma once

#include <cstddef>
#include <vector>
#include <glm/glm/glm.hpp>

namespace snow {
namespace viz {

class ArrowLayer
{
public:
    ArrowLayer() = default;
    ~ArrowLayer();

    ArrowLayer(const ArrowLayer&) = delete;
    ArrowLayer& operator=(const ArrowLayer&) = delete;

    bool initialize(std::size_t rows, std::size_t cols);
    void destroy();

    struct InstanceData
    {
        glm::vec2 center;    // Arrow center in the XY plane
        glm::vec2 direction; // Raw wind direction vector (not normalized)
        float length;        // World-space length of the arrow
        float density;       // Snow density used for coloring
    };

    void update_instances(const std::vector<InstanceData>& instances);
    void draw(std::size_t instance_count) const;

    void set_grid_metrics(float cell_size, float half_height, float half_width)
    {
        cell_size_ = cell_size;
        half_height_ = half_height;
        half_width_ = half_width;
    }

    float cell_size() const { return cell_size_; }
    float half_height() const { return half_height_; }
    float half_width() const { return half_width_; }

private:
    unsigned int vao_ = 0;
    unsigned int vbo_vertices_ = 0;
    unsigned int vbo_instances_ = 0;
    unsigned int ebo_ = 0;

    std::size_t rows_ = 0;
    std::size_t cols_ = 0;
    std::size_t index_count_ = 0;
    std::size_t instance_capacity_ = 0;

    float cell_size_ = 0.0f;
    float half_height_ = 0.0f;
    float half_width_ = 0.0f;
};

} // namespace viz
} // namespace snow

