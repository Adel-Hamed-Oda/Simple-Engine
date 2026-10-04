#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../behaviour/Behaviour.h"
#include "../../core/transform/Transform.h"
#include "../../core/statics/EngineTime.h"

// ------------------------------------------------------------------------
// Constants
// ------------------------------------------------------------------------

enum class ProjectionType
{
	Perspective,
	Orthographic
};

class Camera : public Behaviour
{
public:
	inline static Camera* mainCamera = nullptr;

	ProjectionType projectionType;
	float fov = 45.0f;
	float nearPlane = 0.1f;
	float farPlane = 1000.0f;
	float orthoScale = 1.0f;
	
	Camera(ProjectionType projectionType = ProjectionType::Perspective)
		: projectionType(projectionType)
	{
		if (!mainCamera)
			mainCamera = this;
	}

	~Camera() override
	{
		if (mainCamera == this)
			mainCamera = nullptr;
	}

	glm::mat4 GetViewMatrix()
	{
		if (!transform) return glm::mat4(1.0f);

		glm::mat4 view = glm::mat4(1.0f);

		view = glm::rotate(view, glm::radians(-transform->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		view = glm::rotate(view, glm::radians(-transform->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		view = glm::rotate(view, glm::radians(-transform->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		view = glm::translate(view, -transform->position);

		return view;
	}

	glm::mat4 GetProjectionMatrix(float aspectRatio)
	{
		if (projectionType == ProjectionType::Orthographic)
		{
			return glm::ortho(-orthoScale * aspectRatio, orthoScale * aspectRatio, -orthoScale, orthoScale, nearPlane, farPlane);
		}
		else
		{
			return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
		}
	}
};