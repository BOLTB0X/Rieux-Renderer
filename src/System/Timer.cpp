#include "Pch.h"
#include "Timer.h"

Timer::Timer()
	: m_frequency(0.0f),
	m_startTime(0),
	m_frameTime(0.0f),
	m_beginTime(0),
	m_endTime(0),
	m_totalTime(0.0f) {
} // Timer

Timer::~Timer() {
} // ~Timer

bool Timer::Init() {
	INT64 frequency;

	// 시스템의 초당 사이클 속도
	QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
	if (frequency == 0)
		return false;

	// 부동 소수점 형식으로 저장
	m_frequency = (float)frequency;

	// 초기 시작 시간을 확인
	QueryPerformanceCounter((LARGE_INTEGER*)&m_startTime);

	return true;
} // Init


void Timer::Frame() {
	INT64 currentTime;
	INT64 elapsedTicks;

	QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);

	elapsedTicks = currentTime - m_startTime;

	m_frameTime = (float)elapsedTicks / m_frequency;
	m_totalTime += m_frameTime;
	m_startTime = currentTime;
	return;
} // Frame

void Timer::StartTimer()
{
	QueryPerformanceCounter((LARGE_INTEGER*)&m_beginTime);
	return;
} // StartTimer

void Timer::StopTimer()
{
	QueryPerformanceCounter((LARGE_INTEGER*)&m_endTime);
	return;
} // StopTimer

int Timer::GetTiming() {
	float elapsedTicks;
	INT64 frequency;
	float milliseconds;

	elapsedTicks = (float)(m_endTime - m_beginTime);

	// 타이머의 초당 틱 속도
	QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);

	// 경과 시간을 밀리초 단위로 계산
	milliseconds = (elapsedTicks / (float)frequency) * 1000.0f;

	return (int)milliseconds;
} // GetTiming

float        Timer::GetFrameTime() { return m_frameTime; }
const float& Timer::GetTotalTime() { return m_totalTime; }