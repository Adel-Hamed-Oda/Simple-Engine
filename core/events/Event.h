#pragma once

#include <functional>
#include <vector>

struct Event
{
    std::vector<std::function<void()>> listeners;

    Event() = default;
    Event(std::nullptr_t) {}

    Event(std::function<void()> callback)
    {
        if (callback)
            listeners.push_back(std::move(callback));
    }

    Event& operator=(std::nullptr_t)
    {
        listeners.clear();
        return *this;
    }

    Event& operator=(std::function<void()> callback)
    {
        listeners.clear();
        if (callback)
            listeners.push_back(std::move(callback));
        return *this;
    }

    Event& operator+=(std::function<void()> callback)
    {
        if (callback)
            listeners.push_back(std::move(callback));
        return *this;
    }

    void AddListener(std::function<void()> callback)
    {
        if (callback)
            listeners.push_back(std::move(callback));
    }

    template <typename Func, typename Arg, typename... Args>
    void AddListener(Func&& func, Arg&& arg, Args&&... args)
    {
        listeners.emplace_back(
            std::bind(
                std::forward<Func>(func),
                std::forward<Arg>(arg),
                std::forward<Args>(args)...
            )
        );
    }

    void Clear()
    {
        listeners.clear();
    }

    void Invoke() const
    {
        auto currentListeners = listeners;
        for (const auto& listener : currentListeners)
        {
            if (listener)
                listener();
        }
    }

    void operator()() const
    {
        Invoke();
    }

    explicit operator bool() const
    {
        return !listeners.empty();
    }
};