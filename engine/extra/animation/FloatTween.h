#pragma once

#include "../../core/__Core__.h"

#include "Tween.h"

class FloatTween : public Tween
{
public:
    FloatTween(float* value, float start, float end, float duration, std::function<float(float)> easingFunction = nullptr, std::function<void()> onComplete = nullptr)
        : value(value), startValue(start), endValue(end), duration(duration), easingFunction(easingFunction), onComplete(onComplete) 
    {
        activeTweens.push_back(this);
    }
    ~FloatTween() = default;

    void Update() override
    {
        while (elapsedTime < duration)
        {
            *value = startValue + (endValue - startValue) * GetT();

            elapsedTime += Time::deltaTime;
        }

        if (onComplete)
        {
            onComplete();
        }
    }

    bool Finished() override
    {
        return elapsedTime >= duration;
    }

private:
    float* value;
    float startValue;
    float endValue;
    float elapsedTime = 0;
    float duration;

    std::function<float(float)> easingFunction;
    std::function<void()> onComplete;

    float GetT() 
    {
        if (easingFunction)
        {
            return easingFunction(elapsedTime / duration);
        }
        return elapsedTime / duration;
    }
};