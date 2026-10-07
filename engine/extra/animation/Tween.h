#pragma once

#include "../../core/__Core__.h"

class Tween
{
public:
    inline static std::vector<Tween*> activeTweens;

    Tween() = default;
    virtual ~Tween() = default;

    static void UpdateAll()
    {
        for (auto it = activeTweens.begin(); it != activeTweens.end(); )
        {
            Tween* tween = *it;

            tween->Update();

            if (tween->Finished())
            {
                it = activeTweens.erase(it);
                delete tween;
            }
            else
            {
                ++it;
            }
        }
    }

    virtual void Update() {}
    virtual bool Finished() { return true; }
};