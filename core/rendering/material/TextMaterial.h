#pragma once

#include <string>
#include <glm/glm.hpp>

#include "Material.h"

class TextMaterial : public Material
{
public:
	glm::vec3 color = glm::vec3(1.0f);
	bool doubleSided = true;   // false: text is only visible from its front (+Z) side

	explicit TextMaterial(
		const std::string& shaderName = ""
	) : Material(shaderName) {}
	~TextMaterial() override = default;

	void ApplyMaterial() override
	{
		if (!shader) return;

		shader->setVec3("textColor", color);
		shader->setInt("textTexture", 0);   // glyph bitmap is bound to unit 0 per character
	}
};