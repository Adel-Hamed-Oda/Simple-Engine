#pragma once

#include <iostream>
#include <string>

class Debug
{
public:
	inline static bool enabled = true;
	inline static int frames = 0;

	static void EndFrame()
	{
		frames++;
	}

	// this should prevent me from writing 60 debugs per second, the only issue is it skips important info
	static void Log(const std::string& message, const int frame_skips = 0)
	{
		if (enabled && (frames % (frame_skips + 1) == 0))
			std::cout << "[DEBUG] " << message << std::endl;
	}
	static void LogError(const std::string& message)
	{
		if (enabled)
			std::cerr << "[ERROR] " << message << std::endl;
	}
};