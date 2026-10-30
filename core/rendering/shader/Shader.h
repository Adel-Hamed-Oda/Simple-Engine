#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../../../Config.h"
#include "../../statics/FallBacks.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

class Shader
{
public:
    GLuint ID = 0;

    Shader() = default;

    Shader(const std::string& vertexCode, const std::string& fragmentCode)
    {
        if (vertexCode.empty() || fragmentCode.empty())
            return;

        GLuint vertex = CompileShader(vertexCode, GL_VERTEX_SHADER);
        if (vertex == 0)
            return;

        GLuint fragment = CompileShader(fragmentCode, GL_FRAGMENT_SHADER);
        if (fragment == 0)
        {
            glDeleteShader(vertex);
            return;
        }

        GLuint programID = glCreateProgram();
        glAttachShader(programID, vertex);
        glAttachShader(programID, fragment);
        glLinkProgram(programID);

        GLint success = 0;
        glGetProgramiv(programID, GL_LINK_STATUS, &success);
        if (!success)
        {
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << GetProgramLog(programID) << std::endl;
            glDeleteProgram(programID);
            programID = 0;
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);

        ID = programID;
    }

    ~Shader()
    {
        if (ID != 0)
            glDeleteProgram(ID);
    }

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept
        : ID(std::exchange(other.ID, 0))
    {
    }

    Shader& operator=(Shader&& other) noexcept
    {
        if (this != &other)
        {
            if (ID != 0)
                glDeleteProgram(ID);
            ID = std::exchange(other.ID, 0);
        }
        return *this;
    }

    // =========================================================================
    // Factory Methods (return caller-owned Shader*)
    // =========================================================================

    // Creates a shader from a base path (e.g. "shaders/Image" -> "shaders/Image.vert" & "shaders/Image.frag").
    static Shader* Create(const std::string& basePath)
    {
        return CreateFromFiles(basePath + ".vert", basePath + ".frag");
    }

    // Creates a shader from explicit vertex and fragment file paths relative to CONFIG::DIRECTORY::SHADERS.
    static Shader* CreateFromFiles(const std::string& vertexPath, const std::string& fragmentPath)
    {
        const std::string vertexCode = ReadFile(vertexPath);
        const std::string fragmentCode = ReadFile(fragmentPath);

        Shader* shader = new Shader(vertexCode, fragmentCode);
        if (!shader->IsValid())
        {
            std::cerr << "ERROR::SHADER::CREATION_FAILED: " << vertexPath << " / " << fragmentPath << std::endl;
            delete shader;
            return CreateFallback();
        }

        return shader;
    }

    // Creates a shader directly from vertex and fragment GLSL source strings.
    static Shader* CreateFromSource(const std::string& vertexCode, const std::string& fragmentCode)
    {
        Shader* shader = new Shader(vertexCode, fragmentCode);
        if (!shader->IsValid())
        {
            std::cerr << "ERROR::SHADER::CREATION_FROM_SOURCE_FAILED" << std::endl;
            delete shader;
            return CreateFallback();
        }

        return shader;
    }

    // Compiles and returns the fallback checker shader.
    static Shader* CreateFallback()
    {
        Shader* fallback = new Shader(FallBacks::VertexShaderCode, FallBacks::FragmentShaderCode);
        if (!fallback->IsValid())
            std::cerr << "ERROR::SHADER::FALLBACK_SHADER_CREATION_FAILED" << std::endl;
        return fallback;
    }

    // =========================================================================
    // Instance Methods
    // =========================================================================

    bool IsValid() const
    {
        return ID != 0;
    }

    void use() const
    {
        if (ID != 0)
            glUseProgram(ID);
    }

    void setBool(const std::string& uniformName, bool value) const
    {
        GLint location = GetUniformLocation(uniformName);
        if (location != -1)
            glUniform1i(location, static_cast<int>(value));
    }

    void setInt(const std::string& uniformName, int value) const
    {
        GLint location = GetUniformLocation(uniformName);
        if (location != -1)
            glUniform1i(location, value);
    }

    void setFloat(const std::string& uniformName, float value) const
    {
        GLint location = GetUniformLocation(uniformName);
        if (location != -1)
            glUniform1f(location, value);
    }

    void setVec2(const std::string& uniformName, const glm::vec2& value) const
    {
        GLint location = GetUniformLocation(uniformName);
        if (location != -1)
            glUniform2fv(location, 1, glm::value_ptr(value));
    }

    void setVec3(const std::string& uniformName, const glm::vec3& value) const
    {
        GLint location = GetUniformLocation(uniformName);
        if (location != -1)
            glUniform3fv(location, 1, glm::value_ptr(value));
    }

    void setVec4(const std::string& uniformName, const glm::vec4& value) const
    {
        GLint location = GetUniformLocation(uniformName);
        if (location != -1)
            glUniform4fv(location, 1, glm::value_ptr(value));
    }

    void setMat4(const std::string& uniformName, const glm::mat4& value) const
    {
        GLint location = GetUniformLocation(uniformName);
        if (location != -1)
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }

private:
    GLint GetUniformLocation(const std::string& uniformName) const
    {
        if (ID == 0)
            return -1;

        return glGetUniformLocation(ID, uniformName.c_str());
    }

    static std::string ReadFile(const std::string& filepath)
    {
        const std::string fullPath = CONFIG::DIRECTORY::SHADERS + filepath;

        std::ifstream file(fullPath, std::ios::in | std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "ERROR::SHADER::FILE_NOT_FOUND_OR_READ_FAILED: " << fullPath << std::endl;
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    static std::string GetShaderLog(GLuint shader)
    {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        if (length <= 1)
            return "";

        std::vector<char> log(static_cast<size_t>(length));
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        return std::string(log.data());
    }

    static std::string GetProgramLog(GLuint program)
    {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        if (length <= 1)
            return "";

        std::vector<char> log(static_cast<size_t>(length));
        glGetProgramInfoLog(program, length, nullptr, log.data());
        return std::string(log.data());
    }

    static GLuint CompileShader(const std::string& source, GLenum shaderType)
    {
        const char* shaderCode = source.c_str();
        GLuint shader = glCreateShader(shaderType);
        glShaderSource(shader, 1, &shaderCode, nullptr);
        glCompileShader(shader);

        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            const char* shaderTypeName = (shaderType == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
            std::cerr << "ERROR::SHADER::" << shaderTypeName << "::COMPILATION_FAILED\n"
                      << GetShaderLog(shader) << std::endl;
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }
};
