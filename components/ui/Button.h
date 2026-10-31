#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <cmath>
#include <functional>
#include <utility>
#include <vector>

#include "../Component.h"
#include "../../core/__Core__.h"

class Button : public Component
{
public:
    Event<> OnClickEvent;

    Event<> OnHoverEnterEvent;
    Event<> OnHoverExitEvent;

    // Local size multiplier relative to transform->scale (1.0 matches ImageRenderer's unit quad).
    glm::vec2 size{ 1.0f, 1.0f };

    // Screen-space offset added to transform->position.xy.
    glm::vec2 offset{ 0.0f, 0.0f };

    // When false, the button ignores mouse hover and clicks (e.g., locked Smurf cards).
    bool interactable = true;

    Button() = default;

    explicit Button(
        std::function<void()> onClick,
        glm::vec2 size = glm::vec2(1.0f, 1.0f),
        glm::vec2 offset = glm::vec2(0.0f, 0.0f)
    )
        : OnClickEvent(std::move(onClick)),
          size(size),
          offset(offset)
    {
    }

    template <typename Func, typename Arg, typename... Args>
        requires (!std::same_as<std::decay_t<Func>, Button> &&
                  !std::same_as<std::decay_t<Func>, std::function<void()>> &&
                  !std::convertible_to<Func, std::function<void()>>)
    Button(Func&& func, Arg&& arg, Args&&... args)
    {
        OnClickEvent.AddListener(
            std::forward<Func>(func),
            std::forward<Arg>(arg),
            std::forward<Args>(args)...
        );
    }

    ~Button() override = default;

    void Start() override
    {
        wasMouseDown = Input::GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
        isHovered = false;
        isPressed = false;
    }

    void Update() override
    {
        const bool mouseDown = Input::GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);

        if (!interactable || !transform || !PublicDomain::CurrentWindow || !PublicDomain::CurrentWindow->GetGLFWWindow())
        {
            if (isHovered)
            {
                isHovered = false;
                isPressed = false;
                wasMouseDown = mouseDown;
                OnHoverExitEvent();
            }
            else
            {
                isPressed = false;
                wasMouseDown = mouseDown;
            }
            return;
        }

        const glm::vec2 mouseScreenPos = GetMouseScreenPosition();
        const bool currentlyHovered = ContainsScreenPoint(mouseScreenPos);

        if (currentlyHovered && !isHovered)
        {
            isHovered = true;
            OnHoverEnterEvent();
        }
        else if (!currentlyHovered && isHovered)
        {
            isHovered = false;
            isPressed = false;
            OnHoverExitEvent();
        }

        const bool clickedThisFrame = isHovered && mouseDown && !wasMouseDown;

        if (!mouseDown)
        {
            isPressed = false;
        }
        else if (clickedThisFrame)
        {
            isPressed = true;
        }

        // Update state before invoking OnClickEvent in case the callback loads a new scene.
        wasMouseDown = mouseDown;

        if (clickedThisFrame)
        {
            OnClickEvent();
        }
    }

    bool IsHovered() const { return isHovered; }
    bool IsPressed() const { return isPressed; }

    void SetOnClick(std::function<void()> callback)
    {
        OnClickEvent = std::move(callback);
    }

    void AddOnClickListener(std::function<void()> callback)
    {
        OnClickEvent.AddListener(std::move(callback));
    }

    bool ContainsScreenPoint(const glm::vec2& screenPoint) const
    {
        if (!transform)
            return false;

        const glm::vec2 center(
            transform->position.x + offset.x,
            transform->position.y + offset.y
        );

        const glm::vec2 halfExtents(
            std::abs(transform->scale.x * size.x) * 0.5f,
            std::abs(transform->scale.y * size.y) * 0.5f
        );

        return
            screenPoint.x >= (center.x - halfExtents.x) &&
            screenPoint.x <= (center.x + halfExtents.x) &&
            screenPoint.y >= (center.y - halfExtents.y) &&
            screenPoint.y <= (center.y + halfExtents.y);
    }

    // Converts Input::MousePosition (window pixel coordinates, top-left origin)
    // into normalized screen-space coordinates ([-0.5, +0.5], [-0.5, +0.5]).
    static glm::vec2 GetMouseScreenPosition()
    {
        if (!PublicDomain::CurrentWindow || !PublicDomain::CurrentWindow->GetGLFWWindow())
            return glm::vec2(0.0f);

        GLFWwindow* window = PublicDomain::CurrentWindow->GetGLFWWindow();

        int winWidth = 0, winHeight = 0;
        glfwGetWindowSize(window, &winWidth, &winHeight);
        if (winWidth <= 0 || winHeight <= 0)
            return glm::vec2(0.0f);

        const float screenX = (Input::MousePosition.x / static_cast<float>(winWidth)) - 0.5f;
        const float screenY = 0.5f - (Input::MousePosition.y / static_cast<float>(winHeight));

        return glm::vec2(screenX, screenY);
    }

private:
    bool isHovered = false;
    bool isPressed = false;
    bool wasMouseDown = false;
};