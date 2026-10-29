#pragma once

#include <glm/glm.hpp>
#include <stb_image.h>

#include "../../Config.h"
#include "../../core/rendering/texture/Texture.h"

#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class TextureMap
{
public:
    glm::vec2 size{ 0.0f, 0.0f };
    glm::vec2 offset{ 0.0f, 0.0f };
    int limit = -1;

    TextureMap() = default;

    // Constructor with optional name at the end (or omitted)
    TextureMap(
        const std::string& path,
        glm::vec2 size,
        glm::vec2 offset = glm::vec2(0.0f),
        int limit = -1,
        const std::string& name = ""
    )
        : size(size),
          offset(offset),
          limit(limit),
          name(name.empty() ? path : name)
    {
        LoadAndSlice(path);
    }

    // Constructor matching Texture's (path, name, ...) ordering
    TextureMap(
        const std::string& path,
        const std::string& name,
        glm::vec2 size,
        glm::vec2 offset = glm::vec2(0.0f),
        int limit = -1
    )
        : size(size),
          offset(offset),
          limit(limit),
          name(name.empty() ? path : name)
    {
        LoadAndSlice(path);
    }

    // =========================================================================
    // Static Storage & TextureMap Management
    // =========================================================================

    static TextureMap* Create(
        const std::string& path,
        glm::vec2 size,
        glm::vec2 offset = glm::vec2(0.0f),
        int limit = -1,
        const std::string& name = ""
    )
    {
        const std::string baseName = name.empty() ? path : name;
        if (textureMaps.find(baseName) != textureMaps.end())
        {
            std::cerr << "ERROR::TEXTURE_MAP::ALREADY_EXISTS: " << baseName << std::endl;
            return &textureMaps[baseName];
        }

        TextureMap map(path, size, offset, limit, baseName);
        if (!map.valid)
        {
            std::cerr << "ERROR::TEXTURE_MAP::CREATION_FAILED: " << baseName << std::endl;
            return nullptr;
        }

        auto [it, inserted] = textureMaps.insert_or_assign(baseName, std::move(map));
        return &it->second;
    }

    static TextureMap* Create(
        const std::string& path,
        const std::string& name,
        glm::vec2 size,
        glm::vec2 offset = glm::vec2(0.0f),
        int limit = -1
    )
    {
        return Create(path, size, offset, limit, name);
    }

    static TextureMap* Get(const std::string& name)
    {
        auto it = textureMaps.find(name);
        if (it == textureMaps.end())
        {
            std::cerr << "ERROR::TEXTURE_MAP::NOT_FOUND: " << name << std::endl;
            return nullptr;
        }
        return &it->second;
    }

    static bool Has(const std::string& name)
    {
        return textureMaps.find(name) != textureMaps.end();
    }

    // =========================================================================
    // Instance Methods
    // =========================================================================

    const std::string& GetName() const
    {
        return name;
    }

    size_t GetCount() const
    {
        return textureNames.size();
    }

    const std::vector<std::string>& GetTextureNames() const
    {
        return textureNames;
    }

    std::string GetTextureName(size_t index) const
    {
        if (index >= textureNames.size())
            return "";
        return textureNames[index];
    }

    Texture* GetTexture(size_t index) const
    {
        if (index >= textureNames.size())
            return nullptr;
        return Texture::Get(textureNames[index]);
    }

private:
    inline static std::unordered_map<std::string, TextureMap> textureMaps;

    std::string name;
    std::vector<std::string> textureNames;
    bool valid = false;

    void LoadAndSlice(const std::string& path)
    {
        const int tileWidth = static_cast<int>(size.x);
        const int tileHeight = static_cast<int>(size.y);

        if (size.x <= 0.0f || size.y <= 0.0f || tileWidth <= 0 || tileHeight <= 0)
        {
            std::cerr << "ERROR::TEXTURE_MAP::INVALID_SIZE: " << name
                      << " (size must be positive)" << std::endl;
            return;
        }

        if (offset.x < 0.0f || offset.y < 0.0f)
        {
            std::cerr << "ERROR::TEXTURE_MAP::NEGATIVE_OFFSET: " << name
                      << " (offset cannot be negative)" << std::endl;
            return;
        }

        if (limit < -1)
        {
            std::cerr << "ERROR::TEXTURE_MAP::INVALID_LIMIT: " << name
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

                const std::string subTextureName = name + "_" + std::to_string(createdCount);
                Texture* tex = Texture::CreateFromPixels(
                    tileBuffer.data(),
                    tileWidth,
                    tileHeight,
                    nrChannels,
                    subTextureName
                );

                if (tex)
                {
                    textureNames.push_back(subTextureName);
                }

                ++createdCount;
            }

            if (limit != -1 && createdCount >= limit)
                break;
        }

        stbi_image_free(sheetData);
        valid = !textureNames.empty();
    }
};

