// FPS.cpp
#include "Pch.h"
#include "FPS.h"

FPS::FPS()
	: m_fps(0),
	m_count(0),
	m_startTime(0) {
} // FPS

FPS::~FPS() {
} // ~FPS

bool FPS::Init() {
	m_fps = 0;
	m_count = 0;
	m_startTime = timeGetTime();
	return true;
} // init

void FPS::Frame() {
	m_count++;

	if (timeGetTime() >= (m_startTime + 1000)) {
		m_fps = m_count;
		m_count = 0;

		m_startTime = timeGetTime();
	}
} // Frame

const int& FPS::GetFPS() const {
	return m_fps;
} // GetFPS