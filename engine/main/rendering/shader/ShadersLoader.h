#pragma once
#include "Shader.h"
#include <iostream>
#include <map>
#include <string>
#include <filesystem>

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
            std::cerr << "ERROR::SHADER::NOT_FOUND: " << shaderName << std::endl;
            return nullptr;
        }

        return &it->second;
    }

private:
    inline static std::map<std::string, Shader> shaders;
    inline static bool initialized = false;

    static bool InsertShader(const std::string& shaderName, const std::string& filepath)
    {
        Shader shader(filepath.c_str());

        if (shader.ID == 0)
        {
            std::cerr << "Failed to load shader: " << shaderName << " at path " << filepath << std::endl;
            return false;
        }

        shaders.emplace(shaderName, std::move(shader));
        return true;
    }

    static bool SetupShaders()
    {
        const std::string shadersDir = CONFIG::RENDERING::SHADERS_DIR;

        if (!fs::exists(shadersDir) || !fs::is_directory(shadersDir))
        {
            std::cerr << "Shaders directory not found: " << shadersDir << std::endl;
            return false;
        }

        // Recursively iterate through all files and subdirectories
        for (const auto& entry : fs::recursive_directory_iterator(shadersDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".glsl")
            {
                // Option A: Use filename without extension as key (e.g., "Default" for assets/Shaders/UI/Default.glsl)
                std::string shaderName = entry.path().stem().string();

                // Option B (Alternative): Use relative subpath without extension if you want to prevent name collisions across folders
                // e.g., "UI/Default" instead of "Default"
                // std::string shaderName = fs::relative(entry.path(), shadersDir).replace_extension("").generic_string();

                std::string fullPath = entry.path().string();

                if (!InsertShader(shaderName, fullPath))
                {
                    std::cerr << "Warning: Failed to auto-register shader " << shaderName << std::endl;
                }
            }
        }

        return !shaders.empty();
    }
};