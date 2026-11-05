#pragma once

#include <vector>

class Time
{
public:
    inline static int frames = 0;
    inline static float time = 0.0f;
    inline static float deltaTime = 0.0f;
    inline static float FPS = 0.0f;
    inline static float immediateFPS = 0.0f;
    inline static float timeScale = 1.0f;

    static void SyncClock(float* lastFrameTime, float currentTime)
    {
        if (lastFrameTime)
        {
            *lastFrameTime = currentTime;
        }
        Time::deltaTime = 0.0f;
    }

    static void Update(float* lastFrameTime, float currentTime)
    {
        const float currentFrameTime = static_cast<float>(currentTime);
        const float unscaledDelta = (currentFrameTime > *lastFrameTime)
            ? (currentFrameTime - *lastFrameTime)
            : 0.0f;
        Time::frames++;
        Time::deltaTime = unscaledDelta * Time::timeScale;
        *lastFrameTime = currentFrameTime;
        Time::time += Time::deltaTime;
        if (Time::deltaTime > 0.0f)
            Time::immediateFPS = 1.0f / Time::deltaTime;
        else
            Time::immediateFPS = 0.0f;

        static std::vector<float> deltaBuffer;
        static float deltaSum = 0.0f;
        if (deltaBuffer.size() > 600)
        {
            deltaSum -= deltaBuffer.front();
            deltaBuffer.erase(deltaBuffer.begin());
        }
        deltaBuffer.push_back(unscaledDelta);
        deltaSum += unscaledDelta;

        if (Time::frames % 60 == 0 && deltaSum > 0.0f)
            Time::FPS = (static_cast<float>(deltaBuffer.size()) / deltaSum);
    }
};
