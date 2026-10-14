#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <string>
#include <vector>

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

		deferredTextRenderers.clear();

		for (const auto& obj : scene->GetGameObjects())
		{
			if (!obj || !obj->transform)
				continue;

			Transform& transform = *obj->transform;

			if (MeshRenderer* meshRenderer =
					obj->GetComponent<MeshRenderer>())
			{
				Material* material = meshRenderer->GetMaterial();

				if (material && material->SetupCoreUniforms(transform))
				{
					material->ApplyMaterial();
					SendLights(material->shader);
					meshRenderer->Draw();
				}
			}

			if (TextRenderer* textRenderer =
					obj->GetComponent<TextRenderer>())
			{
				deferredTextRenderers.push_back(textRenderer);
			}
		}

		std::stable_sort(
			deferredTextRenderers.begin(),
			deferredTextRenderers.end(),
			[](const TextRenderer* a, const TextRenderer* b)
			{
				return a->GetZOrder() < b->GetZOrder();
			});

		for (TextRenderer* textRenderer : deferredTextRenderers)
		{
			if (textRenderer)
				textRenderer->Render();
		}

		deferredTextRenderers.clear();
	}

private:
    // C++17 or later: inline static avoids a separate definition in a .cpp.
    inline static std::vector<TextRenderer*> deferredTextRenderers{};

    static void SendLights(Shader* shader)
    {
        if (!shader)
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