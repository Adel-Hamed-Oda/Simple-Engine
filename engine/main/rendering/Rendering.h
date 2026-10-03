#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <string>

#include "../../components/meshrenderer/MeshRenderer.h"
#include "../gameobject/GameObject.h"
#include "../rendering/material/Material.h"
#include "../rendering/shader/Shader.h"
#include "../../components/light_source/LightSource.h"
#include "../../Config.h"

class Rendering
{
public:
	static void SetupRendering()
	{
		if (PublicDomain::Rendering::FaceCulling != FACE_CULLING_MODE::NONE)
		{
			glEnable(GL_CULL_FACE); // Enable back-face culling
		}

		if (PublicDomain::Rendering::FaceCulling == FACE_CULLING_MODE::BACK)
		{
			glCullFace(GL_BACK); // Cull back faces
		}
		else if (PublicDomain::Rendering::FaceCulling == FACE_CULLING_MODE::FRONT)
		{
			glCullFace(GL_FRONT); // Cull front faces
		}
		else if (PublicDomain::Rendering::FaceCulling == FACE_CULLING_MODE::FRONT_AND_BACK)
		{
			glCullFace(GL_FRONT_AND_BACK); // Cull both front and back faces
		}

		if (PublicDomain::Rendering::EnableDepthTest)
		{
			glEnable(GL_DEPTH_TEST);
		}
	}

	static void RenderGameObject(GameObject* obj)
	{
		MeshRenderer* meshRenderer = obj->GetComponent<MeshRenderer>();
		Transform* transform = obj->transform.get();

		if (!meshRenderer || !transform) return;

		Material* material = meshRenderer->GetMaterial();

		if (!material) return;

		// --------------------------------------------------
		// Bind shader + positional uniforms (model/view/projection)
		// --------------------------------------------------

		if (!material->SetupCoreUniforms(*transform)) return;

		// --------------------------------------------------
		// Material properties
		// --------------------------------------------------

		material->ApplyMaterial();

		// --------------------------------------------------
		// Lighting - every active LightSource in the scene
		// --------------------------------------------------

		SendLights(material->shader);

		// --------------------------------------------------
		// Draw
		// --------------------------------------------------

		meshRenderer->Draw();
	}

private:
	static void SendLights(Shader* shader)
	{
		if (!shader) return;

		const auto& lights = LightSource::allLights;

		const int lightCount = static_cast<int>(
			std::min(lights.size(), static_cast<size_t>(CONFIG::LIGHTING::MAX_LIGHTS))
		);

		shader->setInt("numLights", lightCount);

		for (int i = 0; i < lightCount; ++i)
		{
			LightSource* light = lights[i];
			const std::string prefix = "lights[" + std::to_string(i) + "].";

			shader->setVec3(prefix + "position", light->transform->position);
			shader->setVec3(prefix + "color", light->color);
		}
	}
};