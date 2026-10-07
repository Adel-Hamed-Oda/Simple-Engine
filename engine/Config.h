#pragma once

#include <glm/glm.hpp>

enum class FACE_CULLING_MODE
{
    NONE = 0,
    BACK = 1,
    FRONT = 2,
    FRONT_AND_BACK = 3
};

namespace CONFIG
{
    namespace ENGINE
    {
        constexpr const char* VERSION = "0.1.0";

        constexpr int CONTEXT_VERSION_MAJOR = 3;
        constexpr int CONTEXT_VERSION_MINOR = 3;

        constexpr bool LOAD_FIRST_SCENE = true;
        constexpr bool ENABLE_DEBUG = true;
    }

    namespace RENDERING
    {
        constexpr bool ENABLE_DEPTH_TEST = true;

        constexpr FACE_CULLING_MODE FACE_CULLING =
            FACE_CULLING_MODE::BACK;

        constexpr bool HIDE_CURSOR = true;
        constexpr bool ENABLE_VSYNC = true;
    }

    namespace LIGHTING
    {
        constexpr glm::vec4 BACKGROUND_COLOR =
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        // Must match MAX_LIGHTS in the .glsl files, there is probably a better way, but later
        constexpr int MAX_LIGHTS = 256;
    }

    namespace PHYSICS
    {
        constexpr glm::vec3 GRAVITY = glm::vec3(0.0f, -9.81f, 0.0f);
        constexpr float MAX_DELTA_TIME = 0.1f;
    }

    namespace WINDOW
    {
        constexpr const char* TITLE = "hello";

        constexpr int WIDTH = 1000;
        constexpr int HEIGHT = 1000;
    }

    namespace DIRECTORY
    {
        constexpr const char* ASSETS = "assets/";
        constexpr const char* SHADERS = "assets/";
        constexpr const char* TEXTURES = "assets/";
        constexpr const char* AUDIO = "assets/";
    }
}