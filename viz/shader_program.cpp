#include "shader_program.hpp"

#include <glad/gl.h>

#include <fstream>
#include <iostream>
#include <sstream>

namespace snow {
namespace viz {
namespace {

std::string load_file(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
    {
        std::cerr << "[viz] Failed to open shader file: " << path << "\n";
        return {};
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

bool compile_shader(GLuint shader, const std::string& source)
{
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        std::cerr << "[viz] Shader compilation failed: " << info_log << "\n";
        return false;
    }
    return true;
}

}

ShaderProgram::~ShaderProgram()
{
    destroy();
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
{
    program_id_ = other.program_id_;
    other.program_id_ = 0;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        program_id_ = other.program_id_;
        other.program_id_ = 0;
    }
    return *this;
}

bool ShaderProgram::load_from_files(const std::string& vertex_path, const std::string& fragment_path)
{
    destroy();

    const std::string vertex_source = load_file(vertex_path);
    const std::string fragment_source = load_file(fragment_path);
    if (vertex_source.empty() || fragment_source.empty())
    {
        return false;
    }

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    bool success = compile_shader(vertex_shader, vertex_source) &&
                   compile_shader(fragment_shader, fragment_source);

    if (!success)
    {
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return false;
    }

    program_id_ = glCreateProgram();
    glAttachShader(program_id_, vertex_shader);
    glAttachShader(program_id_, fragment_shader);
    glLinkProgram(program_id_);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    GLint link_status = 0;
    glGetProgramiv(program_id_, GL_LINK_STATUS, &link_status);
    if (!link_status)
    {
        char info_log[512];
        glGetProgramInfoLog(program_id_, 512, nullptr, info_log);
        std::cerr << "[viz] Shader linking failed: " << info_log << "\n";
        destroy();
        return false;
    }

    return true;
}

void ShaderProgram::destroy()
{
    if (program_id_ != 0)
    {
        glDeleteProgram(program_id_);
        program_id_ = 0;
    }
}

void ShaderProgram::bind() const
{
    glUseProgram(program_id_);
}

void ShaderProgram::set_uniform(const std::string& name, const glm::mat4& value) const
{
    GLint location = glGetUniformLocation(program_id_, name.c_str());
    if (location != -1)
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
    }
}

void ShaderProgram::set_uniform(const std::string& name, const glm::vec3& value) const
{
    GLint location = glGetUniformLocation(program_id_, name.c_str());
    if (location != -1)
    {
        glUniform3fv(location, 1, &value[0]);
    }
}

void ShaderProgram::set_uniform(const std::string& name, float value) const
{
    GLint location = glGetUniformLocation(program_id_, name.c_str());
    if (location != -1)
    {
        glUniform1f(location, value);
    }
}

void ShaderProgram::set_uniform(const std::string& name, int value) const
{
    GLint location = glGetUniformLocation(program_id_, name.c_str());
    if (location != -1)
    {
        glUniform1i(location, value);
    }
}

} // namespace viz
} // namespace snow
