#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <string>
#include <vector>

#include "../../components/image_renderer/ImageRenderer.h"
#include "../../components/mesh_renderer/MeshRenderer.h"
#include "../../components/text_renderer/TextRenderer.h"
#include "../../components/light_source/LightSource.h"

#include "../gameobject/GameObject.h"
#include "../rendering/material/Material.h"
#include "../rendering/shader/Shader.h"
#include "../../scenes/Scene.h"

#include "../../Config.h"

class Rendering
{
public:
    static void SetupRendering()
    {
        // Configure face culling explicitly.
        if (PublicDomain::Rendering::FaceCulling == FACE_CULLING_MODE::NONE)
        {
            glDisable(GL_CULL_FACE);
        }
        else
        {
            glEnable(GL_CULL_FACE);

            switch (PublicDomain::Rendering::FaceCulling)
            {
                case FACE_CULLING_MODE::BACK:
                    glCullFace(GL_BACK);
                    break;

                case FACE_CULLING_MODE::FRONT:
                    glCullFace(GL_FRONT);
                    break;

                case FACE_CULLING_MODE::FRONT_AND_BACK:
                    glCullFace(GL_FRONT_AND_BACK);
                    break;

                default:
                    glDisable(GL_CULL_FACE);
                    break;
            }
        }

        // Configure depth testing explicitly.
        if (PublicDomain::Rendering::EnableDepthTest)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }
    }

	static void RenderScene(const Scene* scene)
	{
		if (!scene)
			return;

		deferredWorldTextRenderers.clear();
		deferredScreenDrawItems.clear();

		// Pass 1: World-Space Meshes (Opaque, Lit, Depth-Tested)
		for (const auto& obj : scene->GetGameObjects())
		{
			if (!obj || !obj->transform)
				continue;

			Transform* transform = obj->transform.get();

			for (MeshRenderer* meshRenderer : obj->GetComponents<MeshRenderer>())
			{
				Material* material = meshRenderer->GetMaterial();

				if (material && material->SetupCoreUniforms(transform))
				{
					material->ApplyMaterial();
					SendLights(material->shader);
					meshRenderer->Draw();
				}
			}

			for (ImageRenderer* imageRenderer : obj->GetComponents<ImageRenderer>())
			{
				deferredScreenDrawItems.push_back(ScreenDrawItem{
					imageRenderer->GetZOrder(),
					imageRenderer,
					nullptr
				});
			}

			for (TextRenderer* textRenderer : obj->GetComponents<TextRenderer>())
			{
				if (textRenderer->IsScreenSpace())
				{
					deferredScreenDrawItems.push_back(ScreenDrawItem{
						textRenderer->GetZOrder(),
						nullptr,
						textRenderer
					});
				}
				else
				{
					deferredWorldTextRenderers.push_back(textRenderer);
				}
			}
		}

		// Pass 2: World-Space Text (Depth-Tested against 3D world, sorted by zOrder)
		std::stable_sort(
			deferredWorldTextRenderers.begin(),
			deferredWorldTextRenderers.end(),
			[](const TextRenderer* a, const TextRenderer* b)
			{
				return a->GetZOrder() < b->GetZOrder();
			});

		for (TextRenderer* textRenderer : deferredWorldTextRenderers)
		{
			if (textRenderer)
				textRenderer->Render();
		}

		deferredWorldTextRenderers.clear();

		// Pass 3: Screen-Space UI (ImageRenderer + ScreenTextRenderer sorted together by zOrder)
		std::stable_sort(
			deferredScreenDrawItems.begin(),
			deferredScreenDrawItems.end(),
			[](const ScreenDrawItem& a, const ScreenDrawItem& b)
			{
				return a.zOrder < b.zOrder;
			});

		for (const ScreenDrawItem& item : deferredScreenDrawItems)
		{
			item.Render();
		}

		deferredScreenDrawItems.clear();
	}

private:
    struct ScreenDrawItem
    {
        int zOrder = 0;
        const ImageRenderer* imageRenderer = nullptr;
        const TextRenderer* textRenderer = nullptr;

        void Render() const
        {
            if (imageRenderer)
                imageRenderer->Render();
            else if (textRenderer)
                textRenderer->Render();
        }
    };

    inline static std::vector<TextRenderer*> deferredWorldTextRenderers{};
    inline static std::vector<ScreenDrawItem> deferredScreenDrawItems{};

    static void SendLights(const Shader* shader)
    {
        if (!shader || !shader->IsValid())
            return;

        const auto& lights = LightSource::allLights;

        const int maxLights =
            std::max(0, CONFIG::LIGHTING::MAX_LIGHTS);

        const int lightCount = static_cast<int>(
            std::min(lights.size(),
                     static_cast<std::size_t>(maxLights)));

        shader->setInt("numLights", lightCount);

        for (int i = 0; i < lightCount; ++i)
        {
            LightSource* light = lights[i];

            if (!light || !light->transform)
                continue;

            const std::string prefix =
                "lights[" + std::to_string(i) + "].";

            shader->setVec3(
                prefix + "position",
                light->transform->position);

            shader->setVec3(
                prefix + "color",
                light->color);
        }
    }
};
