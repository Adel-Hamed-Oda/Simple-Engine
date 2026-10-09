#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../../statics/FallBacks.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// Owns an OpenGL shader program. Move-only (the destructor deletes the program).
//
// Failure convention:
//   Create / CreateFromFiles / Get never return nullptr. On failure or a missing name they log an
//   error and return the fallback shader. The fallback itself may be invalid (ID == 0) if even it
//   failed to compile; use() and the uniform setters are safe no-ops on an invalid shader.
//
// Lifetime:
//   Shaders live in a static map. Call Shader::Clear() BEFORE destroying the GL context/window,
//   otherwise the static destructors run after the context is gone and glDeleteProgram is UB.
class Shader
{
public:
    GLuint ID = 0;

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

        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);

        GLint success = 0;
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success)
        {
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << GetProgramLog(ID) << std::endl;
            glDeleteProgram(ID);
            ID = 0;
        }

        // Shader objects are no longer needed once linking has been attempted.
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    ~Shader()
    {
        if (ID != 0)
            glDeleteProgram(ID);
    }

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept
        : ID(std::exchange(other.ID, 0)),
          name(std::move(other.name)),
          uniformLocations(std::move(other.uniformLocations))
    {
    }

    Shader& operator=(Shader&& other) noexcept
    {
        if (this != &other)
        {
            if (ID != 0)
                glDeleteProgram(ID);

            ID = std::exchange(other.ID, 0);
            name = std::move(other.name);
            uniformLocations = std::move(other.uniformLocations);
        }
        return *this;
    }

    // =========================================================================
    // Static Storage & Shader Management
    // =========================================================================

    // Creates a shader from a base path (e.g. "default" -> "default.vert" & "default.frag")
    // and stores it in the lookup map under `name` (defaults to `path`).
    static Shader* Create(const std::string& path, const std::string& name = "")
    {
        return CreateFromFiles(path + ".vert", path + ".frag", name.empty() ? path : name);
    }

    // Creates a shader from explicit vertex and fragment paths and stores it in the lookup map.
    // (Named differently from Create so that Create("a.vert", "a.frag") can't silently bind to
    //  the (path, name) overload.)
    static Shader* CreateFromFiles(const std::string& vertexPath, const std::string& fragmentPath, const std::string& name)
    {
        auto existing = shaders.find(name);
        if (existing != shaders.end())
        {
            std::cerr << "ERROR::SHADER::ALREADY_EXISTS: " << name << std::endl;
            return existing->second.get();
        }

        const std::string vertexCode = ReadFile(vertexPath);
        const std::string fragmentCode = ReadFile(fragmentPath);

        auto shader = std::make_unique<Shader>(vertexCode, fragmentCode);
        if (shader->ID == 0)
        {
            std::cerr << "ERROR::SHADER::CREATION_FAILED: " << name << std::endl;
            return GetFallback();
        }

        shader->name = name;
        auto [it, inserted] = shaders.emplace(name, std::move(shader));
        return it->second.get();
    }

    // Looks up a shader by name. Logs an error and returns the fallback shader if missing.
    static Shader* Get(const std::string& name)
    {
        auto it = shaders.find(name);
        if (it == shaders.end())
        {
            std::cerr << "ERROR::SHADER::NOT_FOUND: " << name << std::endl;
            return GetFallback();
        }
        return it->second.get();
    }

    // Returns the fallback shader, creating it on first use. Never nullptr.
    static Shader* GetFallback()
    {
        if (!fallbackShader)
            CreateFallbackShader();
        return fallbackShader.get();
    }

    // Deletes every stored shader (and the fallback). Call while the GL context is still alive.
    static void Clear()
    {
        shaders.clear();
        fallbackShader.reset();
    }

    // =========================================================================
    // Shader Instance Methods
    // =========================================================================

    const std::string& GetName() const
    {
        return name;
    }

    bool IsValid() const
    {
        return ID != 0;
    }

    void use() const
    {
        if (ID != 0)
            glUseProgram(ID);
    }

    // NOTE: the setters below use glUniform*, which affects the currently bound program.
    // Call use() first. (On GL 4.1+ you could switch to glProgramUniform* to drop that requirement.)

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
    inline static std::unordered_map<std::string, std::unique_ptr<Shader>> shaders;
    inline static std::unique_ptr<Shader> fallbackShader;

    std::string name;
    std::unordered_map<std::string, GLint> uniformLocations;

    GLint GetUniformLocation(const std::string& uniformName)
    {
        if (ID == 0)
            return -1;

        auto it = uniformLocations.find(uniformName);
        if (it != uniformLocations.end())
            return it->second;

        GLint location = glGetUniformLocation(ID, uniformName.c_str());
        uniformLocations.emplace(uniformName, location);
        return location;
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

    static void CreateFallbackShader()
    {
        fallbackShader = std::make_unique<Shader>(FallBacks::VertexShaderCode, FallBacks::FragmentShaderCode);
        fallbackShader->name = "__fallback__";

        if (fallbackShader->ID == 0)
            std::cerr << "ERROR::SHADER::FALLBACK_SHADER_CREATION_FAILED" << std::endl;
    }
};