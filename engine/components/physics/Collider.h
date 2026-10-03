#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <algorithm>

#include "../Component.h"

class Collider : public Component
{
public:
    inline static std::vector<Collider*> allColliders;

    glm::vec3 center;

    explicit Collider(glm::vec3 center = glm::vec3(0.0f))
        : center(center)
    {
        allColliders.push_back(this);
    }

    virtual ~Collider() override
    {
        allColliders.erase(
            std::remove(
                allColliders.begin(),
                allColliders.end(),
                this
            ),
            allColliders.end()
        );
    }
};