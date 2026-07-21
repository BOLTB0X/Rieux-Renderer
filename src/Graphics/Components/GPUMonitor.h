#pragma once
#include <dxgi1_4.h>
#include <wrl.h>
#include <string>

class GPUMonitor {
public:
    GPUMonitor();
    GPUMonitor(const GPUMonitor&) = delete;
    GPUMonitor& operator=(const GPUMonitor&) = delete;
    ~GPUMonitor();

    bool Init(IDXGIAdapter3*);
    void Shutdown();
    void Frame();

    const std::string& GetName() const;
    float              GetVRAMUsageMB() const;
    float              GetVRAMTotalMB() const;

private:
    Microsoft::WRL::ComPtr<IDXGIAdapter3> m_adapter;
    std::string                           m_name;
    float                                 m_vramUsageMB;
    float                                 m_vramTotalMB;
}; // GPUMonitor