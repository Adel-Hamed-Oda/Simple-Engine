#pragma once

#include <stb_image.h>
#include <glm/glm.hpp>
#include <string>

// Utility functions for inspecting and querying image file metadata.
class TextureUtils
{
public:
    // Queries the width and height of an image file without decoding all pixel data.
    // Returns true if the image header was read successfully, populating outWidth and outHeight.
    static glm::vec2 GetImageDimensions(const std::string& imagePath)
    {
        int channels = 0;
        int width = 0, height = 0;
        if (stbi_info(imagePath.c_str(), &width, &height, &channels))
        {
            return glm::vec2(static_cast<float>(width), static_cast<float>(height));
        }
        return glm::vec2(0.0f);
    }
};
