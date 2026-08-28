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
// Tools
#include "RootSignatureBuilder.h"

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
    D3D12RootSignature*  GetD3D12RootSignature(const std::string&) const;
    ID3D12RootSignature* GetID3D12RootSignature(const std::string&) const;

private:
    bool CreateRootSignature(const std::string&, const std::function<void(RootSignatureBuilder&)>&);
    UINT GetRootParamIndex(const std::string&, const std::string&) const;

private:
    bool BuildDefaultSponza(const std::string&);
    bool BuildGPUDriven(const std::string&);
    bool BuildFrustumCullingCompute(const std::string&, const std::wstring&);
    bool BuildShadowFrustumCullingCompute(const std::string&, const std::wstring&);
    bool BuildOcclusionCullingCompute(const std::string&, const std::wstring&);
    bool BuildBRDFIntegrationCompute(const std::string&, const std::wstring&);
    bool BuildPrefilterEnvironmentCompute(const std::string&, const std::wstring&);

    bool BuildDepthRecord(const std::string&);
	bool BuildShadowRecord(const std::string&);
    bool BuildCascadedShadowRecord(const std::string&);
	bool BuildHierarchicalZ(const std::string&);
    bool BuildProbeCapture(const std::string&);
    bool BuildGBuffer(const std::string&);

    bool BuildDebugTransReverseZ(const std::string&);
    bool BuildDebugAABB(const std::string&);
    bool BuildDebugLine(const std::string&);

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
