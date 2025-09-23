#pragma once

#include <glm/glm/glm.hpp>

struct GLFWwindow;

namespace snow {
namespace viz {

class Camera
{
public:
    Camera();

    void update(GLFWwindow* window, float delta_time);
    glm::mat4 view_matrix() const;
    glm::mat4 projection_matrix(float aspect) const;
    glm::vec3 position() const;

    void process_mouse_movement(float x_offset, float y_offset);
    void process_scroll(float y_offset);

private:
    void update_camera_vectors();

    glm::vec3 position_;
    glm::vec3 front_;
    glm::vec3 up_;
    glm::vec3 right_;
    glm::vec3 world_up_;

    float yaw_;
    float pitch_;
    float movement_speed_;
    float mouse_sensitivity_;
    float zoom_;
};

} // namespace viz
} // namespace snow
