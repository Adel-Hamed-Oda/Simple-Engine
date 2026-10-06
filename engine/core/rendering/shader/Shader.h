#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

class Shader
{
public:
    unsigned int ID = 0;

    Shader() = default;

    Shader(const std::string& vertexPath, const std::string& fragmentPath)
    {
        std::string vertexCode = ReadFile(vertexPath.c_str());
        std::string fragmentCode = ReadFile(fragmentPath.c_str());

        if (vertexCode.empty() || fragmentCode.empty())
            return;

        unsigned int vertex = CompileShader(vertexCode, GL_VERTEX_SHADER);
        if (vertex == 0)
            return;

        unsigned int fragment = CompileShader(fragmentCode, GL_FRAGMENT_SHADER);
        if (fragment == 0)
        {
            glDeleteShader(vertex);
            return;
        }

        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);

        int success;
        char infoLog[512];
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(ID, 512, nullptr, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
            glDeleteProgram(ID);
            ID = 0;
            glDeleteShader(vertex);
            glDeleteShader(fragment);
            return;
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    // =========================================================================
    // Static Storage & Shader Management
    // =========================================================================

    // Creates a shader from a base path (e.g. "shaders/default" -> "shaders/default.vert" & "shaders/default.frag")
    // and stores it in the lookup map.
    static Shader* Create(const std::string& path, const std::string& name = "")
    {
        std::string name_t = name.empty() ? path : name;
        return Create(path + ".vert", path + ".frag", name_t);
    }

    // Creates a shader with explicit vertex and fragment paths and stores it in the lookup map.
    static Shader* Create(const std::string& vertexPath, const std::string& fragmentPath, const std::string& name)
    {
        if (shaders.find(name) != shaders.end())
        {
            std::cerr << "ERROR::SHADER::ALREADY_EXISTS: " << name << std::endl;
            return &shaders[name];
        }
        
        Shader shader(vertexPath, fragmentPath);
        if (shader.ID == 0)
        {
            std::cerr << "ERROR::SHADER::CREATION_FAILED: " << name << std::endl;
            return nullptr;
        }

        auto [it, inserted] = shaders.insert_or_assign(name, std::move(shader));
        return &it->second;
    }

    // Look up shader in map. Returns nullptr and logs error if missing.
    static Shader* Get(const std::string& name)
    {
        auto it = shaders.find(name);
        if (it == shaders.end())
        {
            std::cerr << "ERROR::SHADER::NOT_FOUND: " << name << std::endl;
            return nullptr;
        }
        return &it->second;
    }

    // =========================================================================
    // Shader Instance Methods
    // =========================================================================

    const std::string& GetName() const
    {
        return name;
    }

    void use() const
    {
        if (ID != 0)
            glUseProgram(ID);
    }

    void setBool(const std::string& uniformName, bool value)
    {
        GLint location = GetUniformLocation(uniformName);
        if (location != -1)
            glUniform1i(location, static_cast<int>(value));
    }

    void setInt(const std::string& uniformName, int value)
    {
        GLint location = GetUniformLocation(uniformName);
        if (location != -1)
            glUniform1i(location, value);
    }

    void setFloat(const std::string& uniformName, float value)
    {
        GLint location = GetUniformLocation(uniformName);
        if (location != -1)
            glUniform1f(location, value);
    }

    void setVec2(const std::string& uniformName, const glm::vec2& value)
    {
        GLint location = GetUniformLocation(uniformName);
        if (location != -1)
            glUniform2fv(location, 1, glm::value_ptr(value));
    }

    void setVec3(const std::string& uniformName, const glm::vec3& value)
    {
        GLint location = GetUniformLocation(uniformName);
        if (location != -1)
            glUniform3fv(location, 1, glm::value_ptr(value));
    }

    void setVec4(const std::string& uniformName, const glm::vec4& value)
    {
        GLint location = GetUniformLocation(uniformName);
        if (location != -1)
            glUniform4fv(location, 1, glm::value_ptr(value));
    }

    void setMat4(const std::string& uniformName, const glm::mat4& value)
    {
        GLint location = GetUniformLocation(uniformName);
        if (location != -1)
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }

private:
    inline static std::unordered_map<std::string, Shader> shaders;

    std::string name;
    std::unordered_map<std::string, GLint> uniformLocations;

    GLint GetUniformLocation(const std::string& uniformName)
    {
        auto it = uniformLocations.find(uniformName);
        if (it != uniformLocations.end())
            return it->second;

        GLint location = glGetUniformLocation(ID, uniformName.c_str());
        uniformLocations[uniformName] = location;
        return location;
    }

    static std::string ReadFile(const char* filepath)
    {
        std::string fullPath = CONFIG::RENDERING::SHADERS_DIR + std::string(filepath);

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

    static unsigned int CompileShader(const std::string& source, GLenum shaderType)
    {
        const char* shaderCode = source.c_str();
        unsigned int shader = glCreateShader(shaderType);
        glShaderSource(shader, 1, &shaderCode, nullptr);
        glCompileShader(shader);

        int success;
        char infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            const char* shaderTypeName = (shaderType == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
            std::cerr << "ERROR::SHADER::" << shaderTypeName << "::COMPILATION_FAILED\n" << infoLog << std::endl;
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }
};