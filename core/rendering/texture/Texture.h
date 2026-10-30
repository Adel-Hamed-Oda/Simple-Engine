#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include "../../../Config.h"

#include <iostream>
#include <string>
#include <utility>

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

    explicit Texture(
        const std::string& texturePath,
        TextureWrapMode wrap = TextureWrapMode::REPEAT,
        TextureFilterMode filter = TextureFilterMode::LINEAR
    )
        : wrapMode(wrap),
          filterMode(filter)
    {
        const std::string fullPath = CONFIG::DIRECTORY::TEXTURES + texturePath;

        stbi_set_flip_vertically_on_load(true);
        int width = 0, height = 0, nrChannels = 0;
        unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &nrChannels, 0);

        if (!data)
        {
            std::cerr << "ERROR::TEXTURE::FILE_NOT_FOUND_OR_READ_FAILED: " << texturePath << std::endl;
            stbi_set_flip_vertically_on_load(false);
            return;
        }

        UploadToGPU(data, width, height, nrChannels);

        stbi_image_free(data);
        stbi_set_flip_vertically_on_load(false);
    }

    Texture(
        const unsigned char* data,
        int width,
        int height,
        int nrChannels,
        TextureWrapMode wrap = TextureWrapMode::REPEAT,
        TextureFilterMode filter = TextureFilterMode::LINEAR
    )
        : wrapMode(wrap),
          filterMode(filter)
    {
        if (!data || width <= 0 || height <= 0)
            return;

        UploadToGPU(data, width, height, nrChannels);
    }

    ~Texture()
    {
        if (ID != 0)
            glDeleteTextures(1, &ID);
    }

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept
        : ID(std::exchange(other.ID, 0)),
          wrapMode(other.wrapMode),
          filterMode(other.filterMode)
    {
    }

    Texture& operator=(Texture&& other) noexcept
    {
        if (this != &other)
        {
            if (ID != 0)
                glDeleteTextures(1, &ID);
            ID = std::exchange(other.ID, 0);
            wrapMode = other.wrapMode;
            filterMode = other.filterMode;
        }
        return *this;
    }

    // =========================================================================
    // Factory Methods (return caller-owned Texture*)
    // =========================================================================

    static Texture* Create(
        const std::string& path,
        TextureWrapMode wrap = TextureWrapMode::REPEAT,
        TextureFilterMode filter = TextureFilterMode::LINEAR
    )
    {
        Texture* texture = new Texture(path, wrap, filter);
        if (!texture->IsValid())
        {
            std::cerr << "ERROR::TEXTURE::CREATION_FAILED: " << path << std::endl;
        }
        return texture;
    }

    static Texture* CreateFromPixels(
        const unsigned char* data,
        int width,
        int height,
        int nrChannels,
        TextureWrapMode wrap = TextureWrapMode::REPEAT,
        TextureFilterMode filter = TextureFilterMode::LINEAR
    )
    {
        Texture* texture = new Texture(data, width, height, nrChannels, wrap, filter);
        if (!texture->IsValid())
        {
            std::cerr << "ERROR::TEXTURE::CREATION_FROM_PIXELS_FAILED" << std::endl;
        }
        return texture;
    }

    // =========================================================================
    // Instance Methods
    // =========================================================================

    bool IsValid() const
    {
        return ID != 0;
    }

private:
    void UploadToGPU(const unsigned char* data, int width, int height, int nrChannels)
    {
        unsigned int texID = 0;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);

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

        ID = texID;
    }

    static GLenum GetFormatFromChannels(int nrChannels)
    {
        switch (nrChannels)
        {
            case 1: return GL_RED;
            case 2: return GL_RG;
            case 3: return GL_RGB;
            case 4: return GL_RGBA;
            default:
                std::cerr << "ERROR::TEXTURE::UNSUPPORTED_CHANNEL_COUNT: "
                          << nrChannels << std::endl;
                return GL_RGB;
        }
    }
};
