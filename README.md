# Simple Engine User Guide

This document explains how to use Simple Engine from an application that includes
the engine as a submodule. It covers the runtime API, architecture conventions,
and CMake integration for consuming projects.

## Overview

Simple Engine is a C++20, OpenGL 3.3 Core profile engine featuring:

- **Window & Context:** GLFW window management and GLAD OpenGL 3.3 Core loading
- **Lifecycle:** Multi-phase engine lifecycle (`Engine::setup()`, `Engine::run()`, `Engine::terminate()`)
- **Scene & Entity System:** Scene management with `GameObject` hierarchies and extensible `Behaviour` components
- **3D Rendering:** `MeshRenderer` with customizable shaders, materials (`DefaultMaterial`), textures, and multi-light support (`LightSource`)
- **2D Screen UI:** Normalized screen-space `ImageRenderer` (`ImageMaterial`) and interactive `Button` components
- **Text Rendering:** FreeType-powered text rendering in both 3D world space (`WorldTextRenderer`) and 2D screen space (`ScreenTextRenderer`) via `TextMaterial`
- **Model & Texture Loading:** Assimp-based 3D model importing (`ModelLoader`), STB image loading, and sprite sheet / atlas slicing (`Texture::CreateTextureMap`)
- **Direct Pointer Architecture:** Simple, predictable raw pointer ownership (`*`) without hidden string registries or lookup maps
- **Physics:** Basic `Rigidbody` numerical integration and axis-aligned `BoxCollider` detection and resolution
- **Audio:** Multi-channel audio playback powered by miniaudio (`AudioSource`)
- **Animation & Timing:** Frame timing (`Time`), heap-managed timers (`Timer`), and property tweens (`Tween`)
- **Diagnostics:** Built-in console logging (`Debug::Log`)

---

## Setup and Minimal Application

### 1. Project Directory & CMakeLists.txt

Create a folder for your project (e.g. `TestOpenGLGame`). Inside the root folder, create a `CMakeLists.txt` file:

```cmake
# Name shouldn't contain spaces
set(APPLICATION_NAME "TestOpenGLGame")

cmake_minimum_required(VERSION 3.10)
project(${APPLICATION_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(engine)

find_package(OpenGL REQUIRED)

add_executable(${APPLICATION_NAME}
    src/Application.cpp
)

target_link_libraries(${APPLICATION_NAME} PRIVATE Engine)

# Delete outdated assets and copy the fresh assets folder next to the executable on every build
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET ${APPLICATION_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -rf
                "$<TARGET_FILE_DIR:${APPLICATION_NAME}>/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/assets"
                "$<TARGET_FILE_DIR:${APPLICATION_NAME}>/assets"
        COMMENT "Removing old assets and copying fresh assets folder to output directory..."
    )
endif()
```

### 2. Adding the Submodule

In your project root directory, add the engine as a git submodule:

```bash
git submodule add "https://github.com/Adel-Hamed-Oda/Simple-Engine.git" engine
```

### 3. Application Entry Point

Create `src/Application.cpp`. The engine requires calling `Engine::setup()` first to initialize GLFW, GLAD, the primary window, and engine subsystems, followed by registering scenes, and finally invoking `Engine::run()`:

```cpp
#include <Engine.h>

class MainScene : public Scene
{
public:
    void Setup() override
    {
        // Construct game objects, materials, and components here
        auto* cameraObj = Instantiate(new GameObject("MainCamera", glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f), glm::vec3(1.0f)));
        auto* camera = cameraObj->AddComponent<Camera>(ProjectionType::Perspective);
        camera->fov = 60.0f;
    }
};

int main()
{
    // 1. Initialize GLFW, window, GLAD OpenGL context, audio, and subsystems
    if (!Engine::setup())
    {
        return -1;
    }

    // 2. Register scenes and select the startup scene
    SceneManager::RegisterScene<MainScene>("main");
    SceneManager::LoadScene("main");

    // 3. Start the main game loop
    return Engine::run();
}
```

To shut down the engine gracefully from any scene or button callback:
```cpp
Engine::terminate();
```

---

## Asset Path Convention

Asset loading does not prepend hardcoded directory prefixes. All asset paths (shaders, textures, models, audio, fonts) are resolved directly relative to the executable's working directory.

When using the standard CMake configuration, assets are copied into an `assets/` folder next to the executable. Pass relative paths starting with `"assets/..."`:

```cpp
Shader* shader = new Shader("assets/shaders/Image.vert", "assets/shaders/Image.frag");
Texture* texture = new Texture("assets/textures/player.png");
Mesh* mesh = ModelLoader::LoadMesh("assets/models/character.obj");
AudioSource sound("assets/audio/click.wav");
```

---

## Scenes and GameObjects

### Scene Lifecycle

`Scene` manages the lifetime of its active `GameObject` instances:

1. `Setup()`: Invoked when the scene is loaded. Construct scene-specific resources (shaders, textures, materials) and instantiate game objects here.
2. `Start()`: Called once on all components before the first frame update.
3. `Update()`: Called every frame; dispatches updates to all active components and behaviours.
4. `Unload()`: Called when switching scenes or shutting down. Clears game objects; free scene-owned resources here.

### Instantiating Objects

Use `Instantiate()` to transfer ownership of a `GameObject` pointer to the active scene:

```cpp
auto* player = Instantiate(new GameObject(
    "Player",
    glm::vec3(0.0f, 0.0f, 0.0f),  // position
    glm::vec3(0.0f, 0.0f, 0.0f),  // rotation (pitch, yaw, roll in degrees)
    glm::vec3(1.0f, 1.0f, 1.0f)   // scale
));
```

You can also construct game objects with components directly using variadic arguments:

```cpp
auto* buttonObj = Instantiate(new GameObject(
    "PlayButton",
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f),
    glm::vec3(0.4f, 0.2f, 1.0f),
    std::make_unique<ImageRenderer>(buttonMaterial),
    std::make_unique<Button>([]() {
        Debug::Log("Play clicked!");
    })
));
```

### Scene Management

```cpp
SceneManager::RegisterScene<MainMenuScene>("mainmenu");
SceneManager::RegisterScene<GameplayScene>("gameplay");

SceneManager::LoadScene("gameplay");
SceneManager::HasScene("mainmenu");
Scene* current = SceneManager::GetCurrentScene();
SceneManager::UnloadScene();
```

---

## Components and Behaviours

Every `GameObject` possesses a `Transform`. Additional components derive from `Component`.

### Adding and Retrieving Components

```cpp
auto* cameraObj = Instantiate(new GameObject("Camera"));
Camera* camera = cameraObj->AddComponent<Camera>(ProjectionType::Perspective);

Camera* sameCamera = cameraObj->GetComponent<Camera>();
```

### Custom Behaviours

Derive from `Behaviour` to implement gameplay logic:

```cpp
class PlayerController : public Behaviour
{
public:
    float speed = 5.0f;

    void Start() override
    {
        Debug::Log("Player initialized");
    }

    void Update() override
    {
        if (Input::GetKey(GLFW_KEY_W))
            transform->position += transform->Forward() * speed * Time::deltaTime;

        if (Input::GetKeyDown(GLFW_KEY_SPACE))
            Debug::Log("Jump triggered");
    }
};
```

---

## Transforms

The `Transform` component manages spatial position, orientation, and hierarchy:

```cpp
glm::vec3 position;  // World or local position
glm::vec3 rotation;  // Euler angles in degrees (applied Y -> X -> Z)
glm::vec3 scale;     // Scale factors
Transform* parent;   // Optional parent transform
```

Directional vectors and matrix helpers:
```cpp
glm::vec3 fwd   = transform->Forward();  // Local -Z direction in world space
glm::vec3 right = transform->Right();    // Local +X direction in world space
glm::vec3 up    = transform->Up();       // Local +Y direction in world space

glm::mat4 model = transform->GetModelMatrix();
```

---

## Rendering Pipeline

The rendering system separates rendering into three organized passes:

1. **World-Space Meshes:** Opaque, lit 3D geometry rendered using `MeshRenderer` with depth testing.
2. **World-Space Text:** 3D font text rendered via `WorldTextRenderer`, depth-tested against the 3D scene and sorted by `zOrder`.
3. **Screen-Space UI:** 2D UI elements (`ImageRenderer` and `ScreenTextRenderer`) rendered with orthographic projection, disabled depth testing, alpha blending, and sorted together by `zOrder`.

### Cameras

Add a `Camera` component to an object. The first constructed camera automatically becomes `Camera::mainCamera`:

```cpp
auto* cameraObj = Instantiate(new GameObject("MainCamera", glm::vec3(0.0f, 1.0f, 5.0f), glm::vec3(-10.0f, 0.0f, 0.0f), glm::vec3(1.0f)));
auto* camera = cameraObj->AddComponent<Camera>(ProjectionType::Perspective);
camera->fov = 60.0f;
camera->nearPlane = 0.1f;
camera->farPlane = 500.0f;
```

For orthographic cameras (e.g. 2D gameplay):
```cpp
auto* orthoCamera = cameraObj->AddComponent<Camera>(ProjectionType::Orthographic);
orthoCamera->orthoScale = 5.0f;
```

### Shaders

Construct shaders directly via `new Shader(...)`. Shaders are compiled immediately upon construction:

```cpp
// 1. Explicit vertex and fragment file paths
Shader* shader = new Shader("assets/shaders/custom.vert", "assets/shaders/custom.frag");

// 2. Base path shorthand (loads "<basePath>.vert" and "<basePath>.frag")
Shader* defaultShader = new Shader("assets/shaders/default");

// 3. Raw source strings
Shader* sourceShader = Shader::CreateFromSource(vertexGLSL, fragmentGLSL);
```

#### Vertex Attribute Convention

All shaders in Simple Engine share the standard vertex attribute layout:

| Location | Attribute | Type | Used By |
| :--- | :--- | :--- | :--- |
| `0` | `aPos` | `vec3` | Meshes, Quads, Text |
| `1` | `aNormal` | `vec3` | Meshes, Quads |
| `2` | `aColor` | `vec4` | Meshes, Quads |
| `3` | `aUV` | `vec2` | Meshes, Quads, Text |

Because text shaders also use `location = 0` (`vec3 aPos`) and `location = 3` (`vec2 aUV`), you can write text shaders in the exact same format as standard shaders:

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in vec2 aUV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoords;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoords = aUV;
}
```

### Textures & Texture Atlases

Construct textures with `new Texture(...)`:

```cpp
Texture* texture = new Texture(
    "assets/textures/wood.png",
    TextureWrapMode::REPEAT,
    TextureFilterMode::LINEAR
);
```

#### Texture Atlases / Sprite Sheets

Slice a sprite sheet into individual `Texture*` tiles using `Texture::CreateTextureMap`:

```cpp
// Slices left-to-right, top-to-bottom
std::vector<Texture*> frames = Texture::CreateTextureMap(
    "assets/sprites/character_sheet.png",
    glm::vec2(64.0f, 64.0f),  // Tile size in pixels (width, height)
    glm::vec2(0.0f, 0.0f),    // Padding/offset between tiles
    -1                        // Limit (-1 for all tiles, or max count)
);
```

### Materials

Materials are lightweight data containers holding shader pointers and material uniforms. Simple Engine provides three built-in material types:

#### 1. DefaultMaterial (3D Lit Geometry)
```cpp
auto* material = new DefaultMaterial(meshShader);
material->texture = texture;
material->baseColor = glm::vec3(1.0f);
material->ambient = glm::vec3(0.1f);
material->specularStrength = 0.5f;
material->shininess = 32.0f;
```

#### 2. ImageMaterial (2D Screen UI Quads)
```cpp
auto* imageMaterial = new ImageMaterial(uiShader);
imageMaterial->texture = iconTexture;
imageMaterial->color = glm::vec4(1.0f, 1.0f, 1.0f, 0.9f);
imageMaterial->grayscale = false; // Set to true to display in black-and-white
```

#### 3. TextMaterial (Font Rendering)
```cpp
auto* textMaterial = new TextMaterial(textShader);
textMaterial->color = glm::vec3(1.0f, 0.9f, 0.2f);
textMaterial->doubleSided = true;
```

### 3D Meshes & MeshRenderer

Create built-in primitives or load external 3D models with `ModelLoader`:

```cpp
// Built-in primitives (heap-allocated Mesh*)
Mesh* cubeMesh   = Mesh::CreateCube();
Mesh* sphereMesh = Mesh::CreateSphere(0.5f, 20, 20);
Mesh* quadMesh   = Mesh::CreateQuad();

// External 3D models via Assimp (.obj, .fbx, .gltf, etc.)
Mesh* modelMesh  = ModelLoader::LoadMesh("assets/models/liberator.obj");

// Attach to an object with a material
auto* cubeObj = Instantiate(new GameObject("Cube"));
cubeObj->AddComponent<MeshRenderer>(cubeMesh, material);
```

### 2D Screen UI: ImageRenderer & Button

Screen-space UI coordinates are normalized to the range `[-0.5, +0.5]`:

- **Position `(0, 0, 0)`:** Exact screen center
- **Position `(0.5, 0.5, 0)`:** Top-right corner
- **Position `(-0.5, -0.5, 0)`:** Bottom-left corner
- **Scale `(1, 1, 1)`:** Full screen width and height
- **Scale `(0.5, 0.5, 1)`:** Half screen width and half screen height

#### ImageRenderer
Renders a 2D quad in screen space with an `ImageMaterial`:
```cpp
auto* panelObj = Instantiate(new GameObject(
    "UIPanel",
    glm::vec3(0.0f, 0.3f, 0.0f),
    glm::vec3(0.0f),
    glm::vec3(0.4f, 0.2f, 1.0f)
));
panelObj->AddComponent<ImageRenderer>(imageMaterial, /* zOrder = */ 0);
```

#### Button
Provides mouse hover and click events for UI elements:
```cpp
auto* button = panelObj->AddComponent<Button>(
    []() {
        Debug::Log("Button clicked!");
    }
);

// Event hooks
button->OnHoverEnterEvent.AddListener([]() { Debug::Log("Hover entered"); });
button->OnHoverExitEvent.AddListener([]() { Debug::Log("Hover exited"); });

// Disable/enable interaction without removing the component
button->interactable = true;
```

### Text Rendering: World & Screen Space

Simple Engine uses FreeType to render TrueType (`.ttf`) and OpenType (`.otf`) fonts.

```cpp
// 1. World-Space Text (anchored in 3D world, rotates/scales with transform, depth-tested)
auto* worldTextObj = Instantiate(new GameObject("SignText", glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.25f)));
worldTextObj->AddComponent<WorldTextRenderer>(
    "Castle Entrance",
    "assets/fonts/arial.ttf",
    48, // font pixel size
    textMaterial,
    /* zOrder = */ 0
);

// 2. Screen-Space Text (HUD, menus, subtitles; uses normalized [-0.5, +0.5] screen coordinates)
auto* hudTextObj = Instantiate(new GameObject("HUDScore", glm::vec3(-0.4f, 0.45f, 0.0f), glm::vec3(0.0f), glm::vec3(0.2f, 0.08f, 1.0f)));
hudTextObj->AddComponent<ScreenTextRenderer>(
    "Score: 100",
    "assets/fonts/arial.ttf",
    48,
    textMaterial,
    /* zOrder = */ 1
);
```

### Lighting

Add `LightSource` to an object to include it in lighting calculations:

```cpp
auto* lightObj = Instantiate(new GameObject("Sun", glm::vec3(5.0f, 10.0f, 5.0f), glm::vec3(0.0f), glm::vec3(1.0f)));
auto* light = lightObj->AddComponent<LightSource>();
light->color = glm::vec3(1.0f, 0.95f, 0.8f);
```

The maximum active lights transmitted to shaders is controlled by `CONFIG::LIGHTING::MAX_LIGHTS` (default `256`).

---

## Input

Input polling is available globally through the `Input` static class:

### Keyboard
```cpp
Input::GetKey(GLFW_KEY_W);       // True while held down
Input::GetKeyDown(GLFW_KEY_W);   // True only on the frame the key was pressed
Input::GetKeyUp(GLFW_KEY_W);     // True only on the frame the key was released
```

### Mouse
```cpp
glm::vec2 pos = Input::MousePosition;  // Window pixel coordinates (origin at top-left)
glm::vec2 delta = Input::MouseDelta;    // Mouse movement delta since last frame

Input::GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
Input::GetMouseButtonUp(GLFW_MOUSE_BUTTON_LEFT);
```

### Cursor Visibility
Control system mouse cursor visibility via `PublicDomain`:
```cpp
PublicDomain::ChangeCursorVisibility(true);  // Normal visible cursor
PublicDomain::ChangeCursorVisibility(false); // Locked & hidden for FPS cameras
```

---

## Physics

### Rigidbodies
Attach a `Rigidbody` to apply gravity and forces:

```cpp
auto* rb = object->AddComponent<Rigidbody>();
rb->mass = 1.0f;
rb->useGravity = true;
rb->gravityScale = 1.0f;
rb->drag = 0.05f;

// Continuous force (accumulates until next physics step)
rb->AddForce(glm::vec3(0.0f, 10.0f, 0.0f));

// Instantaneous velocity change
rb->AddImpulse(glm::vec3(0.0f, 5.0f, 0.0f));
```

### Box Colliders
Attach a `BoxCollider` for axis-aligned bounding box collisions:

```cpp
auto* collider = object->AddComponent<BoxCollider>(
    /* isTrigger = */ false,
    /* center = */ glm::vec3(0.0f),
    /* size = */ glm::vec3(1.0f)
);
```

- Physical collisions automatically separate rigidbodies and resolve velocities along collision normals.
- When `isTrigger` is `true`, no physical force or separation is applied.

---

## Audio

Audio is powered by miniaudio. The audio subsystem is automatically initialized by `Engine::setup()` and uninitialized on shutdown:

```cpp
AudioSource sfx("assets/audio/explosion.wav");
sfx.Play();
```

---

## Timers and Tweens

Timers and Tweens are allocated on the heap via constructors. They register themselves automatically with the engine and update every frame:

### Timers
```cpp
// One-shot timer
new Timer(2.0f, /* repeating = */ false, []() {
    Debug::Log("2 seconds elapsed!");
});

// Repeating timer
new Timer(0.5f, /* repeating = */ true, []() {
    Debug::Log("Tick every 0.5s");
});
```

### Tweens
Interpolates a target variable smoothly over time (supports `float`, `double`, `int`, and GLM vectors):

```cpp
new Tween(
    &playerObject->transform->position,      // Target variable pointer
    glm::vec3(0.0f, 0.0f, 0.0f),              // Start value
    glm::vec3(0.0f, 5.0f, 0.0f),              // Target value
    1.5f,                                     // Duration in seconds
    nullptr,                                  // Optional easing function: float(float t)
    []() { Debug::Log("Jump tween complete!"); } // Optional onComplete callback
);
```

---

## Engine Configuration

Engine defaults can be modified in `Config.h`:

| Namespace | Key Settings |
| :--- | :--- |
| `CONFIG::ENGINE` | `VERSION`, OpenGL context version (`3.3`), `LOAD_FIRST_SCENE`, `ENABLE_DEBUG` |
| `CONFIG::RENDERING` | `ENABLE_DEPTH_TEST`, `FACE_CULLING`, `HIDE_CURSOR`, `ENABLE_VSYNC` |
| `CONFIG::LIGHTING` | `BACKGROUND_COLOR`, `MAX_LIGHTS` (defaults to 256) |
| `CONFIG::PHYSICS` | `GRAVITY` (defaults to `-9.81` on Y), `MAX_DELTA_TIME` (`0.1f`) |
| `CONFIG::WINDOW` | `TITLE`, `WIDTH`, `HEIGHT` |

Runtime overrides can be adjusted dynamically via `PublicDomain`:
```cpp
PublicDomain::Lighting::BackgroundColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
PublicDomain::Physics::Gravity = glm::vec3(0.0f, -19.6f, 0.0f);
PublicDomain::Rendering::EnableDepthTest = true;
PublicDomain::Rendering::FaceCulling = FACE_CULLING_MODE::BACK;
```

---

## Resource Ownership and Cleanup

Simple Engine uses direct raw pointers for resources (`Shader*`, `Texture*`, `Material*`, `Mesh*`).

- Resources created with `new` are owned by the scene or manager that created them.
- When overriding `Scene::Unload()`, clean up any resources instantiated during `Setup()`:

```cpp
void MainScene::Unload() override
{
    Scene::Unload(); // Clears all game objects

    delete myMaterial;
    myMaterial = nullptr;

    delete myTexture;
    myTexture = nullptr;

    delete myShader;
    myShader = nullptr;
}
```

---

## Common Issues & Troubleshooting

- **Black screen / nothing appears:** Verify that a `Camera` exists in the scene, and that the object has a renderer (`MeshRenderer` or `ImageRenderer`) with a valid `Material` and `Shader`.
- **Assets or shaders fail to load:** Ensure paths begin with `"assets/..."` (e.g. `"assets/shaders/Image.vert"`). Check that the post-build step in `CMakeLists.txt` successfully copied the `assets` folder next to your built binary.
- **Text does not appear:** Confirm that FreeType successfully loaded your font path, that the `TextRenderer` scale is non-zero, and that `TextMaterial` has a valid shader assigned.
- **Lighting appears completely dark:** Check that at least one `GameObject` in the scene has an active `LightSource` component and that the material shader declares the required `numLights` and `lights[...]` uniforms.
