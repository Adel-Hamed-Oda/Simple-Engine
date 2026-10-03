#pragma once

#include <vector>

namespace Time
{
    inline int frames = 0;
    inline float time = 0.0f;
    inline float deltaTime = 0.0f;
    inline float FPS = 0.0f;
    inline float immediateFPS = 0.0f;

    inline void Update(float* lastFrameTime, float currentTime)
    {
        float currentFrameTime = static_cast<float>(currentTime);
        Time::frames++;
        Time::deltaTime = currentFrameTime - *lastFrameTime;
        *lastFrameTime = currentFrameTime;
        Time::time += Time::deltaTime;
        if (Time::deltaTime > 0.0f)
            Time::immediateFPS = 1.0f / Time::deltaTime;
        else
            Time::immediateFPS = 0.0f;

        static std::vector<float> deltaBuffer;
        static float deltaSum = 0.0f;
        if (deltaBuffer.size() > 60)
        {
            deltaSum -= deltaBuffer.front();
            deltaBuffer.erase(deltaBuffer.begin());
        }
        deltaBuffer.push_back(Time::deltaTime);
        deltaSum += Time::deltaTime;

        if (Time::frames % 60 == 0)
            Time::FPS = deltaBuffer.size() / deltaSum;
    }
};
