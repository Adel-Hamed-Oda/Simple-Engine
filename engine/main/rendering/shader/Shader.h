#pragma once

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

class Shader
{
public:
    unsigned int ID = 0;

    Shader() = default;
    Shader(const char* shaderPath)
    {
        std::string shaderCode = ReadFile(shaderPath);

        if (shaderCode.empty())
            return;

        std::string vertexCode;
        std::string fragmentCode;

        if (!ParseShaderFile(shaderCode, vertexCode, fragmentCode))
        {
            std::cerr
                << "ERROR::SHADER::INVALID_SHADER_FILE: "
                << shaderPath
                << std::endl;

            return;
        }

        // Compile vertex shader
        unsigned int vertex = CompileShader(
            vertexCode,
            GL_VERTEX_SHADER
        );

        if (vertex == 0)
            return;

        // Compile fragment shader
        unsigned int fragment = CompileShader(
            fragmentCode,
            GL_FRAGMENT_SHADER
        );

        if (fragment == 0)
        {
            glDeleteShader(vertex);
            return;
        }

        // Create shader program
        ID = glCreateProgram();

        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);

        glLinkProgram(ID);

        int success;
        char infoLog[512];

        glGetProgramiv(ID, GL_LINK_STATUS, &success);

        if (!success)
        {
            glGetProgramInfoLog(
                ID,
                512,
                nullptr,
                infoLog
            );

            std::cerr
                << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                << infoLog
                << std::endl;

            glDeleteProgram(ID);
            ID = 0;
        }

		name = shaderPath;

        // Shaders are now linked into the program.
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }
	const std::string& GetName() const
	{
		return name;
	}

    void use() const
    {
        if (ID != 0)
            glUseProgram(ID);
    }

    void setBool(const std::string& name, bool value)
    {
        GLint location = GetUniformLocation(name);

        if (location == -1)
            return;

        glUniform1i(
            location,
            static_cast<int>(value)
        );
    }

    void setInt(const std::string& name, int value)
    {
        GLint location = GetUniformLocation(name);

        if (location == -1)
            return;

        glUniform1i(location, value);
    }

    void setFloat(const std::string& name, float value)
    {
        GLint location = GetUniformLocation(name);

        if (location == -1)
            return;

        glUniform1f(location, value);
    }

    void setVec2(
        const std::string& name,
        const glm::vec2& value
    )
    {
        GLint location = GetUniformLocation(name);

        if (location == -1)
            return;

        glUniform2fv(
            location,
            1,
            glm::value_ptr(value)
        );
    }

    void setVec3(
        const std::string& name,
        const glm::vec3& value
    )
    {
        GLint location = GetUniformLocation(name);

        if (location == -1)
            return;

        glUniform3fv(
            location,
            1,
            glm::value_ptr(value)
        );
    }

    void setVec4(
        const std::string& name,
        const glm::vec4& value
    )
    {
        GLint location = GetUniformLocation(name);

        if (location == -1)
            return;

        glUniform4fv(
            location,
            1,
            glm::value_ptr(value)
        );
    }

    void setMat4(
        const std::string& name,
        const glm::mat4& value
    )
    {
        GLint location = GetUniformLocation(name);

        if (location == -1)
            return;

        glUniformMatrix4fv(
            location,
            1,
            GL_FALSE,
            glm::value_ptr(value)
        );
    }

private:
    std::string name;
    std::unordered_map<std::string, GLint> uniformLocations;

    GLint GetUniformLocation(const std::string& name)
    {
        auto it = uniformLocations.find(name);

        if (it != uniformLocations.end())
            return it->second;

        GLint location = glGetUniformLocation(
            ID,
            name.c_str()
        );

        uniformLocations[name] = location;

        return location;
    }

    static std::string ReadFile(const char* filepath)
    {
        std::ifstream file(
            filepath,
            std::ios::in | std::ios::binary
        );

        if (!file.is_open())
        {
            std::cerr
                << "ERROR::SHADER::FILE_NOT_FOUND_OR_READ_FAILED: "
                << filepath
                << std::endl;

            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        return buffer.str();
    }

    static bool ParseShaderFile(
        const std::string& source,
        std::string& vertexCode,
        std::string& fragmentCode
    )
    {
        const std::string vertexMarker = "#type vertex";
        const std::string fragmentMarker = "#type fragment";

        size_t vertexPosition = source.find(vertexMarker);
        size_t fragmentPosition = source.find(fragmentMarker);

        // Both sections must exist.
        if (vertexPosition == std::string::npos ||
            fragmentPosition == std::string::npos)
        {
            return false;
        }

        // Vertex section must come before fragment section.
        if (vertexPosition > fragmentPosition)
        {
            return false;
        }

        // Start after "#type vertex"
        size_t vertexStart =
            vertexPosition + vertexMarker.length();

        // Everything between the markers is the vertex shader.
        vertexCode = source.substr(
            vertexStart,
            fragmentPosition - vertexStart
        );

        // Everything after "#type fragment" is the fragment shader.
        size_t fragmentStart =
            fragmentPosition + fragmentMarker.length();

        fragmentCode = source.substr(fragmentStart);

        return !vertexCode.empty() &&
            !fragmentCode.empty();
    }

    static unsigned int CompileShader(
        const std::string& source,
        GLenum shaderType
    )
    {
        const char* shaderCode = source.c_str();

        unsigned int shader = glCreateShader(shaderType);

        glShaderSource(
            shader,
            1,
            &shaderCode,
            nullptr
        );

        glCompileShader(shader);

        int success;
        char infoLog[512];

        glGetShaderiv(
            shader,
            GL_COMPILE_STATUS,
            &success
        );

        if (!success)
        {
            glGetShaderInfoLog(
                shader,
                512,
                nullptr,
                infoLog
            );

            const char* shaderTypeName =
                shaderType == GL_VERTEX_SHADER
                ? "VERTEX"
                : "FRAGMENT";

            std::cerr
                << "ERROR::SHADER::"
                << shaderTypeName
                << "::COMPILATION_FAILED\n"
                << infoLog
                << std::endl;

            glDeleteShader(shader);

            return 0;
        }

        return shader;
    }
};