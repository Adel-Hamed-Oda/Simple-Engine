#pragma once

#include "../../core/__Core__.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

class Timer
{
public:
    inline static std::vector<Timer*> activeTimers;

    explicit Timer(
        float duration,
        bool repeating = false,
        std::function<void()> onComplete = nullptr
    )
        : duration(duration),
          repeating(repeating),
          onComplete(std::move(onComplete))
    {
        activeTimers.push_back(this);
    }

    // Overload: any callable plus the arguments to call it with
    template <typename Func, typename Arg, typename... Args>
    Timer(
        float duration,
        bool repeating,
        Func&& func,
        Arg&& arg,
        Args&&... args
    )
        : Timer(
            duration,
            repeating,
            std::function<void()>(
                std::bind(
                    std::forward<Func>(func),
                    std::forward<Arg>(arg),
                    std::forward<Args>(args)...
                )
            )
        )
    {
    }

    ~Timer()
    {
        activeTimers.erase(
            std::remove(activeTimers.begin(), activeTimers.end(), this),
            activeTimers.end()
        );
    }

    static void UpdateAll()
    {
        for (size_t i = 0; i < activeTimers.size(); )
        {
            Timer* timer = activeTimers[i];

            timer->elapsedTime += Time::deltaTime;

            if (timer->Finished())
            {
                auto callback = timer->onComplete;
                if (timer->repeating)
                {
                    // I likey
                    timer->elapsedTime -= timer->duration;
                    ++i;
                }
                else
                {
                    delete timer;
                }

                if (callback)
                {
                    callback();
                }
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
    float elapsedTime = 0.0f;
    float duration = 0.0f;

    bool repeating = false;

    std::function<void()> onComplete;
};
