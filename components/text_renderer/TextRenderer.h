#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../Component.h"
#include "../../core/rendering/material/TextMaterial.h"
#include "../../core/transform/Transform.h"

// World-Space Text Renderer:
// The string is laid out in the object's local XY plane, facing +Z,
// with the baseline at y = 0 and the first glyph starting at x = 0.
// One em (the font's pixelSize) equals one world unit at Transform scale 1.
// Affected by camera view & projection and depth testing.
class TextRenderer : public Component
{
public:
    std::string text;
    std::string fontPath;
    unsigned int pixelSize = 48;
    unsigned int rasterPixelSize = 128;
    TextMaterial* material = nullptr;
    int zOrder = 0;

    TextRenderer(
        std::string text,
        std::string fontPath,
        unsigned int pixelSize,
        TextMaterial* material,
        int zOrder
    )
        : text(std::move(text)),
          fontPath(std::move(fontPath)),
          pixelSize(pixelSize),
          material(material),
          zOrder(zOrder)
    {
        Initialize();
    }

    TextRenderer(
        std::string text,
        std::string fontPath,
        unsigned int pixelSize,
        TextMaterial* material
    )
        : TextRenderer(std::move(text), std::move(fontPath), pixelSize, material, 0)
    {
    }

    TextRenderer(
        std::string text,
        unsigned int pixelSize,
        TextMaterial* material,
        int zOrder
    )
        : TextRenderer(std::move(text), "", pixelSize, material, zOrder)
    {
    }

    TextRenderer(
        std::string text,
        unsigned int pixelSize,
        TextMaterial* material
    )
        : TextRenderer(std::move(text), "", pixelSize, material, 0)
    {
    }

    ~TextRenderer() override
    {
        CleanupGlyphs();

        if (VAO != 0)
            glDeleteVertexArrays(1, &VAO);
        if (VBO != 0)
            glDeleteBuffers(1, &VBO);
    }

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    void SetText(const std::string& value) { text = value; }
    const std::string& GetText() const { return text; }

    const std::string& GetFontPath() const { return fontPath; }

    bool SetFont(const std::string& newFontPath, unsigned int newPixelSize = 0)
    {
        if (newPixelSize > 0)
            pixelSize = newPixelSize;

        fontPath = newFontPath;
        CleanupGlyphs();
        Initialize();
        return !glyphs.empty();
    }

    TextMaterial* GetMaterial() const { return material; }
    void SetMaterial(TextMaterial* value) { material = value; }

    int GetZOrder() const { return zOrder; }
    void SetZOrder(int value) { zOrder = value; }

    float CalculateLocalWidth(const std::string& str) const
    {
        float w = 0.0f;
        const float unit = 1.0f / static_cast<float>(rasterPixelSize);
        for (char c : str)
        {
            auto it = glyphs.find(c);
            if (it != glyphs.end())
                w += static_cast<float>(it->second.advance >> 6) * unit;
        }
        return w;
    }

    virtual void Render() const
    {
        if (VAO == 0 || glyphs.empty() || !transform || !material || text.empty())
            return;

        // World-space text uses the main camera's view/projection.
        if (!material->SetupCoreUniforms(transform))
            return;

        material->ApplyMaterial();

        const float unit = 1.0f / static_cast<float>(rasterPixelSize);   // glyph pixels -> local units

        const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
        const GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean depthMask = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        if (material->doubleSided)
            glDisable(GL_CULL_FACE);

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(VAO);

        float x = 0.0f;

        for (const char character : text)
        {
            const auto glyph = glyphs.find(character);
            if (glyph == glyphs.end())
                continue;

            const Character& g = glyph->second;
            const float xpos = x + g.bearing.x * unit;
            const float ypos = (g.bearing.y - g.size.y) * unit;   // bottom edge (below baseline for descenders)
            const float w = g.size.x * unit;
            const float h = g.size.y * unit;

            // World space is y-up. The bitmap's first row is the TOP of the glyph,
            // so the top edge uses v = 0. Counter-clockwise when viewed from +Z.
            // Layout per vertex: { position.x, position.y, position.z, uv.x, uv.y }
            const float vertices[6][5] = {
                { xpos,     ypos,     0.0f, 0.0f, 1.0f },  // bottom-left
                { xpos + w, ypos,     0.0f, 1.0f, 1.0f },  // bottom-right
                { xpos + w, ypos + h, 0.0f, 1.0f, 0.0f },  // top-right

                { xpos,     ypos,     0.0f, 0.0f, 1.0f },  // bottom-left
                { xpos + w, ypos + h, 0.0f, 1.0f, 0.0f },  // top-right
                { xpos,     ypos + h, 0.0f, 0.0f, 0.0f }   // top-left
            };

            glBindTexture(GL_TEXTURE_2D, g.textureID);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            x += static_cast<float>(g.advance >> 6) * unit;
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glDepthMask(depthMask);
        if (depthEnabled) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
        if (cullEnabled) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
        if (!blendEnabled) glDisable(GL_BLEND);
    }

protected:
    struct Character
    {
        GLuint textureID = 0;
        glm::ivec2 size = glm::ivec2(0);
        glm::ivec2 bearing = glm::ivec2(0);
        long advance = 0;
    };

    std::map<char, Character> glyphs;
    GLuint VAO = 0;
    GLuint VBO = 0;

    void CleanupGlyphs()
    {
        for (const auto& [character, glyph] : glyphs)
        {
            if (glyph.textureID != 0)
                glDeleteTextures(1, &glyph.textureID);
        }
        glyphs.clear();
    }

    void Initialize()
    {
        rasterPixelSize = (pixelSize < 128) ? 128 : pixelSize;

        if (fontPath.empty())
        {
            std::cerr << "ERROR::TEXT_RENDERER::FONT_PATH_EMPTY: No font path specified." << std::endl;
            return;
        }

        FT_Library library = nullptr;
        if (FT_Init_FreeType(&library) != 0)
        {
            std::cerr << "ERROR::TEXT_RENDERER::FREETYPE_INIT_FAILED" << std::endl;
            return;
        }

        FT_Face face = nullptr;
        if (FT_New_Face(library, fontPath.c_str(), 0, &face) != 0)
        {
            std::cerr << "ERROR::TEXT_RENDERER::FONT_LOAD_FAILED: " << fontPath << std::endl;
            FT_Done_FreeType(library);
            return;
        }

        FT_Set_Pixel_Sizes(face, 0, rasterPixelSize);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (unsigned char character = 32; character < 128; ++character)
        {
            if (FT_Load_Char(face, character, FT_LOAD_RENDER) != 0)
                continue;

            GLuint texture = 0;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RED,
                face->glyph->bitmap.width, face->glyph->bitmap.rows,
                0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glyphs.emplace(
                static_cast<char>(character),
                Character{
                    texture,
                    glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                    glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                    face->glyph->advance.x
                });
        }

        FT_Done_Face(face);
        FT_Done_FreeType(library);
        glBindTexture(GL_TEXTURE_2D, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        if (VAO == 0)
        {
            constexpr GLuint POSITION_LOCATION = 0;
            constexpr GLuint UV_LOCATION = 3;
            constexpr GLsizei STRIDE = 5 * sizeof(float);

            const float vertices[6][5] = {};
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

            glEnableVertexAttribArray(POSITION_LOCATION);
            glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, STRIDE, nullptr);

            glEnableVertexAttribArray(UV_LOCATION);
            glVertexAttribPointer(UV_LOCATION, 2, GL_FLOAT, GL_FALSE, STRIDE, reinterpret_cast<void*>(3 * sizeof(float)));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }
    }
};

// Backwards-compatibility alias: WorldTextRenderer is now called TextRenderer
using WorldTextRenderer = TextRenderer;

// ScreenTextRenderer is defined in its own header
#include "ScreenTextRenderer.h"
