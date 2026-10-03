#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

class Window
{
public:
	// monitor and share should be added later
	Window(int width, int height, const char* title)
	{
		m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	}
	~Window();

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

	void Close();

private:
	GLFWwindow* m_window;
};