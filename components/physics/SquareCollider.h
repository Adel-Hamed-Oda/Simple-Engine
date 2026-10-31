#pragma once

#include <glm/glm.hpp>

#include "Collider.h"
#include "../../core/transform/Transform.h"

class SquareCollider : public Collider
{
public:
    glm::vec2 center;
    glm::vec2 size;

    explicit SquareCollider(
        bool isTrigger = false,
        glm::vec2 center = glm::vec2(0.0f),
        glm::vec2 size = glm::vec2(1.0f)
    )
        : Collider(isTrigger),
          center(center),
          size(size)
    {
    }

    bool IsCollidingWith(const SquareCollider& other) const
    {
        if (&other == this || !transform || !other.transform)
            return false;

        if (owner && other.owner == owner)
            return false;

        const glm::vec2 thisPos(transform->position.x, transform->position.y);
        const glm::vec2 otherPos(other.transform->position.x, other.transform->position.y);

        const glm::vec2 thisMin =
            thisPos +
            center -
            size * 0.5f;

        const glm::vec2 thisMax =
            thisPos +
            center +
            size * 0.5f;

        const glm::vec2 otherMin =
            otherPos +
            other.center -
            other.size * 0.5f;

        const glm::vec2 otherMax =
            otherPos +
            other.center +
            other.size * 0.5f;

        return
            thisMin.x <= otherMax.x &&
            thisMax.x >= otherMin.x &&
            thisMin.y <= otherMax.y &&
            thisMax.y >= otherMin.y;
    }
};

