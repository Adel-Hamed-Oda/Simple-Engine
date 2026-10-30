#pragma once

#include <glm/glm.hpp>

#include "Material.h"

class TextMaterial : public Material
{
public:
	glm::vec3 color = glm::vec3(1.0f);
	bool doubleSided = true;   // false: text is only visible from its front (+Z) side

	TextMaterial() = default;

	explicit TextMaterial(Shader* shader)
		: Material(shader) {}

	~TextMaterial() override = default;

	void ApplyMaterial() override
	{
		if (!shader || !shader->IsValid()) return;

		shader->setVec3("textColor", color);
		shader->setInt("textTexture", 0);   // glyph bitmap is bound to unit 0 per character
	}
};
