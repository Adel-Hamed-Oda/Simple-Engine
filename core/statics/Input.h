#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "PublicDomain.h"

class Input
{
public:
    inline static glm::vec2 MousePosition{ 0.0f, 0.0f };
    inline static glm::vec2 MouseDelta{ 0.0f, 0.0f };

    inline static bool CurrentKeys[GLFW_KEY_LAST + 1]{};
    inline static bool PreviousKeys[GLFW_KEY_LAST + 1]{};

    // ---------------------------------------------------------
    // Setup
    // ---------------------------------------------------------

    static void Setup()
    {
        if (!PublicDomain::CurrentWindow)
            return;

        glfwSetCursorPosCallback(
            PublicDomain::CurrentWindow->GetGLFWWindow(),
            [](GLFWwindow*, double xpos, double ypos)
            {
                glm::vec2 newPosition{
                    static_cast<float>(xpos),
                    static_cast<float>(ypos)
                };

                MouseDelta += newPosition - MousePosition;
                MousePosition = newPosition;
            }
        );
    }

    static void Update()
    {
        if (!PublicDomain::CurrentWindow)
            return;

        for (int key = 0; key <= GLFW_KEY_LAST; ++key)
        {
            PreviousKeys[key] = CurrentKeys[key];

            CurrentKeys[key] =
                glfwGetKey(PublicDomain::CurrentWindow->GetGLFWWindow(), key) == GLFW_PRESS;
        }
    }

    static void EndFrame()
    {
        MouseDelta = glm::vec2(0.0f);
    }

    // ---------------------------------------------------------
    // Keyboard
    // ---------------------------------------------------------

    static bool GetKey(int key)
    {
        if (key < 0 || key > GLFW_KEY_LAST)
            return false;

        return CurrentKeys[key];
    }

    static bool GetKeyDown(int key)
    {
        if (key < 0 || key > GLFW_KEY_LAST)
            return false;

        return CurrentKeys[key] && !PreviousKeys[key];
    }

    static bool GetKeyUp(int key)
    {
        if (key < 0 || key > GLFW_KEY_LAST)
            return false;

        return !CurrentKeys[key] && PreviousKeys[key];
    }

    // ---------------------------------------------------------
    // Mouse
    // ---------------------------------------------------------

    static bool GetMouseButtonDown(int button)
    {
        if (!PublicDomain::CurrentWindow)
            return false;

        return glfwGetMouseButton(
            PublicDomain::CurrentWindow->GetGLFWWindow(),
            button
        ) == GLFW_PRESS;
    }

    static bool GetMouseButtonUp(int button)
    {
        if (!PublicDomain::CurrentWindow)
            return false;

        return glfwGetMouseButton(
            PublicDomain::CurrentWindow->GetGLFWWindow(),
            button
        ) == GLFW_RELEASE;
    }
};