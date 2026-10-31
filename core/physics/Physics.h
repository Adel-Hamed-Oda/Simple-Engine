#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

#include "../../Config.h"
#include "../gameobject/GameObject.h"
#include "../statics/EngineTime.h"
#include "../statics/PublicDomain.h"
#include "../../components/physics/Collider.h"
#include "../../components/physics/BoxCollider.h"
#include "../../components/physics/SquareCollider.h"
#include "../../components/physics/Rigidbody.h"

class Physics
{
public:
    static void Update()
    {
        const float dt = (glm::min)(
            Time::deltaTime,
            CONFIG::PHYSICS::MAX_DELTA_TIME
        );

        // --------------------------------------------------
        // 1. Integrate rigidbodies
        // --------------------------------------------------

        for (Rigidbody* body : Rigidbody::allRigidbodies)
        {
            if (!body || !body->transform || body->isKinematic)
                continue;

            PhysicsStep(body, dt);
        }

        // --------------------------------------------------
        // 2. Collision detection + physical resolution
        // --------------------------------------------------

        std::vector<CollisionPair> currentPairs;

        for (size_t i = 0; i < Collider::allColliders.size(); ++i)
        {
            Collider* colA = Collider::allColliders[i];
            if (!colA || !colA->transform)
                continue;

            for (size_t j = i + 1; j < Collider::allColliders.size(); ++j)
            {
                Collider* colB = Collider::allColliders[j];
                if (!colB || !colB->transform)
                    continue;

                // Prevent self-collision (same collider or colliders on the same GameObject)
                if (colA == colB)
                    continue;
                if (colA->owner && colA->owner == colB->owner)
                    continue;

                bool colliding = false;

                if (BoxCollider* boxA = dynamic_cast<BoxCollider*>(colA))
                {
                    if (BoxCollider* boxB = dynamic_cast<BoxCollider*>(colB))
                    {
                        colliding = CollisionStep3D(boxA, boxB);
                    }
                }
                else if (SquareCollider* sqA = dynamic_cast<SquareCollider*>(colA))
                {
                    if (SquareCollider* sqB = dynamic_cast<SquareCollider*>(colB))
                    {
                        colliding = CollisionStep2D(sqA, sqB);
                    }
                }

                if (colliding)
                {
                    currentPairs.emplace_back(colA->GetColliderId(), colB->GetColliderId());
                }
            }
        }

        // --------------------------------------------------
        // 3. Diff current vs previous pairs for Enter / Stay / Exit
        // --------------------------------------------------

        std::sort(currentPairs.begin(), currentPairs.end());
        currentPairs.erase(
            std::unique(currentPairs.begin(), currentPairs.end()),
            currentPairs.end()
        );

        std::vector<CollisionPair> enterPairs;
        std::vector<CollisionPair> stayPairs;
        std::vector<CollisionPair> exitPairs;

        size_t prevIdx = 0;
        size_t currIdx = 0;

        while (prevIdx < previousPairs.size() && currIdx < currentPairs.size())
        {
            const CollisionPair& prev = previousPairs[prevIdx];
            const CollisionPair& curr = currentPairs[currIdx];

            if (curr < prev)
            {
                enterPairs.push_back(curr);
                ++currIdx;
            }
            else if (prev < curr)
            {
                exitPairs.push_back(prev);
                ++prevIdx;
            }
            else
            {
                stayPairs.push_back(curr);
                ++prevIdx;
                ++currIdx;
            }
        }

        while (currIdx < currentPairs.size())
        {
            enterPairs.push_back(currentPairs[currIdx++]);
        }

        while (prevIdx < previousPairs.size())
        {
            exitPairs.push_back(previousPairs[prevIdx++]);
        }

        previousPairs = std::move(currentPairs);

        // --------------------------------------------------
        // 4. Dispatch OnCollisionEnter, OnCollisionStay, OnCollisionExit
        // --------------------------------------------------

        for (const CollisionPair& pair : enterPairs)
            DispatchCollisionEvent(pair, CollisionEventType::Enter);

        for (const CollisionPair& pair : stayPairs)
            DispatchCollisionEvent(pair, CollisionEventType::Stay);

        for (const CollisionPair& pair : exitPairs)
            DispatchCollisionEvent(pair, CollisionEventType::Exit);
    }

private:
    enum class CollisionEventType
    {
        Enter,
        Stay,
        Exit
    };

    struct CollisionPair
    {
        uint64_t idA = 0;
        uint64_t idB = 0;

        CollisionPair() = default;
        CollisionPair(uint64_t a, uint64_t b)
            : idA(a < b ? a : b),
              idB(a < b ? b : a)
        {
        }

        bool operator==(const CollisionPair& other) const
        {
            return idA == other.idA && idB == other.idB;
        }

        bool operator<(const CollisionPair& other) const
        {
            return (idA < other.idA) || (idA == other.idA && idB < other.idB);
        }
    };

    inline static std::vector<CollisionPair> previousPairs;

    static void InvokeColliderEvent(Collider* collider, CollisionEventType type, GameObject* other)
    {
        switch (type)
        {
            case CollisionEventType::Enter:
                collider->OnCollisionEnter(other);
                break;
            case CollisionEventType::Stay:
                collider->OnCollisionStay(other);
                break;
            case CollisionEventType::Exit:
                collider->OnCollisionExit(other);
                break;
        }
    }

    static void DispatchCollisionEvent(const CollisionPair& pair, CollisionEventType type)
    {
        Collider* a = Collider::FindById(pair.idA);
        Collider* b = Collider::FindById(pair.idB);
        if (a && b && a->owner && b->owner && a->owner != b->owner)
        {
            InvokeColliderEvent(a, type, b->owner);
        }

        // Re-resolve by ID in case the first callback destroyed either GameObject/Collider
        a = Collider::FindById(pair.idA);
        b = Collider::FindById(pair.idB);
        if (a && b && a->owner && b->owner && a->owner != b->owner)
        {
            InvokeColliderEvent(b, type, a->owner);
        }
    }

    static void PhysicsStep(Rigidbody* body, float dt)
    {
        if (body->mass <= 0.0f)
            return;

        glm::vec3 force = body->ConsumeForces();

        if (body->useGravity)
        {
            force +=
                PublicDomain::Physics::Gravity *
                body->gravityScale *
                body->mass;
        }

        body->velocity +=
            (force / body->mass) * dt;

        if (body->drag > 0.0f)
        {
            const float damping =
                1.0f - body->drag * dt;

            body->velocity *=
                (glm::max)(damping, 0.0f);
        }

        body->transform->position +=
            body->velocity * dt;
    }

    // 3D AABB collision test + optional physical resolution
    static bool CollisionStep3D(BoxCollider* a, BoxCollider* b)
    {
        if (!a || !b || !a->transform || !b->transform)
            return false;

        const glm::vec3 halfSizeA = glm::abs(a->size) * 0.5f;
        const glm::vec3 halfSizeB = glm::abs(b->size) * 0.5f;

        const glm::vec3 aMin = a->transform->position + a->center - halfSizeA;
        const glm::vec3 aMax = a->transform->position + a->center + halfSizeA;

        const glm::vec3 bMin = b->transform->position + b->center - halfSizeB;
        const glm::vec3 bMax = b->transform->position + b->center + halfSizeB;

        if (aMax.x <= bMin.x ||
            aMin.x >= bMax.x ||
            aMax.y <= bMin.y ||
            aMin.y >= bMax.y ||
            aMax.z <= bMin.z ||
            aMin.z >= bMax.z)
        {
            return false;
        }

        // Trigger colliders still report overlap for events, but do not physically resolve.
        if (a->isTrigger || b->isTrigger)
            return true;


        const float penetrationX = (glm::min)(aMax.x - bMin.x, bMax.x - aMin.x);
        const float penetrationY = (glm::min)(aMax.y - bMin.y, bMax.y - aMin.y);
        const float penetrationZ = (glm::min)(aMax.z - bMin.z, bMax.z - aMin.z);

        float penetration = penetrationX;
        glm::vec3 normal(1.0f, 0.0f, 0.0f);

        if (penetrationY < penetration)
        {
            penetration = penetrationY;
            normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        if (penetrationZ < penetration)
        {
            penetration = penetrationZ;
            normal = glm::vec3(0.0f, 0.0f, 1.0f);
        }

        const glm::vec3 aCenter = (aMin + aMax) * 0.5f;
        const glm::vec3 bCenter = (bMin + bMax) * 0.5f;

        if (glm::dot(bCenter - aCenter, normal) < 0.0f)
        {
            normal = -normal;
        }

        ResolvePenetration(a, b, normal, penetration);
        return true;
    }

    // 2D AABB (XY plane) collision test + optional physical resolution
    static bool CollisionStep2D(SquareCollider* a, SquareCollider* b)
    {
        if (!a || !b || !a->transform || !b->transform)
            return false;

        const glm::vec2 posA(a->transform->position.x, a->transform->position.y);
        const glm::vec2 posB(b->transform->position.x, b->transform->position.y);

        const glm::vec2 halfSizeA = glm::abs(a->size) * 0.5f;
        const glm::vec2 halfSizeB = glm::abs(b->size) * 0.5f;

        const glm::vec2 aMin = posA + a->center - halfSizeA;
        const glm::vec2 aMax = posA + a->center + halfSizeA;

        const glm::vec2 bMin = posB + b->center - halfSizeB;
        const glm::vec2 bMax = posB + b->center + halfSizeB;

        if (aMax.x <= bMin.x ||
            aMin.x >= bMax.x ||
            aMax.y <= bMin.y ||
            aMin.y >= bMax.y)
        {
            return false;
        }

        // Trigger colliders still report overlap for events, but do not physically resolve.
        if (a->isTrigger || b->isTrigger)
            return true;

        const float penetrationX = (glm::min)(aMax.x - bMin.x, bMax.x - aMin.x);
        const float penetrationY = (glm::min)(aMax.y - bMin.y, bMax.y - aMin.y);

        float penetration = penetrationX;
        glm::vec3 normal(1.0f, 0.0f, 0.0f);

        if (penetrationY < penetration)
        {
            penetration = penetrationY;
            normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        const glm::vec2 aCenter = (aMin + aMax) * 0.5f;
        const glm::vec2 bCenter = (bMin + bMax) * 0.5f;
        const glm::vec2 delta = bCenter - aCenter;

        if (glm::dot(delta, glm::vec2(normal.x, normal.y)) < 0.0f)
        {
            normal = -normal;
        }

        ResolvePenetration(a, b, normal, penetration);
        return true;
    }

    static void ResolvePenetration(
        Collider* a,
        Collider* b,
        const glm::vec3& normal,
        float penetration
    )
    {
        Rigidbody* rbA = a->owner
            ? a->owner->GetComponent<Rigidbody>()
            : nullptr;

        Rigidbody* rbB = b->owner
            ? b->owner->GetComponent<Rigidbody>()
            : nullptr;

        const bool canMoveA =
            rbA &&
            rbA->transform &&
            !rbA->isKinematic &&
            rbA->mass > 0.0f;

        const bool canMoveB =
            rbB &&
            rbB->transform &&
            !rbB->isKinematic &&
            rbB->mass > 0.0f;

        if (!canMoveA && !canMoveB)
            return;

        if (canMoveA && canMoveB)
        {
            const float inverseMassA = 1.0f / rbA->mass;
            const float inverseMassB = 1.0f / rbB->mass;
            const float totalInverseMass = inverseMassA + inverseMassB;

            const glm::vec3 correction = normal * penetration;
            const glm::vec3 correctionA = correction * (inverseMassA / totalInverseMass);
            const glm::vec3 correctionB = correction * (inverseMassB / totalInverseMass);

            rbA->transform->position -= correctionA;
            rbB->transform->position += correctionB;

            ResolveVelocity(rbA, rbB, normal);
            return;
        }

        if (canMoveA)
        {
            rbA->transform->position -= normal * penetration;
            RemoveVelocityIntoSurface(rbA, normal);
            return;
        }

        if (canMoveB)
        {
            rbB->transform->position += normal * penetration;
            RemoveVelocityIntoSurface(rbB, -normal);
            return;
        }
    }

    // ======================================================
    // Collision velocity resolution
    // ======================================================

    static void ResolveVelocity(
        Rigidbody* a,
        Rigidbody* b,
        const glm::vec3& normal)
    {
        glm::vec3 relativeVelocity =
            b->velocity - a->velocity;

        const float velocityAlongNormal =
            glm::dot(relativeVelocity, normal);

        // Already moving apart.
        if (velocityAlongNormal > 0.0f)
            return;

        const float inverseMassA =
            1.0f / a->mass;

        const float inverseMassB =
            1.0f / b->mass;

        const float restitution = 0.0f;

        const float impulseMagnitude =
            -(1.0f + restitution) *
            velocityAlongNormal /
            (inverseMassA + inverseMassB);

        const glm::vec3 impulse =
            impulseMagnitude * normal;

        a->velocity -=
            impulse * inverseMassA;

        b->velocity +=
            impulse * inverseMassB;
    }

    // ======================================================
    // Remove velocity going into a surface.
    // ======================================================

    static void RemoveVelocityIntoSurface(
        Rigidbody* body,
        const glm::vec3& normal)
    {
        const float velocityIntoSurface =
            glm::dot(body->velocity, normal);

        if (velocityIntoSurface < 0.0f)
        {
            body->velocity -=
                velocityIntoSurface * normal;
        }
    }
};
