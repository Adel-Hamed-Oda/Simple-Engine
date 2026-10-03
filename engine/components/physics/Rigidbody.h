#pragma once

#include <vector>
#include <algorithm>

#include <glm/glm.hpp>

#include "../Component.h"
#include "../../core/transform/Transform.h"

class Rigidbody : public Component
{
public:
    glm::vec3 velocity = glm::vec3(0.0f);

    float mass;
    float gravityScale;
    float drag;

    bool useGravity = true;
    bool isKinematic = false;  // kinematic bodies are skipped entirely by Physics::Update - move them by hand

    Rigidbody(float mass = 1.0f, float gravityScale = 1.0f, float drag = 0.0f, bool isKinematic = false, bool useGravity = true)
        : mass(mass), gravityScale(gravityScale), drag(drag), isKinematic(isKinematic), useGravity(useGravity)
    {
        allRigidbodies.push_back(this);
    }
    ~Rigidbody() override
    {
        allRigidbodies.erase(
            std::remove(allRigidbodies.begin(), allRigidbodies.end(), this),
            allRigidbodies.end()
        );
    }

    // A continuous force (thrust, wind...). Accumulates until Physics::Update()
    // integrates it this frame, so it's framerate-independent - call it every
    // frame you want it applied (e.g. from a Behaviour::Update()).
    void AddForce(const glm::vec3& force)
    {
        forceAccumulator += force;
    }

    // An instantaneous change in momentum (a jump, an explosion). Applied
    // directly to velocity, no deltaTime involved - call it once, not per frame.
    void AddImpulse(const glm::vec3& impulse)
    {
        if (mass <= 0.0f) return;
        velocity += impulse / mass;
    }

    // Physics::Update() calls this once per body per frame to drain the
    // accumulator; you shouldn't need to call it yourself.
    glm::vec3 ConsumeForces()
    {
        const glm::vec3 force = forceAccumulator;
        forceAccumulator = glm::vec3(0.0f);
        return force;
    }

    inline static std::vector<Rigidbody*> allRigidbodies;

private:
    glm::vec3 forceAccumulator = glm::vec3(0.0f);
};