#pragma once

#include "../Component.h"

class Behaviour : public Component
{
public:
    Behaviour() = default;
    virtual ~Behaviour() = default;

    virtual void Start() {}
    virtual void Update() {}
};