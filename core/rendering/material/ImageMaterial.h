#pragma once

#include <glm/glm.hpp>

#include "Material.h"
#include "../texture/Texture.h"

class ImageMaterial : public Material
{
public:
	Texture* texture = nullptr;
	glm::vec4 color = glm::vec4(1.0f);
	bool grayscale = false;

	ImageMaterial() = default;

	explicit ImageMaterial(Shader* shader, Texture* texture = nullptr)
		: Material(shader),
		  texture(texture) {}

	~ImageMaterial() override = default;

	static ImageMaterial* Create(Shader* shader, Texture* texture = nullptr)
	{
		return new ImageMaterial(shader, texture);
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

		shader->setBool("hasTexture", hasTexture);
		shader->setInt("imageTexture", 0);
		shader->setVec4("imageColor", color);
		shader->setBool("grayscale", grayscale);
	}
};
