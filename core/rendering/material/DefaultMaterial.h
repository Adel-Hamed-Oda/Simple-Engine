#pragma once

#include <glm/glm.hpp>

#include "Material.h"
#include "../texture/Texture.h"

class DefaultMaterial : public Material
{
public:
	Texture* texture = nullptr;
	glm::vec3 baseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 ambient = glm::vec3(0.1f, 0.1f, 0.1f);
	float specularStrength = 0.5f;
	float shininess = 32.0f;

	DefaultMaterial() = default;

	explicit DefaultMaterial(Shader* shader, Texture* texture = nullptr)
		: Material(shader),
		  texture(texture) {}

	~DefaultMaterial() override = default;

	static DefaultMaterial* Create(Shader* shader, Texture* texture = nullptr)
	{
		return new DefaultMaterial(shader, texture);
	}

	Texture* GetTexture() const { return texture; }
	bool HasTexture() const { return texture != nullptr && texture->IsValid(); }

	void SetTexture(Texture* tex) { texture = tex; }
	void ClearTexture() { texture = nullptr; }

	void ApplyMaterial() override
	{
		if (!shader || !shader->IsValid()) return;

		const bool hasTexture = HasTexture();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, hasTexture ? texture->ID : 0);

		shader->setBool("material.hasTexture", hasTexture);
		shader->setInt("material.textureSampler", 0);
		shader->setVec3("material.baseColor", baseColor);
		shader->setVec3("material.ambient", ambient);
		shader->setFloat("material.specularStrength", specularStrength);
		shader->setFloat("material.shininess", shininess);
	}
};
