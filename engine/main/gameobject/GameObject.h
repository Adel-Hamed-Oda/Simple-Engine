#pragma once

#include <algorithm>
#include <concepts>
#include <memory>
#include <vector>

#include "../../components/Component.h"
#include "../transform/Transform.h"
#include "../../components/behaviour/Behaviour.h"

class GameObject
{
public:
    std::unique_ptr<Transform> transform;
	std::string name;

    GameObject(std::string name)
        : name(name), transform(std::make_unique<Transform>(this)) {}

    GameObject(std::string name, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
        : name(name), transform(std::make_unique<Transform>(this, position, rotation, scale)) {}

    template <typename... T>
        requires (std::derived_from<T, Component> && ...)
    GameObject(std::string name, std::unique_ptr<T>... components)
        : name(name), transform(std::make_unique<Transform>(this))
    {
        (AddComponent(std::move(components)), ...);
    }

    template <typename... T>
        requires (std::derived_from<T, Component> && ...)
    GameObject(std::string name, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, std::unique_ptr<T>... components)
        : name(name), transform(std::make_unique<Transform>(this, position, rotation, scale))
    {
        (AddComponent(std::move(components)), ...);
    }

    ~GameObject() = default;

    void Start()
    {
        for (const auto& component : components)
        {
            if (Behaviour* behaviour = dynamic_cast<Behaviour*>(component.get()))
            {
                behaviour->Start();
            }
        }
    }

    void Update()
    {
        for (const auto& component : components)
        {
            if (Behaviour* behaviour = dynamic_cast<Behaviour*>(component.get()))
            {
                behaviour->Update();
            }
        }
    }

    template <typename T>
        requires std::derived_from<T, Component>
    T* AddComponent()
    {
        return AddComponent(std::make_unique<T>());
    }

    template <typename T>
        requires std::derived_from<T, Component>
    T* AddComponent(std::unique_ptr<T> component)
    {
        component->SetOwner(this);
        component->SetTransform(transform.get());

        T* ptr = component.get();

        components.push_back(std::move(component));

        return ptr;
    }

    Component* RemoveComponent(Component* component)
    {
        auto it = std::find_if(
            components.begin(),
            components.end(),
            [component](const std::unique_ptr<Component>& comp)
            {
                return comp.get() == component;
            }
        );

        if (it != components.end())
        {
            components.erase(it);
            return component;
        }

        return nullptr;
    }

    template <typename T>
        requires std::derived_from<T, Component>
    T* GetComponent()
    {
        for (const auto& component : components)
        {
            if (T* t = dynamic_cast<T*>(component.get()))
            {
                return t;
            }
        }

        return nullptr;
    }

private:
    std::vector<std::unique_ptr<Component>> components;
};