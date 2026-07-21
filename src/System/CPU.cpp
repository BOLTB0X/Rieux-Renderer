#include "Pch.h"
#include "CPU.h"
#include <intrin.h>

CPU::CPU()
    : m_canReadCpu(true),
    m_queryHandle(nullptr),
    m_counterHandle(nullptr),
    m_lastSampleTime(0),
    m_cpuUsage(0),
    m_name("Unknown CPU") {
} // CPU

CPU::~CPU() {
} // ~Cpu

bool CPU::Init() {
    if (PdhOpenQuery(NULL, 0, &m_queryHandle) == ERROR_SUCCESS) {
        PdhAddCounter(m_queryHandle, TEXT("\\Processor(_Total)\\% Processor Time"), 0, &m_counterHandle);
        m_canReadCpu = true;
    }
    m_lastSampleTime = GetTickCount64();

    int CPUInfo[4] = { -1 };
    char CPUBrandString[0x40];
    __cpuid(CPUInfo, 0x80000000);
    unsigned int nExIds = CPUInfo[0];

    memset(CPUBrandString, 0, sizeof(CPUBrandString));
    if (nExIds >= 0x80000004) {
        __cpuidex(CPUInfo, 0x80000002, 0); memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        __cpuidex(CPUInfo, 0x80000003, 0); memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        __cpuidex(CPUInfo, 0x80000004, 0); memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
        m_name = CPUBrandString;
    }

    return true;
} // Init

void CPU::Shutdown() {
    if (m_canReadCpu)
    {
        PdhCloseQuery(m_queryHandle);
    }
} // Shutdown

void CPU::Frame() {
    if (m_canReadCpu && (m_lastSampleTime + 1000) < GetTickCount64()) {
        m_lastSampleTime = GetTickCount64();
        PdhCollectQueryData(m_queryHandle);

        PDH_FMT_COUNTERVALUE value;
        PdhGetFormattedCounterValue(m_counterHandle, PDH_FMT_LONG, NULL, &value);
        m_cpuUsage = value.longValue;
    }
} // Frame

const long& CPU::GetCPUPercentage() const {
    return m_cpuUsage;
} // GetCPUPercentage

const std::string& CPU::GetName() const {
    return m_name;
} // GetName