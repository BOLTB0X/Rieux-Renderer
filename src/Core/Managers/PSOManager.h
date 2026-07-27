#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <dxcapi.h>
// D3D12
#include "D3D12PipelineState.h"

class PSOManager {
public:
    struct InitParams {
        ID3D12Device*        device;
        ID3D12RootSignature* mainRootSignature;
        DXGI_FORMAT          rtvFormat;
        DXGI_FORMAT          dsvFormat;

        InitParams();
    }; // InitParams

public:
    PSOManager();
    PSOManager(const PSOManager&) = delete;
    PSOManager& operator=(const PSOManager&) = delete;
    ~PSOManager();

    bool                Init(const InitParams&);
    void                Shutdown();
    D3D12PipelineState* GetPSO(const std::string&) const;

private:
    bool BuildStaticSponzaPSO();
    bool BuildSolidCullBack(const std::string&, D3D12PipelineState::InitParams);
    bool BuildSolidCullNone(const std::string&, D3D12PipelineState::InitParams);
    bool BuildWireframeCullBack(const std::string&, D3D12PipelineState::InitParams);
    bool BuildWireframeCullNone(const std::string&, D3D12PipelineState::InitParams);

private:
    ID3D12Device*                                                        m_device;
    ID3D12RootSignature*                                                 m_mainRootSignature;
    DXGI_FORMAT                                                          m_rtvFormat;
    DXGI_FORMAT                                                          m_dsvFormat;
    std::unordered_map<std::string, std::unique_ptr<D3D12PipelineState>> m_psoMap;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<IDxcBlob>>    m_shaderBlobs;
}; // PSOManager