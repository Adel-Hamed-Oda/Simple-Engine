#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include "Config.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

enum class TextureWrapMode
{
    REPEAT = GL_REPEAT,
    MIRRORED_REPEAT = GL_MIRRORED_REPEAT,
    CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,
    CLAMP_TO_BORDER = GL_CLAMP_TO_BORDER
};

enum class TextureFilterMode
{
    NEAREST = GL_NEAREST,
    LINEAR = GL_LINEAR,
    NEAREST_MIPMAP_NEAREST = GL_NEAREST_MIPMAP_NEAREST,
    LINEAR_MIPMAP_NEAREST = GL_LINEAR_MIPMAP_NEAREST,
    NEAREST_MIPMAP_LINEAR = GL_NEAREST_MIPMAP_LINEAR,
    LINEAR_MIPMAP_LINEAR = GL_LINEAR_MIPMAP_LINEAR
};

class Texture
{
public:
    unsigned int ID = 0;

    TextureWrapMode wrapMode = TextureWrapMode::REPEAT;
    TextureFilterMode filterMode = TextureFilterMode::LINEAR;

    Texture() = default;
    Texture(const std::string& shaderPath)
    {
        std::string fullPath = CONFIG::RENDERING::TEXTURES_DIR + shaderPath;

        stbi_set_flip_vertically_on_load(true); // Flip the image vertically on load
        int width, height, nrChannels;
        unsigned char *data = stbi_load(fullPath.c_str(), &width, &height, &nrChannels, 0);

        if (!data)
        {
            std::cerr << "ERROR::TEXTURE::FILE_NOT_FOUND_OR_READ_FAILED: " << shaderPath << std::endl;
            stbi_set_flip_vertically_on_load(false);
            return;
        }

        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_2D, ID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLenum>(wrapMode));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLenum>(wrapMode));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLenum>(filterMode));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLenum>(filterMode));

        const GLenum format = GetFormatFromChannels(nrChannels);
        GLint previousUnpackAlignment = 4;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
        stbi_set_flip_vertically_on_load(false);
    }

    // =========================================================================
    // Static Storage & Texture Management
    // =========================================================================

    static Texture* Create(const std::string& path, const std::string& name = "")
    {
        std::string name_t = name.empty() ? path : name;
        if (textures.find(name_t) != textures.end())
        {
            std::cerr << "ERROR::TEXTURE::ALREADY_EXISTS: " << name_t << std::endl;
            return &textures[name_t];
        }

        Texture texture(path);
        if (texture.ID == 0)
        {
            std::cerr << "ERROR::TEXTURE::CREATION_FAILED: " << name_t << std::endl;
            return nullptr;
        }

        auto [it, inserted] = textures.insert_or_assign(name_t, std::move(texture));
        return &it->second;
    }

    // Look up texture in map. Returns nullptr and logs error if missing.
    static Texture* Get(const std::string& name)
    {
        auto it = textures.find(name);
        if (it == textures.end())
        {
            std::cerr << "ERROR::TEXTURE::NOT_FOUND: " << name << std::endl;
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

private:
    inline static std::unordered_map<std::string, Texture> textures;

    std::string name;

    GLenum GetFormatFromChannels(int nrChannels)
    {
        switch (nrChannels)
        {
            case 1: return GL_RED;
            case 2: return GL_RG;
            case 3: return GL_RGB;
            case 4: return GL_RGBA;
            default:
                std::cerr << "ERROR::TEXTURE::" + name + "::UNSUPPORTED_CHANNEL_COUNT: "
                          << nrChannels << std::endl;
                return GL_RGB; // Default to RGB if unsupported
        }
    }
    
};