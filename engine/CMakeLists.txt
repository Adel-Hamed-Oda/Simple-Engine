cmake_minimum_required(VERSION 3.20)
project(CustomEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(external/glfw)
add_subdirectory(external/glm)

# 4. Create your Engine as a static library
add_library(Engine STATIC
    IDK.cpp
    Engine.h
    Config.h
)

# 5. Set up the Include Directories
# Expose the engine root so users can simply #include "Engine.h"
target_include_directories(Engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/components
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/scenes
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/external/stb
)

# 6. Find system OpenGL natively
find_package(OpenGL REQUIRED)

# 7. Link everything together
target_link_libraries(Engine PUBLIC glfw OpenGL::GL)