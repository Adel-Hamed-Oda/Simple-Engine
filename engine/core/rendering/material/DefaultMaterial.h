#pragma once

#include <string>

#include <glm/glm.hpp>

#include "Material.h"
#include "../texture/Texture.h"

class DefaultMaterial : public Material
{
public:
	std::string textureName = "";
	glm::vec3 baseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 ambient = glm::vec3(0.1f, 0.1f, 0.1f);
	float specularStrength = 0.5f;
	float shininess = 32.0f;

	explicit DefaultMaterial(const std::string& shaderName) : Material(shaderName) {}
	~DefaultMaterial() override = default;

	void ApplyMaterial() override
	{
		if (!shader) return;

		bool hasTexture = false;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);

		if (!textureName.empty())
		{
			Texture* texture = Texture::Get(textureName);
			if (texture)
			{
				glBindTexture(GL_TEXTURE_2D, texture->ID);
				hasTexture = true;
			}
		}

		shader->setBool("material.hasTexture", hasTexture);
		shader->setInt("material.textureSampler", 0);
		shader->setVec3("material.baseColor", baseColor);
		shader->setVec3("material.ambient", ambient);
		shader->setFloat("material.specularStrength", specularStrength);
		shader->setFloat("material.shininess", shininess);
	}
};