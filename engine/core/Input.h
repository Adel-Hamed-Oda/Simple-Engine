#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "../core/PublicDomain.h"

namespace Input
{
    // ---------------------------------------------------------
    // Mouse
    // ---------------------------------------------------------

    inline glm::vec2 MousePosition{ 0.0f, 0.0f };
    inline glm::vec2 MouseDelta{ 0.0f, 0.0f };

    // ---------------------------------------------------------
    // Keyboard
    // ---------------------------------------------------------

    inline bool CurrentKeys[GLFW_KEY_LAST + 1]{};
    inline bool PreviousKeys[GLFW_KEY_LAST + 1]{};

    // ---------------------------------------------------------
    // Setup
    // ---------------------------------------------------------

    inline void Setup()
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

    inline void Update()
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

    inline void EndFrame()
    {
        MouseDelta = glm::vec2(0.0f);
    }

    // ---------------------------------------------------------
    // Keyboard
    // ---------------------------------------------------------

    inline bool GetKey(int key)
    {
        if (key < 0 || key > GLFW_KEY_LAST)
            return false;

        return CurrentKeys[key];
    }

    inline bool GetKeyDown(int key)
    {
        if (key < 0 || key > GLFW_KEY_LAST)
            return false;

        return CurrentKeys[key] && !PreviousKeys[key];
    }

    inline bool GetKeyUp(int key)
    {
        if (key < 0 || key > GLFW_KEY_LAST)
            return false;

        return !CurrentKeys[key] && PreviousKeys[key];
    }

    // ---------------------------------------------------------
    // Mouse buttons
    // ---------------------------------------------------------

    inline bool GetMouseButtonDown(int button)
    {
        if (!PublicDomain::CurrentWindow)
            return false;

        return glfwGetMouseButton(
            PublicDomain::CurrentWindow->GetGLFWWindow(),
            button
        ) == GLFW_PRESS;
    }

    inline bool GetMouseButtonUp(int button)
    {
        if (!PublicDomain::CurrentWindow)
            return false;

        return glfwGetMouseButton(
            PublicDomain::CurrentWindow->GetGLFWWindow(),
            button
        ) == GLFW_RELEASE;
    }
}