#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>

class Window
{
public:
	// monitor and share should be added later but I am too lazy for this
	Window(int width, int height, const char* title, bool fullscreen = false)
	{
		m_windowedWidth = width;
		m_windowedHeight = height;
		m_isFullscreen = fullscreen;

		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;

		if (fullscreen)
		{
			// If starting in fullscreen, prepare sensible centered windowed bounds for when toggled off
			if (mode)
			{
				m_windowedWidth = (mode->width * 3) / 4;
				m_windowedHeight = (mode->height * 3) / 4;
				m_windowedX = (mode->width - m_windowedWidth) / 2;
				m_windowedY = (mode->height - m_windowedHeight) / 2;
			}
			else
			{
				m_windowedWidth = 1280;
				m_windowedHeight = 720;
				m_windowedX = 100;
				m_windowedY = 100;
			}

			if (monitor && mode)
			{
				m_window = glfwCreateWindow(mode->width, mode->height, title, monitor, nullptr);
			}
			else if (monitor)
			{
				m_window = glfwCreateWindow(width, height, title, monitor, nullptr);
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
			if (m_window && mode)
			{
				m_windowedX = (mode->width - width) / 2;
				m_windowedY = (mode->height - height) / 2;
				glfwSetWindowPos(m_window, m_windowedX, m_windowedY);
			}
		}

		if (m_window && !fullscreen)
		{
			glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);
			glfwGetWindowSize(m_window, &m_windowedWidth, &m_windowedHeight);
		}
	}

	~Window()
	{
		Close();
	}

	// please do not use if you don't know what you are doing
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
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
			if (mode)
			{
				// Ensure windowed dimensions are smaller than monitor so window borders/title bar fit comfortably
				if (m_windowedWidth <= 0 || m_windowedHeight <= 0 ||
				    m_windowedWidth >= mode->width || m_windowedHeight >= mode->height)
				{
					m_windowedWidth = (mode->width * 3) / 4;
					m_windowedHeight = (mode->height * 3) / 4;
					m_windowedX = (mode->width - m_windowedWidth) / 2;
					m_windowedY = (mode->height - m_windowedHeight) / 2;
				}
			}

			// Restore window decorations (title bar, borders)
			glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_TRUE);

			// Restore windowed position and dimensions
			glfwSetWindowMonitor(
				m_window,
				nullptr,
				m_windowedX,
				m_windowedY,
				m_windowedWidth > 0 ? m_windowedWidth : 1280,
				m_windowedHeight > 0 ? m_windowedHeight : 720,
				GLFW_DONT_CARE
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
	int m_windowedX = 0;
	int m_windowedY = 0;
	int m_windowedWidth = 1920;
	int m_windowedHeight = 1080;
};
