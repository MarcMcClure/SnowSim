#include "camera.hpp"

#include <GLFW/glfw3.h>
// TODO: something about these includes are screwed up
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

namespace snow {
namespace viz {

Camera::Camera()
    : position_(0.0f, 0.0f, 20.0f),
      front_(0.0f, 0.0f, -1.0f),
      up_(0.0f, 1.0f, 0.0f),
      world_up_(0.0f, 1.0f, 0.0f),
      yaw_(-90.0f),
      pitch_(0.0f),
      movement_speed_(10.0f),
      mouse_sensitivity_(0.1f),
      zoom_(45.0f)
{
    update_camera_vectors();
}

void Camera::update(GLFWwindow* window, float delta_time)
{
    const float velocity = movement_speed_ * delta_time;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        position_ += front_ * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        position_ -= front_ * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        position_ -= right_ * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        position_ += right_ * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        position_ -= up_ * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        position_ += up_ * velocity;
    }
}

glm::mat4 Camera::view_matrix() const
{
    return glm::lookAt(position_, position_ + front_, up_);
}

glm::mat4 Camera::projection_matrix(float aspect) const
{
    return glm::perspective(glm::radians(zoom_), aspect, 0.1f, 500.0f);
}

glm::vec3 Camera::position() const
{
    return position_;
}

void Camera::process_mouse_movement(float x_offset, float y_offset)
{
    x_offset *= mouse_sensitivity_;
    y_offset *= mouse_sensitivity_;

    yaw_ += x_offset;
    pitch_ += y_offset;

    if (pitch_ > 89.0f) pitch_ = 89.0f;
    if (pitch_ < -89.0f) pitch_ = -89.0f;

    update_camera_vectors();
}

void Camera::process_scroll(float y_offset)
{
    zoom_ -= static_cast<float>(y_offset);
    if (zoom_ < 1.0f) zoom_ = 1.0f;
    if (zoom_ > 45.0f) zoom_ = 45.0f;
}

void Camera::update_camera_vectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front.y = sin(glm::radians(pitch_));
    front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front_ = glm::normalize(front);
    right_ = glm::normalize(glm::cross(front_, world_up_));
    up_ = glm::normalize(glm::cross(right_, front_));
}

} // namespace viz
} // namespace snow
