#pragma once

#include <glm/glm/glm.hpp>

#include <string>

namespace snow {
namespace viz {

class ShaderProgram
{
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    bool load_from_files(const std::string& vertex_path, const std::string& fragment_path);
    void destroy();

    void bind() const;

    void set_uniform(const std::string& name, const glm::mat4& value) const;
    void set_uniform(const std::string& name, const glm::vec3& value) const;
    void set_uniform(const std::string& name, float value) const;
    void set_uniform(const std::string& name, int value) const;

    bool is_valid() const { return program_id_ != 0; }

private:
    unsigned int program_id_ = 0;
};

} // namespace viz
} // namespace snow
