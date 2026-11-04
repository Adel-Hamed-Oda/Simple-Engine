#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

#include "Scene.h"
#include "../extra/animation/Tween.h"
#include "../extra/timer/Timer.h"

class SceneManager
{
public:

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

        // If called while a scene is currently running its Update(), defer the
        // scene transition to the end of the update pass. This prevents deleting
        // the active Scene and its GameObjects while their code is still executing
        // on the call stack (preventing read access violations and dangling pointers).
        if (isUpdating)
        {
            pendingSceneName = name;
            return true;
        }

        return ExecuteSceneChange(name);
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

        // Clear all active tweens and timers before unloading objects
        // so no callbacks or value-lerp writes target freed transforms or entities.
        Tween::ClearAll();
        Timer::ClearAll();

        currentScene->Unload();
        currentScene.reset();
    }

    static void Update()
    {
        if (!pendingSceneName.empty())
        {
            std::string target = pendingSceneName;
            pendingSceneName.clear();
            ExecuteSceneChange(target);
        }

        if (currentScene)
        {
            isUpdating = true;
            currentScene->Update();
            isUpdating = false;
        }

        if (!pendingSceneName.empty())
        {
            std::string target = pendingSceneName;
            pendingSceneName.clear();
            ExecuteSceneChange(target);
        }
    }

    static Scene* GetCurrentScene() { return currentScene.get(); }
    static bool HasScene(const std::string& name) { return scenes.contains(name); }

private:
    static bool ExecuteSceneChange(const std::string& name)
    {
        auto it = scenes.find(name);
        if (it == scenes.end())
        {
            return false;
        }

        UnloadScene();

        currentScene = it->second();
        if (!currentScene)
        {
            return false;
        }

        currentScene->Setup();
        currentScene->Start();

        return true;
    }

    inline static bool isUpdating = false;
    inline static std::string pendingSceneName = "";

    inline static std::unique_ptr<Scene> currentScene = nullptr;

    inline static std::unordered_map<
        std::string,
        std::function<std::unique_ptr<Scene>()>
    > scenes;
};
