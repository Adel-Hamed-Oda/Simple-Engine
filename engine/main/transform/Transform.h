#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class GameObject;

class Transform
{
public:
    GameObject* owner = nullptr;

    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    Transform(GameObject* owner, glm::vec3 position = glm::vec3(0.0f), glm::vec3 rotation = glm::vec3(0.0f), glm::vec3 scale = glm::vec3(1.0f)) 
        : owner(owner), position(position), rotation(rotation), scale(scale) 
    {
    }

    glm::vec3 Forward() const
    {
        glm::mat4 rotationMatrix = GetRotationMatrix();

        // Local -Z
        return glm::normalize(
            glm::vec3(rotationMatrix[2]) * -1.0f
        );
    }

    glm::vec3 Right() const
    {
        glm::mat4 rotationMatrix = GetRotationMatrix();

        // Local +X
        return glm::normalize(
            glm::vec3(rotationMatrix[0])
        );
    }
    
    glm::vec3 Up() const
    {
        glm::mat4 rotationMatrix = GetRotationMatrix();

        // Local +Y
        return glm::normalize(
            glm::vec3(rotationMatrix[1])
        );
    }

    // Note: order is important here, won't be fixed until we start using quaternions
    glm::mat4 GetRotationMatrix() const
    {
        glm::mat4 rotationMatrix(1.0f);

        rotationMatrix = glm::rotate(
            rotationMatrix,
            glm::radians(rotation.y),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        rotationMatrix = glm::rotate(
            rotationMatrix,
            glm::radians(rotation.x),
            glm::vec3(1.0f, 0.0f, 0.0f)
        );
        
        rotationMatrix = glm::rotate(
            rotationMatrix,
            glm::radians(rotation.z),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );

        return rotationMatrix;
    }

    glm::mat4 GetModelMatrix() const
    {
        glm::mat4 model(1.0f);

        // Translation
        model = glm::translate(model, position);

        // Rotation
        model *= GetRotationMatrix();

        // Scale
        model = glm::scale(model, scale);

        return model;
    }
};