#pragma once

#include <string>
#include <glm/glm.hpp>

#include "Material.h"
#include "../texture/Texture.h"

class ImageMaterial : public Material
{
public:
	std::string textureName = "";
	glm::vec4 color = glm::vec4(1.0f);
	bool grayscale = false;

	explicit ImageMaterial(
		const std::string& textureName = "",
		const std::string& shaderName = "Image"
	)
		: Material(shaderName), textureName(textureName) {}

	~ImageMaterial() override = default;

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

		shader->setBool("hasTexture", hasTexture);
		shader->setInt("imageTexture", 0);
		shader->setVec4("imageColor", color);
		shader->setBool("grayscale", grayscale);
	}
};

