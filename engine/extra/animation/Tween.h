#pragma once

#include "../../core/__Core__.h"

class Tween
{
public:
    inline static std::vector<Tween*> activeTweens;

    static void Update()
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

    virtual void Update() = 0;
    virtual bool Finished() = 0;
};