#pragma once
#pragma comment(lib, "pdh.lib")
#include <pdh.h>

class CPU {
public:
    CPU();
    CPU(const CPU&) = delete;
    CPU& operator=(const CPU&) = delete;
    ~CPU();

    bool       Init();
    void       Shutdown();
    void       Frame();

    const long&        GetCPUPercentage() const;
    const std::string& GetName() const;

private:
    bool          m_canReadCpu;
    HQUERY        m_queryHandle;
    HCOUNTER      m_counterHandle;
    unsigned long m_lastSampleTime;
    long          m_cpuUsage;
    std::string   m_name;
}; // CPU