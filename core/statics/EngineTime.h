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

    static void Update(float* lastFrameTime, float currentTime)
    {
        float currentFrameTime = static_cast<float>(currentTime);
        Time::frames++;
        Time::deltaTime = (currentFrameTime - *lastFrameTime) * Time::timeScale;
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
        deltaBuffer.push_back(Time::deltaTime / Time::timeScale);
        deltaSum += Time::deltaTime / Time::timeScale;

        if (Time::frames % 60 == 0)
            Time::FPS = (deltaBuffer.size() / deltaSum);
    }
};
