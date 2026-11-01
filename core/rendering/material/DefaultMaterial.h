#pragma once

#include <glm/glm.hpp>

#include "Material.h"
#include "../texture/Texture.h"

class DefaultMaterial : public Material
{
public:
	Texture* texture = nullptr;
	glm::vec4 baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	glm::vec3 ambient = glm::vec3(0.1f, 0.1f, 0.1f);
	float specularStrength = 0.5f;
	float shininess = 32.0f;

	DefaultMaterial() = default;

	explicit DefaultMaterial(Shader* shader)
		: Material(shader) {}

	~DefaultMaterial() override = default;

	Texture* GetTexture() const override { return texture; }
	void SetTexture(Texture* tex) override { texture = tex; }

	void ApplyMaterial() override
	{
		if (!shader || !shader->IsValid()) return;

		const bool hasTexture = texture && texture->IsValid();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, hasTexture ? texture->ID : 0);

		shader->setBool("material.hasTexture", hasTexture);
		shader->setInt("material.textureSampler", 0);
		shader->setVec4("material.baseColor", baseColor);
		shader->setVec3("material.ambient", ambient);
		shader->setFloat("material.specularStrength", specularStrength);
		shader->setFloat("material.shininess", shininess);
	}
};
