#pragma once
#include <string>
#include <vector>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>

class GPUMonitor {
public:
    struct InitParams {
        IDXGIAdapter3*      adapter;
        ID3D12Device*       device;
        ID3D12CommandQueue* commandQueue;
        uint32_t            maxQueries;

        InitParams() : adapter(nullptr), device(nullptr), commandQueue(nullptr), maxQueries(64) {
        }
    }; // InitDefaultParams

public:
    GPUMonitor();
    GPUMonitor(const GPUMonitor&) = delete;
    GPUMonitor& operator=(const GPUMonitor&) = delete;
    ~GPUMonitor();

    bool Init(const InitParams&);
    void Shutdown();
    void Frame();

    void RecordTimestamp(ID3D12GraphicsCommandList*, uint32_t);
    void ResolveQueryData(ID3D12GraphicsCommandList*);

    const std::string& GetName() const;
    float              GetVRAMUsageMB() const;
    float              GetVRAMTotalMB() const;
    double             GetTimeMs(uint32_t, uint32_t) const;

private:
    Microsoft::WRL::ComPtr<IDXGIAdapter3>   m_adapter;
    std::string                             m_name;
    float                                   m_vramUsageMB;
    float                                   m_vramTotalMB;
    Microsoft::WRL::ComPtr<ID3D12QueryHeap> m_queryHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource>  m_queryResultBuffer;
    uint64_t                                m_gpuTickFrequency;
    uint32_t                                m_maxQueries;
    std::vector<uint64_t>                   m_queryData;
}; // GPUMonitor