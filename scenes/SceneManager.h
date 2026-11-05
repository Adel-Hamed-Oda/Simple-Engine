#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

#include "../Config.h"
#include "../core/audio/AudioSource.h"
#include "Scene.h"
#include "../extra/animation/Tween.h"
#include "../extra/timer/Timer.h"

class SceneManager
{
public:
    inline static bool pauseOnSceneLoad = CONFIG::ENGINE::PAUSE_ON_SCENE_LOAD;

    template<typename T>
    static void RegisterScene(const std::string& name)
    {
        scenes[name] = []()
            {
                return std::make_unique<T>();
            };
    }

    static bool LoadScene(const std::string& name)
    {
        auto it = scenes.find(name);

        if (it == scenes.end())
        {
            return false;
        }

        // If called while a frame update or scene load is active, defer the
        // scene transition so active scenes, colliders, tweens, and timers are
        // never destroyed while their code is still executing on the call stack.
        if (isUpdating || isLoading)
        {
            pendingSceneName = name;
            return true;
        }

        const bool loaded = ExecuteSceneChange(name);
        if (loaded && !pendingSceneName.empty())
        {
            ProcessPendingSceneLoad();
        }
        return loaded;
    }

    static bool LoadScene(const int index)
    {
        if (index < 0 || index >= static_cast<int>(scenes.size()))
        {
            return false;
        }
        auto it = std::next(scenes.begin(), index);
        return LoadScene(it->first);
    }

    static void UnloadScene()
    {
        if (!currentScene)
            return;

        // Clear all active tweens, timers, and playing sounds before unloading
        // objects so no callbacks, lerps, or looping tracks persist across scenes.
        Tween::ClearAll();
        Timer::ClearAll();
        AudioSource::StopAll();

        currentScene->Unload();
        currentScene.reset();
    }

    static void ProcessPendingSceneLoad()
    {
        int redirects = 0;
        while (!pendingSceneName.empty() && redirects < 8)
        {
            std::string target = pendingSceneName;
            pendingSceneName.clear();
            ExecuteSceneChange(target);
            ++redirects;
        }
    }

    static void Update()
    {
        if (pauseOnSceneLoad && !pendingSceneName.empty())
        {
            return;
        }

        if (!pendingSceneName.empty())
        {
            ProcessPendingSceneLoad();
        }

        if (currentScene)
        {
            const bool wasUpdating = isUpdating;
            isUpdating = true;
            currentScene->Update();
            isUpdating = wasUpdating;
        }

        if (!pauseOnSceneLoad && !pendingSceneName.empty())
        {
            ProcessPendingSceneLoad();
        }
    }

    static Scene* GetCurrentScene() { return currentScene.get(); }
    static bool HasScene(const std::string& name) { return scenes.contains(name); }
    static bool IsLoading() { return isLoading; }
    static bool HasPendingSceneLoad() { return !pendingSceneName.empty(); }
    static bool HasSceneJustLoaded() { return sceneJustLoaded; }
    static void ConsumeSceneJustLoaded() { sceneJustLoaded = false; }
    static void SetDeferSceneLoads(bool defer) { isUpdating = defer; }

private:
    static bool ExecuteSceneChange(const std::string& name)
    {
        auto it = scenes.find(name);
        if (it == scenes.end())
        {
            return false;
        }

        isLoading = true;

        UnloadScene();

        currentScene = it->second();
        if (!currentScene)
        {
            isLoading = false;
            return false;
        }

        currentScene->Setup();
        currentScene->Start();

        isLoading = false;
        sceneJustLoaded = true;

        return true;
    }

    inline static bool isUpdating = false;
    inline static bool isLoading = false;
    inline static bool sceneJustLoaded = false;
    inline static std::string pendingSceneName = "";

    inline static std::unique_ptr<Scene> currentScene = nullptr;

    inline static std::unordered_map<
        std::string,
        std::function<std::unique_ptr<Scene>()>
    > scenes;
};
