#pragma once

class GameObject;
class Transform;

class Component
{
public:
	explicit Component() = default;
	explicit Component(GameObject* owner, Transform* transform)
		: owner(owner), transform(transform) {
	}
    virtual ~Component() = default;

    virtual void Start() {}
    virtual void Update() {}

    void SetOwner(GameObject* go) { owner = go; }
    void SetTransform(Transform* t) { transform = t; }

    GameObject* owner = nullptr;
    Transform* transform = nullptr;
};