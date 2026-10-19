#pragma once

#include <glad/gl.h>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include "../Component.h"
#include "../../core/rendering/material/ImageMaterial.h"

// Screen-space 2D image/quad renderer for HUD, cards, buttons, and menus.
// Uses an identity view matrix and a screen-space orthographic projection,
// ignoring Camera::mainCamera and scene lighting.
class ImageRenderer : public Component
{
public:
    explicit ImageRenderer(
        std::shared_ptr<ImageMaterial> material = nullptr,
        int zOrder = 0
    )
        : material(material ? std::move(material) : std::make_shared<ImageMaterial>()),
          zOrder(zOrder)
    {
        EnsureQuadInitialized();
    }

    explicit ImageRenderer(
        const std::string& textureName,
        int zOrder = 0
    )
        : material(std::make_shared<ImageMaterial>(textureName)),
          zOrder(zOrder)
    {
        EnsureQuadInitialized();
    }

    ~ImageRenderer() override = default;

    ImageMaterial* GetMaterial() const { return material.get(); }
    void SetMaterial(std::shared_ptr<ImageMaterial> value)
    {
        if (value)
            material = std::move(value);
    }

    int GetZOrder() const { return zOrder; }
    void SetZOrder(int value) { zOrder = value; }

    void Render() const
    {
        if (quadVAO == 0 || !transform || !material)
            return;

        if (!material->SetupScreenSpaceUniforms(*transform))
            return;
        material->ApplyMaterial();

        const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        const GLboolean cullEnabled  = glIsEnabled(GL_CULL_FACE);
        const GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        if (depthEnabled) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
        if (cullEnabled) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
        if (!blendEnabled) glDisable(GL_BLEND);
    }

private:
    std::shared_ptr<ImageMaterial> material;
    int zOrder = 0;

    inline static GLuint quadVAO = 0;
    inline static GLuint quadVBO = 0;

    struct QuadVertex
    {
        float pos[3];
        float normal[3];
        float color[4];
        float uv[2];
    };

    static void EnsureQuadInitialized()
    {
        if (quadVAO != 0)
            return;

        const QuadVertex vertices[6] = {
            // bottom-left
            { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
            // bottom-right
            { {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
            // top-right
            { {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },

            // bottom-left
            { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
            // top-right
            { {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
            // top-left
            { { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } }
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);

        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Location 0: Position (vec3)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), reinterpret_cast<void*>(offsetof(QuadVertex, pos)));

        // Location 1: Normal (vec3)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), reinterpret_cast<void*>(offsetof(QuadVertex, normal)));

        // Location 2: Color (vec4)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), reinterpret_cast<void*>(offsetof(QuadVertex, color)));

        // Location 3: UV (vec2)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), reinterpret_cast<void*>(offsetof(QuadVertex, uv)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
};