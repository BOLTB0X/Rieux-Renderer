#pragma once
#include <windows.h>

class Timer {
public:
	Timer();
	Timer(const Timer&) = delete;
	Timer& operator=(const Timer&) = delete;
	~Timer();

	bool Init();
	void Frame();

	void StartTimer();
	void StopTimer();

	int          GetTiming();
	float        GetFrameTime();
	const float& GetTotalTime();

private:
	float m_frequency;
	INT64 m_startTime;
	float m_frameTime;
	INT64 m_beginTime, m_endTime;
	float m_totalTime;
}; // Timer