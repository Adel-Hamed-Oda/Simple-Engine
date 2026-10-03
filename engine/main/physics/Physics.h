#pragma once

#include <glm/glm.hpp>

#include "../../Config.h"
#include "../../core/EngineTime.h"
#include "../../components/__Components__.h"

class Physics
{
public:
    static void Update()
    {
        const float dt = glm::min(
            Time::deltaTime,
            CONFIG::PHYSICS::MAX_DELTA_TIME
        );

        // --------------------------------------------------
        // Integrate rigidbodies
        // --------------------------------------------------

        for (Rigidbody* body : Rigidbody::allRigidbodies)
        {
            if (!body || !body->transform || body->isKinematic)
                continue;

            PhysicsStep(body, dt);
        }

        // --------------------------------------------------
        // Collision detection + resolution
        // --------------------------------------------------

        for (size_t i = 0; i < Collider::allColliders.size(); ++i)
        {
            BoxCollider* a =
                dynamic_cast<BoxCollider*>(Collider::allColliders[i]);

            if (!a || !a->transform)
                continue;

            for (size_t j = i + 1; j < Collider::allColliders.size(); ++j)
            {
                BoxCollider* b =
                    dynamic_cast<BoxCollider*>(Collider::allColliders[j]);

                if (!b || !b->transform)
                    continue;

                CollisionStep(a, b);
            }
        }
    }

private:
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
                glm::max(damping, 0.0f);
        }

        body->transform->position +=
            body->velocity * dt;
    }

    // OH MY
    static void CollisionStep(
        BoxCollider* a,
        BoxCollider* b)
    {
        if (!a || !b)
            return;

        if (!a->transform || !b->transform)
            return;

        // --------------------------------------------------
        // Trigger colliders do not physically collide.
        // --------------------------------------------------

        if (a->isTrigger || b->isTrigger)
            return;

        // --------------------------------------------------
        // Calculate AABB bounds
        // --------------------------------------------------

        const glm::vec3 aMin =
            a->transform->position +
            a->center -
            a->size * 0.5f;

        const glm::vec3 aMax =
            a->transform->position +
            a->center +
            a->size * 0.5f;

        const glm::vec3 bMin =
            b->transform->position +
            b->center -
            b->size * 0.5f;

        const glm::vec3 bMax =
            b->transform->position +
            b->center +
            b->size * 0.5f;

        // --------------------------------------------------
        // Check for overlap
        // --------------------------------------------------

        if (aMax.x <= bMin.x ||
            aMin.x >= bMax.x ||
            aMax.y <= bMin.y ||
            aMin.y >= bMax.y ||
            aMax.z <= bMin.z ||
            aMin.z >= bMax.z)
        {
            return;
        }

        // --------------------------------------------------
        // Calculate penetration on each axis
        // --------------------------------------------------

        const float penetrationX =
            glm::min(aMax.x - bMin.x,
                bMax.x - aMin.x);

        const float penetrationY =
            glm::min(aMax.y - bMin.y,
                bMax.y - aMin.y);

        const float penetrationZ =
            glm::min(aMax.z - bMin.z,
                bMax.z - aMin.z);

        // --------------------------------------------------
        // Find the axis with the smallest penetration.
        //
        // This gives us the collision normal.
        // --------------------------------------------------

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

        // --------------------------------------------------
        // Make the normal point from A -> B
        // --------------------------------------------------

        const glm::vec3 aCenter =
            (aMin + aMax) * 0.5f;

        const glm::vec3 bCenter =
            (bMin + bMax) * 0.5f;

        if (glm::dot(bCenter - aCenter, normal) < 0.0f)
        {
            normal = -normal;
        }

        // --------------------------------------------------
        // Find rigidbodies
        // --------------------------------------------------

        Rigidbody* rbA = a->owner
            ? a->owner->GetComponent<Rigidbody>()
            : nullptr;

        Rigidbody* rbB = b->owner
            ? b->owner->GetComponent<Rigidbody>()
            : nullptr;

        // --------------------------------------------------
        // Determine whether each body can move
        // --------------------------------------------------

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

        // --------------------------------------------------
        // Neither object can move.
        // --------------------------------------------------

        if (!canMoveA && !canMoveB)
            return;

        // --------------------------------------------------
        // Both objects can move.
        //
        // Split the correction according to inverse mass.
        // --------------------------------------------------

        if (canMoveA && canMoveB)
        {
            const float inverseMassA =
                1.0f / rbA->mass;

            const float inverseMassB =
                1.0f / rbB->mass;

            const float totalInverseMass =
                inverseMassA + inverseMassB;

            const glm::vec3 correction =
                normal * penetration;

            const glm::vec3 correctionA =
                correction *
                (inverseMassA / totalInverseMass);

            const glm::vec3 correctionB =
                correction *
                (inverseMassB / totalInverseMass);

            rbA->transform->position -= correctionA;
            rbB->transform->position += correctionB;

            ResolveVelocity(
                rbA,
                rbB,
                normal
            );

            return;
        }

        // --------------------------------------------------
        // Only A can move.
        // --------------------------------------------------

        if (canMoveA)
        {
            rbA->transform->position -=
                normal * penetration;

            RemoveVelocityIntoSurface(
                rbA,
                normal
            );

            return;
        }

        // --------------------------------------------------
        // Only B can move.
        // --------------------------------------------------

        if (canMoveB)
        {
            rbB->transform->position +=
                normal * penetration;

            RemoveVelocityIntoSurface(
                rbB,
                -normal
            );

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