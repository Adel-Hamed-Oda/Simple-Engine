#pragma once

#include <glm/glm.hpp>
#include <algorithm>
#include <cstdint>
#include <vector>

#include "../Component.h"
#include "../../core/events/Event.h"

class Collider : public Component
{
public:
    inline static std::vector<Collider*> allColliders;

    bool isTrigger = false;

    Event<GameObject*> OnCollisionEnter;
    Event<GameObject*> OnCollisionStay;
    Event<GameObject*> OnCollisionExit;

    explicit Collider(bool isTrigger = false)
        : isTrigger(isTrigger),
          colliderId(++nextColliderId)
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

    uint64_t GetColliderId() const { return colliderId; }

    static Collider* FindById(uint64_t id)
    {
        for (Collider* col : allColliders)
        {
            if (col && col->colliderId == id)
                return col;
        }
        return nullptr;
    }

private:
    inline static uint64_t nextColliderId = 0;
    uint64_t colliderId = 0;
};
