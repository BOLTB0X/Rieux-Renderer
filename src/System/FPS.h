#pragma once
#pragma comment(lib, "winmm.lib")
#include <windows.h>
#include <mmsystem.h>

class FPS {
public:
	FPS();
	FPS(const FPS&) = delete;
	FPS& operator=(const FPS&) = delete;
	~FPS();

	bool       Init();
	void       Frame();
	const int& GetFPS() const;

private:
	int           m_fps;
	int           m_count;
	unsigned long m_startTime;
}; // FPS