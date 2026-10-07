#pragma once

#include <fstream>
#include <string>
#include <numeric>
#include <iostream>

#include "components/__Components__.h"
#include "core/__Core__.h"
#include "scenes/__Scenes__.h"
#include "extra/__Extra__.h"
#include "Config.h"

class Engine
{
public:
    static int run()
    {
        if (!GLFW()) {
            std::cout << "GLFW initialization failed" << std::endl;
            return 0;
        }
        if (!CreatePrimaryWindow()) {
            std::cout << "Primary Window creation failed" << std::endl;
            return 0;
        }
        if (!GLAD()) {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return 0;
        }

        GenericSetup();

        return MainLoop();
    }

private:
    static bool GLFW()
    {
        if (!glfwInit())
        {
            glfwTerminate();
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, CONFIG::ENGINE::CONTEXT_VERSION_MAJOR);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, CONFIG::ENGINE::CONTEXT_VERSION_MINOR);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        return true;
    }

    static bool CreatePrimaryWindow()
    {
		PublicDomain::CurrentWindow = new Window(CONFIG::WINDOW::WIDTH, CONFIG::WINDOW::HEIGHT, CONFIG::WINDOW::TITLE);
        if (PublicDomain::CurrentWindow == nullptr)
        {
            glfwTerminate();
            return false;
        }
        glfwMakeContextCurrent(PublicDomain::CurrentWindow->GetGLFWWindow());
        return true;
    }

    static bool GLAD()
    {
        return gladLoadGL((GLADloadfunc)glfwGetProcAddress);
    }

    static void GenericSetup()
    {
        glfwSetFramebufferSizeCallback(PublicDomain::CurrentWindow->GetGLFWWindow(), framebuffer_size_callback);

        glfwSetInputMode(PublicDomain::CurrentWindow->GetGLFWWindow(), GLFW_CURSOR, CONFIG::RENDERING::HIDE_CURSOR ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL); // IDK where to put this tbh

        Rendering::SetupRendering();

        Input::Setup();

        if (!CONFIG::ENGINE::ENABLE_DEBUG)
        {
            Debug::enabled = false;
        }

        if (CONFIG::ENGINE::LOAD_FIRST_SCENE)
        {
            SceneManager::LoadScene(0);
        }
    }

    static bool MainLoop()
    {
        while (!glfwWindowShouldClose(PublicDomain::CurrentWindow->GetGLFWWindow()))
        {
            glfwPollEvents();

            glClearColor(
                PublicDomain::Lighting::BackgroundColor.r,
                PublicDomain::Lighting::BackgroundColor.g,
                PublicDomain::Lighting::BackgroundColor.b,
                PublicDomain::Lighting::BackgroundColor.a
            );

            glClear(
                GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT
            );

            static float lastFrameTime = 0.0f;
            Time::Update(&lastFrameTime, static_cast<float>(glfwGetTime()));

            Input::Update();
            Physics::Update();
            SceneManager::Update();
            Tween::Update();

            Scene* scene = SceneManager::GetCurrentScene();
            if (scene)
            {
                for (const auto& obj : scene->GetGameObjects())
                {
                    Rendering::RenderGameObject(obj.get());
                }
            }

            Debug::EndFrame();
            Input::EndFrame();

            glfwSwapBuffers(PublicDomain::CurrentWindow->GetGLFWWindow());
        }

        SceneManager::UnloadScene();
        glfwTerminate();

        return true;
    }

    static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
    {
        glViewport(0, 0, width, height);
    }
};