#pragma once

#include <cstdint>
#include <glm/glm/glm.hpp>

namespace snow {
namespace viz {

bool initialize(std::int32_t width, std::int32_t height, const char* title);
void shutdown();

void poll_events();
void process_input();
bool should_close();

void begin_frame();
void end_frame();

void render_cube(const glm::vec3& light_direction,
                 const glm::vec3& light_color,
                 const glm::vec3& object_color);

glm::mat4 view_matrix();
glm::mat4 projection_matrix();
glm::vec3 camera_position();

bool visualizer_is_closed();

} // namespace viz
} // namespace snow

