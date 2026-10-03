#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

#include "../../statics/PublicDomain.h"
#include "../../../components/camera/Camera.h"
#include "../../transform/Transform.h"
#include "../shader/Shader.h"
#include "../shader/ShadersLoader.h"

// A Material is shared, per-shader *data* (colors, shininess, ...).
// It is NOT a Component: it isn't attached to one GameObject, so it has no
// transform of its own. The object being drawn passes its Transform in.
class Material
{
public:
	Shader* shader = nullptr;

	explicit Material(const std::string& shaderName)
	{
		shader = ShadersLoader::GetShader(shaderName);
		if (!shader)
		{
			std::cerr
				<< "ERROR::MATERIAL::SHADER_NOT_FOUND: "
				<< shaderName
				<< std::endl;
		}
	}
	virtual ~Material() = default;

	// Binds the shader program, then uploads model/view/projection/viewer position.
	// Returns false if the object can't be drawn (missing shader or camera).
	bool SetupCoreUniforms(const Transform& transform) const
	{
		if (!shader || shader->ID == 0) return false;

		if (!Camera::mainCamera || !Camera::mainCamera->transform)
		{
			std::cerr << "ERROR::MATERIAL::NO_MAIN_CAMERA" << std::endl;
			return false;
		}

		// Uniforms are uploaded to whichever program is currently bound,
		// so the program MUST be bound before any setXxx() call.
		shader->use();

		shader->setMat4("model", transform.GetModelMatrix());
		shader->setMat4("view", Camera::mainCamera->GetViewMatrix());
		shader->setMat4("projection", Camera::mainCamera->GetProjectionMatrix(GetAspectRatio()));
		shader->setVec3("viewingPosition", Camera::mainCamera->transform->position);

		return true;
	}

	// Uploads this material's own properties (called after SetupCoreUniforms).
	virtual void ApplyMaterial() = 0;

private:
	static float GetAspectRatio()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(PublicDomain::CurrentWindow->GetGLFWWindow(), &width, &height);
		if (height == 0) return 1.0f; // minimized window
		return static_cast<float>(width) / static_cast<float>(height);
	}
};