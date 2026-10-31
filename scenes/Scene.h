#pragma once

#include <glm/glm.hpp>
#include <algorithm>
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
        for (size_t i = 0; i < gameObjects.size(); ++i)
        {
            if (gameObjects[i])
                gameObjects[i]->Start();
        }
    }

    virtual void Update()
    {
        // Snapshot current object pointers so Instantiate/Destroy inside Update()
        // cannot invalidate vector iterators.
        std::vector<GameObject*> snapshot;
        snapshot.reserve(gameObjects.size());
        for (const auto& obj : gameObjects)
        {
            if (obj)
                snapshot.push_back(obj.get());
        }

        for (GameObject* obj : snapshot)
        {
            auto it = std::find_if(
                gameObjects.begin(),
                gameObjects.end(),
                [obj](const std::unique_ptr<GameObject>& owned)
                {
                    return owned.get() == obj;
                }
            );

            if (it != gameObjects.end() && *it)
            {
                (*it)->Update();
            }
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

    GameObject* FindFirstGameObjectByName(const std::string& name)
    {
        for (const auto& obj : gameObjects)
        {
            if (obj->name == name)
            {
                return obj.get();
            }
        }
        return nullptr;
    }

    std::vector<GameObject*> FindGameObjectsByName(const std::string& name)
    {
        std::vector<GameObject*> outObjects;
        for (const auto& obj : gameObjects)
        {
            if (obj->name == name)
            {
                outObjects.push_back(obj.get());
            }
        }
        return outObjects;
    }

    bool Destroy(GameObject* gameObject)
    {
        auto it = std::find_if(
            gameObjects.begin(),
            gameObjects.end(),
            [gameObject](const std::unique_ptr<GameObject>& obj)
            {
                return obj.get() == gameObject;
            }
        );

        if (it != gameObjects.end())
        {
            gameObjects.erase(it);
            return true;
        }

        return false;
    }

protected:
    std::vector<std::unique_ptr<GameObject>> gameObjects;
};
