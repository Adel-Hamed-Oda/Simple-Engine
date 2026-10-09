#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "../core/gameobject/GameObject.h"

class Scene
{
public:
    Scene() = default;
    virtual ~Scene() = default;

    virtual void Setup() 
    {

    }

    virtual void Start()
    {
        for (const auto& obj : gameObjects)
        {
            obj->Start();
        }
    }

    virtual void Update()
    {
        for (const auto& obj : gameObjects)
        {
            obj->Update();
        }
    }

    virtual void Unload()
    {
        gameObjects.clear();
    }

    const std::vector<std::unique_ptr<GameObject>>& GetGameObjects() const
    {
        return gameObjects;
    }

    GameObject* Instantiate(GameObject* gameObject)
    {
        gameObjects.emplace_back(gameObject);
        return gameObject;
    }

protected:
    std::vector<std::unique_ptr<GameObject>> gameObjects;
};