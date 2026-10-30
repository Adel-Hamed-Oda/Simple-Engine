#pragma once

#include <glm/glm.hpp>
#include <stb_image.h>

#include "../../Config.h"
#include "../../core/rendering/texture/Texture.h"

#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

class TextureMap
{
public:
    glm::vec2 size{ 0.0f, 0.0f };
    glm::vec2 offset{ 0.0f, 0.0f };
    int limit = -1;

    TextureMap() = default;

    TextureMap(
        const std::string& path,
        glm::vec2 size,
        glm::vec2 offset = glm::vec2(0.0f),
        int limit = -1
    )
        : size(size),
          offset(offset),
          limit(limit)
    {
        LoadAndSlice(path);
    }

    ~TextureMap()
    {
        for (Texture* tex : textures)
        {
            delete tex;
        }
        textures.clear();
    }

    TextureMap(const TextureMap&) = delete;
    TextureMap& operator=(const TextureMap&) = delete;

    TextureMap(TextureMap&& other) noexcept
        : size(other.size),
          offset(other.offset),
          limit(other.limit),
          textures(std::move(other.textures)),
          valid(std::exchange(other.valid, false))
    {
        other.textures.clear();
    }

    TextureMap& operator=(TextureMap&& other) noexcept
    {
        if (this != &other)
        {
            for (Texture* tex : textures)
            {
                delete tex;
            }
            size = other.size;
            offset = other.offset;
            limit = other.limit;
            textures = std::move(other.textures);
            other.textures.clear();
            valid = std::exchange(other.valid, false);
        }
        return *this;
    }

    // =========================================================================
    // Factory Method (returns caller-owned TextureMap*)
    // =========================================================================

    static TextureMap* Create(
        const std::string& path,
        glm::vec2 size,
        glm::vec2 offset = glm::vec2(0.0f),
        int limit = -1
    )
    {
        TextureMap* map = new TextureMap(path, size, offset, limit);
        if (!map->valid)
        {
            std::cerr << "ERROR::TEXTURE_MAP::CREATION_FAILED: " << path << std::endl;
        }
        return map;
    }

    // =========================================================================
    // Instance Methods
    // =========================================================================

    bool IsValid() const
    {
        return valid;
    }

    size_t GetCount() const
    {
        return textures.size();
    }

    const std::vector<Texture*>& GetTextures() const
    {
        return textures;
    }

    Texture* GetTexture(size_t index) const
    {
        if (index >= textures.size())
        {
            std::cerr << "ERROR::TEXTURE_MAP::INDEX_OUT_OF_RANGE: " << index << std::endl;
            return nullptr;
        }
        return textures[index];
    }

    Texture* operator[](size_t index) const
    {
        return GetTexture(index);
    }

private:
    std::vector<Texture*> textures;
    bool valid = false;

    void LoadAndSlice(const std::string& path)
    {
        const int tileWidth = static_cast<int>(size.x);
        const int tileHeight = static_cast<int>(size.y);

        if (size.x <= 0.0f || size.y <= 0.0f || tileWidth <= 0 || tileHeight <= 0)
        {
            std::cerr << "ERROR::TEXTURE_MAP::INVALID_SIZE: " << path
                      << " (size must be positive)" << std::endl;
            return;
        }

        if (offset.x < 0.0f || offset.y < 0.0f)
        {
            std::cerr << "ERROR::TEXTURE_MAP::NEGATIVE_OFFSET: " << path
                      << " (offset cannot be negative)" << std::endl;
            return;
        }

        if (limit < -1)
        {
            std::cerr << "ERROR::TEXTURE_MAP::INVALID_LIMIT: " << path
                      << " (limit must be -1 or non-negative)" << std::endl;
            return;
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
            std::cerr << "ERROR::TEXTURE_MAP::FILE_NOT_FOUND_OR_READ_FAILED: " << path << std::endl;
            return;
        }

        if (tileWidth > sheetWidth || tileHeight > sheetHeight)
        {
            std::cerr << "ERROR::TEXTURE_MAP::SIZE_EXCEEDS_IMAGE_DIMENSIONS: " << path << std::endl;
            stbi_image_free(sheetData);
            return;
        }

        if (limit == 0)
        {
            stbi_image_free(sheetData);
            valid = true;
            return;
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

                Texture* tex = Texture::CreateFromPixels(
                    tileBuffer.data(),
                    tileWidth,
                    tileHeight,
                    nrChannels
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
        valid = !textures.empty();
    }
};
