#pragma once

#include "../../core/__Core__.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <type_traits>
#include <vector>

class Timer
{
public:
    inline static std::vector<Timer*> activeTimers;

    static Timer* Create(
        float duration,
        bool repeating = false,
        std::function<void()> onComplete = nullptr
    )
    {
        Timer* timer = new Timer();
        timer->duration = duration;
        timer->repeating = repeating;
        timer->onComplete = std::move(onComplete);

        activeTimers.push_back(timer);

        return timer;
    }

    static void UpdateAll()
    {
        for (size_t i = 0; i < activeTimers.size(); )
        {
            Timer* timer = activeTimers[i];

            timer->Update();

            if (timer->Finished())
            {
                activeTimers.erase(activeTimers.begin() + i);
                delete timer;
            }
            else
            {
                ++i;
            }
        }
    }

    const float GetElapsedTime() const
    {
        return elapsedTime;
    }
    const float GetProgress() const
    {
        return std::clamp(elapsedTime / duration, 0.0f, 1.0f);
    }
    const float GetDuration() const
    {
        return duration;
    }

    bool Finished() const
    {
        return elapsedTime >= duration;
    }

private:
    Timer() = default;

    void Update()
    {
        elapsedTime += Time::deltaTime;

        if (Finished() && onComplete)
        {
            onComplete();
            if (repeating)
            {
                // I likey
                elapsedTime -= duration;
            }
        }
    }

    float elapsedTime = 0.0f;
    float duration = 0.0f;

    bool repeating = false;

    std::function<void()> onComplete;
};