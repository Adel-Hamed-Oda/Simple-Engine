#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

#include "Scene.h"

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

        // Remove the currently loaded scene.
        UnloadScene();

        // Create the requested scene.
        currentScene = it->second();

        if (!currentScene)
        {
            return false;
        }

        currentScene->Setup();
        currentScene->Start();

        return true;
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

        currentScene->Unload();
        currentScene.reset();
    }

    static void Update() { if (currentScene) currentScene->Update(); }
    static Scene* GetCurrentScene() { return currentScene.get(); }
    static bool HasScene(const std::string& name) { return scenes.contains(name); }


private:

    inline static std::unique_ptr<Scene> currentScene = nullptr;

    inline static std::unordered_map<
        std::string,
        std::function<std::unique_ptr<Scene>()>
    > scenes;
};