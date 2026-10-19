#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "../Component.h"
#include "../../core/rendering/material/TextMaterial.h"   // adjust to your layout

// World-space text. The string is laid out in the object's local XY plane,
// facing +Z, with the baseline at y = 0 and the first glyph starting at x = 0.
// One em (the font's pixelSize) equals one world unit at Transform scale 1,
// so Transform position/rotation/scale place, orient and size the text.
class TextRenderer : public Component
{
public:
    TextRenderer(
        std::string text,
        std::string fontPath,
        unsigned int pixelSize = 48,
        std::shared_ptr<TextMaterial> material = nullptr,
        int zOrder = 0
    )
        : text(std::move(text)),
          fontPath(std::move(fontPath)),
          pixelSize(pixelSize),
          material(material ? std::move(material) : std::make_shared<TextMaterial>("Text")),
          zOrder(zOrder)
    {
        Initialize();
    }

    ~TextRenderer() override
    {
        for (const auto& [character, glyph] : glyphs)
            glDeleteTextures(1, &glyph.textureID);

        if (VAO != 0)
            glDeleteVertexArrays(1, &VAO);
        if (VBO != 0)
            glDeleteBuffers(1, &VBO);
    }

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    virtual bool IsScreenSpace() const { return false; }

    void SetText(const std::string& value) { text = value; }
    const std::string& GetText() const { return text; }

    TextMaterial* GetMaterial() const { return material.get(); }
    void SetMaterial(std::shared_ptr<TextMaterial> value)
    {
        if (value)
            material = std::move(value);
    }

    void Render() const
    {
        if (VAO == 0 || glyphs.empty() || !transform || !material || text.empty())
            return;

        const bool screenSpace = IsScreenSpace();

        // World-space text uses the main camera's view/projection.
        // Screen-space text uses an identity view and screen orthographic projection.
        const bool uniformsReady = screenSpace
            ? material->SetupScreenSpaceUniforms(*transform)
            : material->SetupCoreUniforms(*transform);

        if (!uniformsReady)
            return;
        material->ApplyMaterial();

        const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
        const GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean depthMask = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        if (screenSpace)
            glDisable(GL_DEPTH_TEST);
        if (material->doubleSided)
            glDisable(GL_CULL_FACE);

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(VAO);

        const float unit = 1.0f / static_cast<float>(pixelSize);   // glyph pixels -> world units
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
            const float vertices[6][4] = {
                { xpos,     ypos,     0.0f, 1.0f },  // bottom-left
                { xpos + w, ypos,     1.0f, 1.0f },  // bottom-right
                { xpos + w, ypos + h, 1.0f, 0.0f },  // top-right

                { xpos,     ypos,     0.0f, 1.0f },  // bottom-left
                { xpos + w, ypos + h, 1.0f, 0.0f },  // top-right
                { xpos,     ypos + h, 0.0f, 0.0f }   // top-left
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

    int GetZOrder() const { return zOrder; }
    void SetZOrder(int value) { zOrder = value; }

private:
    struct Character
    {
        GLuint textureID = 0;
        glm::ivec2 size = glm::ivec2(0);
        glm::ivec2 bearing = glm::ivec2(0);
        long advance = 0;
    };

    std::string text;
    std::string fontPath;
    unsigned int pixelSize;
    std::shared_ptr<TextMaterial> material;
    int zOrder = 0;

    std::map<char, Character> glyphs;
    GLuint VAO = 0;
    GLuint VBO = 0;

    void Initialize()
    {
        FT_Library library = nullptr;
        FT_Face face = nullptr;
        if (FT_Init_FreeType(&library) != 0 ||
            FT_New_Face(library, fontPath.c_str(), 0, &face) != 0)
        {
            std::cerr << "ERROR::TEXT_RENDERER::FONT_LOAD_FAILED: " << fontPath << std::endl;
            if (library)
                FT_Done_FreeType(library);
            return;
        }

        FT_Set_Pixel_Sizes(face, 0, pixelSize);
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
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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

        const float vertices[6][4] = {};
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
};

// Explicit World-Space Text Renderer (affected by camera view/projection and depth test).
class WorldTextRenderer : public TextRenderer
{
public:
    using TextRenderer::TextRenderer;
    bool IsScreenSpace() const override { return false; }
};

// Screen-Space Text Renderer (HUD, menus, subtitles; ignores camera view/projection and depth testing).
class ScreenTextRenderer : public TextRenderer
{
public:
    using TextRenderer::TextRenderer;
    bool IsScreenSpace() const override { return true; }
};