#pragma once

#include <glm/glm.hpp>

#include "Collider.h"
#include "../../main/transform/Transform.h"

class BoxCollider : public Collider
{
public:
    bool isTrigger;
    glm::vec3 size;

    explicit BoxCollider(
        bool isTrigger = false,
        glm::vec3 center = glm::vec3(0.0f),
        glm::vec3 size = glm::vec3(1.0f)
    )
        : Collider(center),
        isTrigger(isTrigger),
        size(size)
    {
    }

    bool IsCollidingWith(const BoxCollider& other) const
    {
        glm::vec3 thisMin =
            transform->position +
            center -
            size * 0.5f;

        glm::vec3 thisMax =
            transform->position +
            center +
            size * 0.5f;

        glm::vec3 otherMin =
            other.transform->position +
            other.center -
            other.size * 0.5f;

        glm::vec3 otherMax =
            other.transform->position +
            other.center +
            other.size * 0.5f;

        return
            thisMin.x <= otherMax.x &&
            thisMax.x >= otherMin.x &&
            thisMin.y <= otherMax.y &&
            thisMax.y >= otherMin.y &&
            thisMin.z <= otherMax.z &&
            thisMax.z >= otherMin.z;
    }
};