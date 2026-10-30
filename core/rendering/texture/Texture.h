#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include "../../../Config.h"

#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

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
    // Factory Methods (return caller-owned Texture* / std::vector<Texture*>)
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

    // Slices a texture sheet / atlas into individual Texture* instances left-to-right, top-to-bottom.
    // - size: pixel dimensions (width, height) of each tile (must be positive).
    // - offset: pixel padding/offset between tiles (cannot be negative).
    // - limit: -1 for no limit, otherwise caps the maximum number of textures created.
    static std::vector<Texture*> CreateTextureMap(
        const std::string& path,
        glm::vec2 size,
        glm::vec2 offset = glm::vec2(0.0f),
        int limit = -1,
        TextureWrapMode wrap = TextureWrapMode::REPEAT,
        TextureFilterMode filter = TextureFilterMode::LINEAR
    )
    {
        std::vector<Texture*> textures;

        const int tileWidth = static_cast<int>(size.x);
        const int tileHeight = static_cast<int>(size.y);

        if (size.x <= 0.0f || size.y <= 0.0f || tileWidth <= 0 || tileHeight <= 0)
        {
            std::cerr << "ERROR::TEXTURE::TEXTURE_MAP::INVALID_SIZE: " << path
                      << " (size must be positive)" << std::endl;
            return textures;
        }

        if (offset.x < 0.0f || offset.y < 0.0f)
        {
            std::cerr << "ERROR::TEXTURE::TEXTURE_MAP::NEGATIVE_OFFSET: " << path
                      << " (offset cannot be negative)" << std::endl;
            return textures;
        }

        if (limit < -1)
        {
            std::cerr << "ERROR::TEXTURE::TEXTURE_MAP::INVALID_LIMIT: " << path
                      << " (limit must be -1 or non-negative)" << std::endl;
            return textures;
        }

        const int offsetX = static_cast<int>(offset.x);
        const int offsetY = static_cast<int>(offset.y);

        const std::string fullPath = CONFIG::DIRECTORY::TEXTURES + path;

        // Load unflipped so tile 0 starts at top-left (x = 0, y = 0) and advances
        // left-to-right, top-to-bottom; each individual tile is flipped vertically
        // before uploading to OpenGL so its UV orientation matches Texture::Create().
        stbi_set_flip_vertically_on_load(false);
        int sheetWidth = 0, sheetHeight = 0, nrChannels = 0;
        unsigned char* sheetData = stbi_load(fullPath.c_str(), &sheetWidth, &sheetHeight, &nrChannels, 0);

        if (!sheetData)
        {
            std::cerr << "ERROR::TEXTURE::TEXTURE_MAP::FILE_NOT_FOUND_OR_READ_FAILED: " << path << std::endl;
            return textures;
        }

        if (tileWidth > sheetWidth || tileHeight > sheetHeight)
        {
            std::cerr << "ERROR::TEXTURE::TEXTURE_MAP::SIZE_EXCEEDS_IMAGE_DIMENSIONS: " << path << std::endl;
            stbi_image_free(sheetData);
            return textures;
        }

        if (limit == 0)
        {
            stbi_image_free(sheetData);
            return textures;
        }

        const size_t rowBytes = static_cast<size_t>(tileWidth) * static_cast<size_t>(nrChannels);
        std::vector<unsigned char> tileBuffer(rowBytes * static_cast<size_t>(tileHeight));

        int createdCount = 0;

        for (int y = 0; y + tileHeight <= sheetHeight; y += tileHeight + offsetY)
        {
            for (int x = 0; x + tileWidth <= sheetWidth; x += tileWidth + offsetX)
            {
                if (limit != -1 && createdCount >= limit)
                    break;

                for (int row = 0; row < tileHeight; ++row)
                {
                    const int srcY = y + (tileHeight - 1 - row);
                    const size_t srcOffset =
                        (static_cast<size_t>(srcY) * static_cast<size_t>(sheetWidth) + static_cast<size_t>(x)) *
                        static_cast<size_t>(nrChannels);
                    const size_t dstOffset = static_cast<size_t>(row) * rowBytes;

                    std::memcpy(tileBuffer.data() + dstOffset, sheetData + srcOffset, rowBytes);
                }

                Texture* tex = CreateFromPixels(
                    tileBuffer.data(),
                    tileWidth,
                    tileHeight,
                    nrChannels,
                    wrap,
                    filter
                );

                if (tex && tex->IsValid())
                {
                    textures.push_back(tex);
                }
                else
                {
                    delete tex;
                }

                ++createdCount;
            }

            if (limit != -1 && createdCount >= limit)
                break;
        }

        stbi_image_free(sheetData);
        return textures;
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
