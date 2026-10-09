#pragma once

#include "../../core/__Core__.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <type_traits>
#include <vector>

class Tween
{
public:
    inline static std::vector<Tween*> activeTweens;

    // Works for int, short, float, double, and glm vectors as far as I can tell.
    template <typename T>
    static Tween* Create(
        T* value,
        const T& start,
        const T& end,
        float duration,
        std::function<float(float)> easingFunction = nullptr,
        std::function<void()> onComplete = nullptr
    )
    {
        Tween* tween = new Tween();
        tween->duration = duration;
        tween->easingFunction = std::move(easingFunction);
        tween->onComplete = std::move(onComplete);
        tween->apply = [value, start, end](float t)
        {
            *value = Lerp<T>(start, end, t);
        };

        activeTweens.push_back(tween);

        return tween;
    }

    static void UpdateAll()
    {
        for (size_t i = 0; i < activeTweens.size(); )
        {
            Tween* tween = activeTweens[i];

            tween->Update();

            if (tween->Finished())
            {
                activeTweens.erase(activeTweens.begin() + i);
                delete tween;
            }
            else
            {
                ++i;
            }
        }
    }

    float GetT() const
    {
        float t = duration > 0.0f ? std::clamp(elapsedTime / duration, 0.0f, 1.0f) : 1.0f;

        if (easingFunction)
        {
            return easingFunction(t);
        }

        return t;
    }

    bool Finished() const
    {
        return elapsedTime >= duration;
    }

private:
    Tween() = default;

    void Update()
    {
        elapsedTime += Time::deltaTime;

        apply(GetT());

        if (Finished() && onComplete)
        {
            onComplete();
        }
    }

    template <typename T>
    static T Lerp(const T& a, const T& b, float t)
    {
        if constexpr (std::is_arithmetic_v<T>)
        {
            using Real = std::conditional_t<std::is_floating_point_v<T>, T, double>;

            Real ra = static_cast<Real>(a);
            Real rb = static_cast<Real>(b);
            Real r = ra + (rb - ra) * static_cast<Real>(t);

            if constexpr (std::is_floating_point_v<T>)
            {
                return r;
            }
            else
            {
                return static_cast<T>(std::round(r));
            }
        }
        else
        {
            using Scalar = typename T::value_type;
            using Real = std::conditional_t<std::is_floating_point_v<Scalar>, Scalar, double>;
            using RealVec = glm::vec<T::length(), Real, glm::defaultp>;

            RealVec ra(a);
            RealVec rb(b);
            RealVec r = ra + (rb - ra) * static_cast<Real>(t);

            if constexpr (std::is_floating_point_v<Scalar>)
            {
                return T(r);
            }
            else
            {
                return T(glm::round(r));
            }
        }
    }

    float elapsedTime = 0.0f;
    float duration = 0.0f;

    std::function<void(float)> apply;
    std::function<float(float)> easingFunction;
    std::function<void()> onComplete;
};