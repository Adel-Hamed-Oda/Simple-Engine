#pragma once

#include <vector>
#include <algorithm>

#include <glm/glm.hpp>

#include "../Component.h"
#include "../../main/transform/Transform.h"

class LightSource : public Component
{
public:
    inline static std::vector<LightSource*> allLights;

    glm::vec3 color;

    LightSource(glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f))
        : color(color)
    {
        allLights.push_back(this);
    }
    ~LightSource() override
    {
        allLights.erase(
            std::remove(allLights.begin(), allLights.end(), this),
            allLights.end()
        );
    }
};