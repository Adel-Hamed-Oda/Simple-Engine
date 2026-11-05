#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <limits>
#include <string>
#include <utility>

#include "TextRenderer.h"
#include "../../core/rendering/material/TextMaterial.h"
#include "../../core/transform/Transform.h"

// Screen-Space Text Renderer (HUD, menus, subtitles; ignores camera view/projection and depth testing).
// Normalized to [-0.5, +0.5] so Position (0, 0, 0) is center, (0.5, 0.5, 0) is top-right,
// and Scale (1, 1, 1) is full screen width & height.
class ScreenTextRenderer : public TextRenderer
{
public:
    // Local size multiplier relative to owner Transform's scale (1.0 matches full quad bounds)
    glm::vec2 size{ 1.0f, 1.0f };

    // Local offset relative to owner Transform's center in screen-space
    glm::vec2 offset{ 0.0f, 0.0f };

    ScreenTextRenderer(
        std::string text,
        std::string fontPath,
        unsigned int pixelSize,
        TextMaterial* material,
        int zOrder
    )
        : TextRenderer(std::move(text), std::move(fontPath), pixelSize, material, zOrder)
    {
    }

    ScreenTextRenderer(
        std::string text,
        std::string fontPath,
        unsigned int pixelSize,
        TextMaterial* material
    )
        : TextRenderer(std::move(text), std::move(fontPath), pixelSize, material, 0)
    {
    }

    ScreenTextRenderer(
        std::string text,
        unsigned int pixelSize,
        TextMaterial* material,
        int zOrder
    )
        : TextRenderer(std::move(text), "", pixelSize, material, zOrder)
    {
    }

    ScreenTextRenderer(
        std::string text,
        unsigned int pixelSize,
        TextMaterial* material
    )
        : TextRenderer(std::move(text), "", pixelSize, material, 0)
    {
    }

    void Render() const override
    {
        if (VAO == 0 || glyphs.empty() || !transform || !material || text.empty())
            return;

        // Screen-space text uses an identity view and screen orthographic projection.
        if (!material->SetupScreenSpaceUniforms(transform))
            return;

        material->ApplyMaterial();

        const float unit = 1.0f / static_cast<float>(rasterPixelSize);   // glyph pixels -> local units

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        bool hasVisibleGlyph = false;
        float advanceX = 0.0f;

        for (const char character : text)
        {
            const auto glyph = glyphs.find(character);
            if (glyph == glyphs.end())
                continue;

            const Character& g = glyph->second;
            const float xpos = advanceX + g.bearing.x * unit;
            const float ypos = (g.bearing.y - g.size.y) * unit;
            const float w = g.size.x * unit;
            const float h = g.size.y * unit;

            if (w > 0.0f && h > 0.0f)
            {
                minX = (std::min)(minX, xpos);
                maxX = (std::max)(maxX, xpos + w);
                minY = (std::min)(minY, ypos);
                maxY = (std::max)(maxY, ypos + h);
                hasVisibleGlyph = true;
            }

            advanceX += static_cast<float>(g.advance >> 6) * unit;
        }

        if (!hasVisibleGlyph)
            return;

        minX = (std::min)(minX, 0.0f);
        maxX = (std::max)(maxX, advanceX);

        const float textWidth = maxX - minX;
        const float textHeight = maxY - minY;
        if (textWidth <= 0.0f || textHeight <= 0.0f)
            return;

        const float centerX = (minX + maxX) * 0.5f;
        const float centerY = (minY + maxY) * 0.5f;
        const float invWidth = 1.0f / textWidth;
        const float invHeight = 1.0f / textHeight;

        const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
        const GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean depthMask = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);
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
            const float ypos = (g.bearing.y - g.size.y) * unit;
            const float w = g.size.x * unit;
            const float h = g.size.y * unit;

            const float x0 = (((xpos - centerX) * invWidth) * size.x + offset.x);
            const float x1 = (((xpos + w - centerX) * invWidth) * size.x + offset.x);
            const float y0 = (((ypos - centerY) * invHeight) * size.y + offset.y);
            const float y1 = (((ypos + h - centerY) * invHeight) * size.y + offset.y);

            const float vertices[6][5] = {
                { x0, y0, 0.0f, 0.0f, 1.0f },  // bottom-left
                { x1, y0, 0.0f, 1.0f, 1.0f },  // bottom-right
                { x1, y1, 0.0f, 1.0f, 0.0f },  // top-right

                { x0, y0, 0.0f, 0.0f, 1.0f },  // bottom-left
                { x1, y1, 0.0f, 1.0f, 0.0f },  // top-right
                { x0, y1, 0.0f, 0.0f, 0.0f }   // top-left
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
};
