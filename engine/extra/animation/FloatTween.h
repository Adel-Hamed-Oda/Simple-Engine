#pragma once

#include "../../core/__Core__.h"

#include "Tween.h"

class FloatTween : public Tween
{
public:
    FloatTween() : Tween() {}
    ~FloatTween() override = default;

    static FloatTween* Create(
        float* value,
        float start,
        float end,
        float duration,
        std::function<float(float)> easingFunction = nullptr,
        std::function<void()> onComplete = nullptr
    )
    {
        FloatTween* tween = new FloatTween();
        tween->value = value;
        tween->startValue = start;
        tween->endValue = end;
        tween->duration = duration;
        tween->easingFunction = easingFunction;
        tween->onComplete = onComplete;

        Tween::activeTweens.push_back(tween);

        return tween;
    }

    void Update() override
    {
        elapsedTime += Time::deltaTime;

        *value = startValue + (endValue - startValue) * GetT();

        if (Finished() && onComplete)
        {
            onComplete();
        }
    }

    const float GetT()
    {
        float t = std::clamp(elapsedTime / duration, 0.0f, 1.0f);

        if (easingFunction)
        {
            return easingFunction(t);
        }

        return t;
    }

    bool Finished() override
    {
        return elapsedTime >= duration;
    }

private:
    float* value;

    float startValue;
    float endValue;

    float elapsedTime = 0.0f;
    float duration;

    std::function<float(float)> easingFunction;
    std::function<void()> onComplete;
};