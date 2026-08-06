#pragma once
#include <d3d12.h>
#include <wrl/client.h>
// Utils
#include "GPUCommons.h"

class RenderTexture;
class D3D12PipelineState;

class DepthRecorder {
public:
    struct InitParams {
        ID3D12Device*        device;
        ID3D12RootSignature* rootSignature;
        D3D12PipelineState*  solidDepthPSO;
        D3D12PipelineState*  alphaDepthPSO;

        InitParams() : device(nullptr), rootSignature(nullptr),
            solidDepthPSO(nullptr), alphaDepthPSO(nullptr) {
        }
	}; // InitParams

    struct RecordParams {
        ID3D12GraphicsCommandList* cmdList;
        RenderTexture*             depthTexture;
        D3D12_GPU_VIRTUAL_ADDRESS  frameConstantsGPUAddress;
        D3D12_GPU_VIRTUAL_ADDRESS  lightConstantsGPUAddress;
        ID3D12Resource*            mainIndirectBuffer;
        UINT                       mainCount;
        ID3D12Resource*            vaseIndirectBuffer;
        UINT                       vaseCount;

        RecordParams() : cmdList(nullptr), depthTexture(nullptr),
            frameConstantsGPUAddress(0), lightConstantsGPUAddress(0),
            mainIndirectBuffer(nullptr), mainCount(0),
            vaseIndirectBuffer(nullptr), vaseCount(0) {
        }
    }; // RecordParams

public:
    DepthRecorder();
    DepthRecorder(const DepthRecorder&) = delete;
    DepthRecorder& operator=(const DepthRecorder&) = delete;
    ~DepthRecorder();

    bool Init(const InitParams&);
    void RecordDepthPre(const RecordParams&);
	void RecordShadowMap(const RecordParams&);

private:
    Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_commandSignature;
    ID3D12RootSignature*                           m_rootSignature;
    D3D12PipelineState*                            m_solidDepthPSO;
    D3D12PipelineState*                            m_alphaDepthPSO;
}; // DepthRecorder