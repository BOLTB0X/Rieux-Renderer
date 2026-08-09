#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <dxcapi.h>
#include <functional>
// D3D12
#include "D3D12PipelineState.h"
#include "D3D12RootSignature.h"

class PSOManager {
public:
    struct InitParams {
        ID3D12Device* device;
        DXGI_FORMAT   rtvFormat;
        DXGI_FORMAT   dsvFormat;

        InitParams();
    }; // InitDefaultParams

public:
    PSOManager();
    PSOManager(const PSOManager&) = delete;
    PSOManager& operator=(const PSOManager&) = delete;
    ~PSOManager();

    bool                 Init(const InitParams&);
    void                 Shutdown();

    D3D12PipelineState*  GetPSO(const std::string&) const;
    ID3D12RootSignature* GetRootSignature(const std::string&) const;

private:
    bool CreateRootSignature(const std::string&, const std::function<void(D3D12RootSignature::Builder&)>&);
    UINT GetRootParamIndex(const std::string&, const std::string&) const;

private:
    bool BuildDefaultSponzaPSO(const std::string&);
    bool BuildGPUDrivenPSO(const std::string&);
    bool BuildCullingComputePSO(const std::string&, const std::wstring&);

    bool BuildSolidCullBack(const std::string&, D3D12PipelineState::DefaultInitParams);
    bool BuildSolidCullNone(const std::string&, D3D12PipelineState::DefaultInitParams);
    bool BuildWireframeCullBack(const std::string&, D3D12PipelineState::DefaultInitParams);
    bool BuildWireframeCullNone(const std::string&, D3D12PipelineState::DefaultInitParams);

private:
    ID3D12Device*                                                        m_device;
    DXGI_FORMAT                                                          m_rtvFormat;
    DXGI_FORMAT                                                          m_dsvFormat;

    std::unordered_map<std::string, std::unique_ptr<D3D12RootSignature>> m_rootSignatureMap;
    std::unordered_map<std::string, std::unique_ptr<D3D12PipelineState>> m_psoMap;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<IDxcBlob>>    m_shaderBlobs;
}; // PSOManager