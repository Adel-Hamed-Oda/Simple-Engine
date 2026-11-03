#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>

class Window
{
public:
	// monitor and share should be added later
	Window(int width, int height, const char* title, bool fullscreen = false)
	{
		m_windowedWidth = width;
		m_windowedHeight = height;
		m_isFullscreen = fullscreen;

		if (fullscreen)
		{
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			if (monitor)
			{
				const GLFWvidmode* mode = glfwGetVideoMode(monitor);
				if (mode)
				{
					m_window = glfwCreateWindow(mode->width, mode->height, title, monitor, nullptr);
				}
				else
				{
					m_window = glfwCreateWindow(width, height, title, monitor, nullptr);
				}
			}
			else
			{
				m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
				m_isFullscreen = false;
			}
		}
		else
		{
			m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
		}

		if (m_window)
		{
			glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);
		}
	}

	~Window()
	{
		Close();
	}

	GLFWwindow* GetGLFWWindow() const
	{
		return m_window;
	}
	glm::vec2 GetPosition() const
	{
		int x, y;
		glfwGetWindowPos(m_window, &x, &y);
		return glm::vec2(x, y);
	}
	glm::vec2 GetSize() const
	{
		int width, height;
		glfwGetWindowSize(m_window, &width, &height);
		return glm::vec2(width, height);
	}
	std::string GetTitle() const
	{
		return std::string(glfwGetWindowTitle(m_window));
	}

	bool IsFullscreen() const
	{
		return m_isFullscreen;
	}

	void SetPosition(int x, int y)
	{
		glfwSetWindowPos(m_window, x, y);
	}
	void SetSize(float width = -1.0f, float height = -1.0f)
	{
		glm::vec2 size = GetSize();
		if (width == -1.0f) width = size.x;
		if (height == -1.0f) height = size.y;
		glfwSetWindowSize(m_window, (int)width, (int)height);
	}
	void SetTitle(const std::string& title)
	{
		glfwSetWindowTitle(m_window, title.c_str());
	}

	void SetFullscreen(bool fullscreen)
	{
		if (m_isFullscreen == fullscreen || !m_window)
			return;

		m_isFullscreen = fullscreen;

		if (m_isFullscreen)
		{
			// Save current windowed position and dimensions
			glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);
			glfwGetWindowSize(m_window, &m_windowedWidth, &m_windowedHeight);

			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			if (monitor)
			{
				const GLFWvidmode* mode = glfwGetVideoMode(monitor);
				if (mode)
				{
					glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
				}
				else
				{
					glfwSetWindowMonitor(m_window, monitor, 0, 0, m_windowedWidth, m_windowedHeight, GLFW_DONT_CARE);
				}
			}
		}
		else
		{
			// Restore previous windowed position and dimensions
			glfwSetWindowMonitor(
				m_window,
				nullptr,
				m_windowedX,
				m_windowedY,
				m_windowedWidth > 0 ? m_windowedWidth : 1280,
				m_windowedHeight > 0 ? m_windowedHeight : 720,
				0
			);
		}
	}

	void ToggleFullscreen()
	{
		SetFullscreen(!m_isFullscreen);
	}

	void Close()
	{
		if (m_window)
		{
			glfwDestroyWindow(m_window);
			m_window = nullptr;
		}
	}

private:
	GLFWwindow* m_window = nullptr;
	bool m_isFullscreen = false;
	int m_windowedX = 100;
	int m_windowedY = 100;
	int m_windowedWidth = 1920;
	int m_windowedHeight = 1080;
};
