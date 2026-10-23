#pragma once

#include <string>

#include "../../Config.h"
#include "../rendering/Window.h"

namespace PublicDomain
{
    namespace Rendering
    {
        inline bool EnableDepthTest = CONFIG::RENDERING::ENABLE_DEPTH_TEST;

		inline FACE_CULLING_MODE FaceCulling = CONFIG::RENDERING::FACE_CULLING;
    }

    namespace Lighting
    {
        inline glm::vec4 BackgroundColor = CONFIG::LIGHTING::BACKGROUND_COLOR;
    }

    namespace Physics
    {
		inline glm::vec3 Gravity = CONFIG::PHYSICS::GRAVITY;
    }
    
    inline Window* CurrentWindow = nullptr;

    inline void ChangeCursorVisibility(bool visible)
    {
        if (CurrentWindow)
        {
            if (visible)
                glfwSetInputMode(CurrentWindow->GetGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            else
                glfwSetInputMode(CurrentWindow->GetGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}