# Simple Engine User Guide

This document explains how to use Simple Engine from an application that includes
the engine as a submodule. It focuses on the runtime API and available features.
Project-specific CMake integration is intentionally not covered here.

## Overview

Simple Engine is a C++20, OpenGL-based engine with:

- GLFW window and input handling
- GLAD OpenGL loading
- Scene and game-object management
- Component-based behaviours
- Transform hierarchies
- Mesh rendering with materials, shaders, textures, and lights
- Perspective and orthographic cameras
- Basic rigidbody physics and axis-aligned box colliders
- Audio playback through miniaudio
- Frame timing, timers, and value tweens
- Debug logging

The public entry point is `Engine::run()`. It initializes the window and
graphics context, initializes engine systems, runs the game loop, and performs
shutdown when the window closes.

## Minimal application

Include `Engine.h`, define at least one scene, register it, and start the
engine:

```cpp
#include <memory>

#include "Engine.h"

class MainScene : public Scene
{
public:
    void Setup() override
    {
        // Create the objects used by this scene here.
    }
};

int main()
{
    SceneManager::RegisterScene<MainScene>("main");
    return Engine::run();
}
```

`Setup()` is called after the OpenGL context and engine systems have been
initialized. This is the appropriate place to create OpenGL-backed resources
such as shaders, textures, and mesh renderers.

By default, `CONFIG::ENGINE::LOAD_FIRST_SCENE` is `true`, so the engine loads
the first registered scene when startup completes. Register one startup scene
when relying on this behavior. To load scenes explicitly, use
`SceneManager::LoadScene("scene-name")` after scenes have been registered and
the engine has initialized.

## Scenes and game objects

`Scene` owns its game objects. Add an object with `Instantiate()`:

```cpp
auto* player = Instantiate(new GameObject(
    "Player",
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f),
    glm::vec3(1.0f)
));
```

`Instantiate()` takes ownership of the pointer. Do not delete an object after
passing it to the scene.

A scene has the following lifecycle:

1. `Setup()` creates and configures objects and resources.
2. `Start()` is called once after setup and forwards to every behaviour.
3. `Update()` is called every frame and forwards to every behaviour.
4. `Unload()` is called when the scene is replaced or the engine shuts down.

Register and load scenes by name:

```cpp
SceneManager::RegisterScene<LevelScene>("level");
SceneManager::RegisterScene<MenuScene>("menu");

SceneManager::LoadScene("level");
SceneManager::HasScene("menu");
SceneManager::GetCurrentScene();
SceneManager::UnloadScene();
```

## Components and behaviours

Every `GameObject` has a `Transform`. Additional components derive from
`Component`. Use `AddComponent<T>()` and `GetComponent<T>()`:

```cpp
auto* cameraObject = Instantiate(new GameObject("Camera"));
auto* camera = cameraObject->AddComponent<Camera>();

cameraObject->transform->position = glm::vec3(0.0f, 0.0f, 5.0f);
cameraObject->transform->rotation = glm::vec3(0.0f, 180.0f, 0.0f);

Camera* sameCamera = cameraObject->GetComponent<Camera>();
```

To create game-specific logic, derive from `Behaviour`:

```cpp
class PlayerController : public Behaviour
{
public:
    void Start() override
    {
        Debug::Log("Player started");
    }

    void Update() override
    {
        if (Input::GetKey(GLFW_KEY_W))
            transform->position += transform->Forward() * 3.0f * Time::deltaTime;

        if (Input::GetKeyDown(GLFW_KEY_SPACE))
            Debug::Log("Jump pressed");
    }
};
```

The engine assigns both `owner` and `transform` when the component is added.
Behaviour updates run during the scene update phase.

## Transforms

`Transform` exposes:

```cpp
glm::vec3 position;
glm::vec3 rotation; // degrees, applied in Y, then X, then Z order
glm::vec3 scale;
Transform* parent;
```

Useful helpers include:

```cpp
transform->Forward();
transform->Right();
transform->Up();
transform->GetRotationMatrix();
transform->GetModelMatrix();
```

The forward direction is local `-Z`. Parent transforms affect the rotation and
model matrix of their children.

## Rendering

### Cameras

Add a `Camera` component to an object. The first camera constructed becomes
`Camera::mainCamera` and is used for rendering:

```cpp
auto* cameraObject = Instantiate(new GameObject(
    "Main Camera",
    glm::vec3(0.0f, 1.0f, 5.0f),
    glm::vec3(-10.0f, 0.0f, 0.0f),
    glm::vec3(1.0f)
));

auto* camera = cameraObject->AddComponent(
    std::make_unique<Camera>(ProjectionType::Perspective)
);
camera->fov = 60.0f;
camera->nearPlane = 0.1f;
camera->farPlane = 500.0f;
```

For a 2D-style camera, use `ProjectionType::Orthographic` and configure
`orthoScale`.

### Meshes and renderers

`Mesh` provides built-in geometry generators:

```cpp
Mesh cube = Mesh::CreateCube(glm::vec4(0.2f, 0.7f, 1.0f, 1.0f));
Mesh sphere = Mesh::CreateSphere(0.5f, 20, 20);
```

Custom meshes can fill `vertices`, `triangles`, `normals`, `colors`, and
`uvs`. `triangles` contains unsigned integer indices. Optional vertex
attributes are used only when they have the same number of entries as
`vertices`.

Attach a mesh renderer with a shared material:

```cpp
auto* cubeObject = Instantiate(new GameObject("Cube"));
auto material = std::make_shared<DefaultMaterial>("default");

cubeObject->AddComponent(
    std::make_unique<MeshRenderer>(Mesh::CreateCube(), material)
);
```

The renderer is drawn automatically for every object in the active scene that
has both a `MeshRenderer` and a valid material.

### Shaders

Create shaders after the engine has initialized the graphics context:

```cpp
Shader* shader = Shader::Create("shaders/default");
```

This reads:

```text
assets/shaders/default.vert
assets/shaders/default.frag
```

`Shader::Create(path, name)` stores a shader under a custom lookup name.
`Shader::CreateFromFiles(vertexPath, fragmentPath, name)` supports separate
paths. Shader file paths are relative to `CONFIG::DIRECTORY::SHADERS`, which
defaults to `assets/`.

Use a shader by name when constructing a material:

```cpp
auto material = std::make_shared<DefaultMaterial>("default");
```

The built-in shader and material convention expects the following vertex
attributes:

| Location | Attribute         |
| -------- | ----------------- |
| 0        | Position (`vec3`) |
| 1        | Normal (`vec3`)   |
| 2        | Color (`vec4`)    |
| 3        | UV (`vec2`)       |

`Shader::Get()` returns the named shader, or a fallback shader when the name is
missing. Creation and lookup failures are logged to `stderr`.

### Materials and textures

`DefaultMaterial` supports:

```cpp
material->baseColor;
material->ambient;
material->specularStrength;
material->shininess;
material->textureName;
```

Load a texture and reference it by its registry name:

```cpp
Texture* texture = Texture::Create("textures/crate.png", "crate");
if (texture)
    material->textureName = "crate";
```

The file above resolves to `assets/textures/crate.png`. Texture loading flips
images vertically while loading and restores the image-loader setting after
the operation.

Materials are shared with `std::shared_ptr`, so multiple mesh renderers can
use the same material.

### Lights

Add `LightSource` to an object. Every active light is sent to the material's
shader:

```cpp
auto* lightObject = Instantiate(new GameObject(
    "Key Light",
    glm::vec3(2.0f, 4.0f, 2.0f),
    glm::vec3(0.0f),
    glm::vec3(1.0f)
));

auto* light = lightObject->AddComponent<LightSource>();
light->color = glm::vec3(1.0f, 0.9f, 0.8f);
```

The maximum number of lights is `CONFIG::LIGHTING::MAX_LIGHTS`. The shader
used for lighting must provide the uniform layout expected by the material and
lighting code.

## Input

Keyboard state is available through `Input`:

```cpp
Input::GetKey(GLFW_KEY_A);       // held
Input::GetKeyDown(GLFW_KEY_A);   // pressed since the previous frame
Input::GetKeyUp(GLFW_KEY_A);     // released since the previous frame
```

Mouse data is available through:

```cpp
Input::MousePosition;
Input::MouseDelta;
Input::GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
Input::GetMouseButtonUp(GLFW_MOUSE_BUTTON_LEFT);
```

`MouseDelta` is reset at the end of each frame. Despite their names,
`GetMouseButtonDown()` and `GetMouseButtonUp()` currently report the current
pressed and released states respectively; they are not edge-triggered like
the keyboard methods.

## Physics

### Rigidbodies

Add a `Rigidbody` to participate in physics integration:

```cpp
auto* body = cubeObject->AddComponent<Rigidbody>();
body->mass = 1.0f;
body->gravityScale = 1.0f;
body->drag = 0.1f;
body->AddForce(glm::vec3(5.0f, 0.0f, 0.0f));
body->AddImpulse(glm::vec3(0.0f, 4.0f, 0.0f));
```

`AddForce()` accumulates force until the next physics update. `AddImpulse()`
changes velocity immediately. Set `useGravity` to `false` for a body that
should not receive gravity. Set `isKinematic` to `true` to exclude it from
automatic integration; move its transform yourself.

### Box colliders

`BoxCollider` uses an axis-aligned bounding box:

```cpp
auto* collider = cubeObject->AddComponent<BoxCollider>(
    false,                 // isTrigger
    glm::vec3(0.0f),       // center offset
    glm::vec3(1.0f)        // size
);
```

Non-trigger colliders are resolved by moving rigidbodies apart. Trigger
colliders do not receive physical resolution. `BoxCollider::IsCollidingWith()`
can be used for a direct overlap test.

The current collision implementation uses world position plus collider center
and size. It does not apply object rotation or scale to the collider bounds.
Physics uses `CONFIG::PHYSICS::MAX_DELTA_TIME` to cap the integration step.

## Audio

Audio is initialized and shut down automatically by `Engine::run()`:

```cpp
AudioSource music("audio/music.ogg");
music.Play();
```

Audio paths are relative to `CONFIG::DIRECTORY::AUDIO`, which defaults to
`assets/`. The example therefore loads `assets/audio/music.ogg`.

For explicit initialization in a custom lifecycle, use `AudioSource::Init()`
and `AudioSource::Shutdown()`, and ensure they are called only once per
miniaudio engine lifetime.

## Time, timers, and tweens

Per-frame timing is exposed through `Time`:

```cpp
float seconds = Time::time;
float dt = Time::deltaTime;
float fps = Time::FPS;
Time::timeScale = 0.5f;
```

`deltaTime` is scaled by `timeScale`. Physics additionally clamps its step to
`CONFIG::PHYSICS::MAX_DELTA_TIME`.

Create a one-shot or repeating timer:

```cpp
Timer::Create(2.0f, false, []
{
    Debug::Log("Two seconds elapsed");
});
```

Tween a numeric value or GLM vector:

```cpp
Tween::Create(
    &cubeObject->transform->position,
    glm::vec3(0.0f),
    glm::vec3(0.0f, 2.0f, 0.0f),
    1.0f,
    nullptr,
    [] { Debug::Log("Tween complete"); }
);
```

Timers and tweens are updated automatically by the engine loop. The pointer
passed to `Tween::Create()` must remain valid until the tween completes.

## Configuration

Edit `Config.h` before building the consuming application to set defaults:

| Namespace           | Examples                                                      |
| ------------------- | ------------------------------------------------------------- |
| `CONFIG::ENGINE`    | OpenGL version, first-scene loading, debug mode               |
| `CONFIG::RENDERING` | Depth testing, face culling, cursor visibility, vsync setting |
| `CONFIG::LIGHTING`  | Background color and maximum light count                      |
| `CONFIG::PHYSICS`   | Gravity and maximum physics timestep                          |
| `CONFIG::WINDOW`    | Initial title, width, and height                              |
| `CONFIG::DIRECTORY` | Asset, shader, texture, and audio roots                       |

Some runtime values are copied into `PublicDomain` during static
initialization. Use these values when changing settings at runtime:

```cpp
PublicDomain::Lighting::BackgroundColor =
    glm::vec4(0.05f, 0.08f, 0.12f, 1.0f);

PublicDomain::Physics::Gravity = glm::vec3(0.0f, -9.81f, 0.0f);
PublicDomain::Rendering::EnableDepthTest = true;
PublicDomain::Rendering::FaceCulling = FACE_CULLING_MODE::BACK;
```

`EnableDepthTest` and face culling are applied during engine setup. Change
these before `Engine::run()` if you need different startup behavior.

## Frame order

Each frame currently runs in this order:

1. Poll window events.
2. Clear the color and depth buffers.
3. Update `Time`.
4. Update keyboard input.
5. Update physics.
6. Update the active scene and behaviours.
7. Update tweens and timers.
8. Render scene objects.
9. Finish debug and input frame state.
10. Swap the window buffers.

This order means forces added from a behaviour are integrated on the next
physics pass, while changes made directly to transforms are visible during the
same frame's rendering pass.

## Shutdown and resource lifetime

Closing the primary window exits `Engine::run()`. The engine unloads the active
scene, shuts down audio, and terminates GLFW.

OpenGL-backed resources must be destroyed while a valid OpenGL context still
exists. If an application manages shader storage outside the normal engine
lifetime, call `Shader::Clear()` before destroying the window/context.

## Common issues

- **No object appears:** verify that the scene has a `Camera`, the camera is
  the first camera created, and the object has a `MeshRenderer` with a valid
  material.
- **Shader files are not found:** paths are resolved relative to
  `CONFIG::DIRECTORY::SHADERS` and the process working directory. A path such
  as `shaders/default` resolves to `assets/shaders/default.vert` and
  `assets/shaders/default.frag` by default.
- **Textures do not load:** verify the path relative to `assets/` and check
  `Texture::Get()`/the engine error output for the registered name.
- **Lighting has no effect:** verify that the active shader declares the light
  uniforms expected by `Rendering` and that the light object has a valid
  transform.
- **Physics behaves unexpectedly:** remember that colliders are axis-aligned
  and do not account for transform rotation or scale.
- **Startup scene is unexpected:** `LOAD_FIRST_SCENE` loads scene index `0`.
  Prefer a single registered startup scene when using the default behavior.
