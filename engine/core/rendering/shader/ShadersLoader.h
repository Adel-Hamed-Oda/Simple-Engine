#pragma once

#include "Shader.h"

#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

class ShadersLoader
{
public:
    static Shader* GetShader(const std::string& shaderName)
    {
        if (!initialized)
        {
            if (!SetupShaders())
            {
                std::cerr << "Shader setup failed." << std::endl;
                return nullptr;
            }

            initialized = true;
        }

        auto it = shaders.find(shaderName);

        if (it == shaders.end())
        {
            std::cerr
                << "ERROR::SHADER::NOT_FOUND: "
                << shaderName
                << std::endl;

            return nullptr;
        }

        return &it->second;
    }

private:
    inline static std::map<std::string, Shader> shaders;
    inline static bool initialized = false;

    static bool InsertShader(
        const std::string& shaderName,
        const fs::path& vertexPath,
        const fs::path& fragmentPath
    )
    {
        Shader shader(
            vertexPath.string().c_str(),
            fragmentPath.string().c_str(),
            shaderName
        );

        if (shader.ID == 0)
        {
            std::cerr
                << "Failed to load shader: "
                << shaderName
                << std::endl;

            return false;
        }

        shaders.emplace(
            shaderName,
            std::move(shader)
        );

        return true;
    }

    static bool SetupShaders()
    {
        const fs::path shadersDir =
            CONFIG::RENDERING::SHADERS_DIR;

        if (!fs::exists(shadersDir) ||
            !fs::is_directory(shadersDir))
        {
            std::cerr
                << "Shaders directory not found: "
                << shadersDir
                << std::endl;

            return false;
        }

        std::map<std::string, fs::path> vertexFiles;
        std::map<std::string, fs::path> fragmentFiles;

        for (const auto& entry :
             fs::recursive_directory_iterator(shadersDir))
        {
            if (!entry.is_regular_file())
                continue;

            const fs::path path = entry.path();
            const std::string extension =
                path.extension().string();

            std::string shaderName =
                fs::relative(path, shadersDir)
                    .replace_extension("")
                    .generic_string();

            if (extension == ".vert")
            {
                vertexFiles[shaderName] = path;
            }
            else if (extension == ".frag")
            {
                fragmentFiles[shaderName] = path;
            }
        }

        std::set<std::string> shaderNames;

        for (const auto& [name, path] : vertexFiles)
            shaderNames.insert(name);
        for (const auto& [name, path] : fragmentFiles)
            shaderNames.insert(name);

        if (shaderNames.empty())
        {
            std::cerr
                << "No shader files found in: "
                << shadersDir
                << std::endl;

            return false;
        }

        for (const std::string& shaderName : shaderNames)
        {
            auto vertexIt = vertexFiles.find(shaderName);
            auto fragmentIt = fragmentFiles.find(shaderName);

            // Missing .vert
            if (vertexIt == vertexFiles.end())
            {
                throw std::runtime_error(
                    "ERROR::SHADER::MISSING_VERTEX_FILE: " +
                    shaderName +
                    ".vert"
                );
            }

            // Missing .frag
            if (fragmentIt == fragmentFiles.end())
            {
                throw std::runtime_error(
                    "ERROR::SHADER::MISSING_FRAGMENT_FILE: " +
                    shaderName +
                    ".frag"
                );
            }

            if (!InsertShader(
                    shaderName,
                    vertexIt->second,
                    fragmentIt->second))
            {
                std::cerr
                    << "Warning: Failed to auto-register shader "
                    << shaderName
                    << std::endl;
            }
        }

        return !shaders.empty();
    }
};