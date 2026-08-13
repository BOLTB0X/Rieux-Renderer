#pragma once
// Resources
#include "Data/AssimpModel.h"
#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3d12.h>
// Components
#include "Transform.h"

class Sponza : public AssimpModel {
public:
    struct InitParams : public AssimpModel::InitParams {
        ID3D12RootSignature* rootSignature = nullptr;
        ID3D12RootSignature* debugRootSignature = nullptr;
        ID3D12PipelineState* psoSolidCull = nullptr;
        ID3D12PipelineState* psoSolidNoCull = nullptr;
        ID3D12PipelineState* psoWireCull = nullptr;
        ID3D12PipelineState* psoWireNoCull = nullptr;
        ID3D12PipelineState* psoDepthSolid = nullptr;
        ID3D12PipelineState* psoDepthAlpha = nullptr;
        ID3D12PipelineState* psoShadowSolid = nullptr;
        ID3D12PipelineState* psoShadowAlpha = nullptr;
        ID3D12PipelineState* psoDebug = nullptr;
    }; // InitDefaultParams

    enum class SubmitIndirectType {
        General,
        Depth,
        Shadow
    }; // RenderTextureType

    struct SubmitIndirectParams {
        ID3D12GraphicsCommandList* cmdList;

		D3D12_GPU_VIRTUAL_ADDRESS  frameConstantsGPUAddress;
		D3D12_GPU_VIRTUAL_ADDRESS  lightConstantsGPUAddress;
		UINT                       shadowMapDescriptorIndex;

        ID3D12Resource*            mainVisibleCommandsBuffer;
        ID3D12Resource*            mainCounterBuffer;

        ID3D12Resource*            vaseVisibleCommandsBuffer;
        ID3D12Resource*            vaseCounterBuffer;

        SubmitIndirectType         type;

        SubmitIndirectParams()
            : cmdList(nullptr), frameConstantsGPUAddress(0), lightConstantsGPUAddress(0), shadowMapDescriptorIndex(UINT_MAX),
            mainVisibleCommandsBuffer(nullptr), mainCounterBuffer(nullptr),
            vaseVisibleCommandsBuffer(nullptr), vaseCounterBuffer(nullptr),
            type(SubmitIndirectType::General){
        }
    }; // SubmitIndirectParams

    struct RenderDebugParams {
        ID3D12GraphicsCommandList* cmdList;
        D3D12_GPU_VIRTUAL_ADDRESS  frameConstantsGPUAddress;
        ID3D12Resource*            mainCmdBuffer;
        ID3D12Resource*            mainCounter;
        ID3D12Resource*            vaseCmdBuffer;
        ID3D12Resource*            vaseCounter;

        RenderDebugParams() : cmdList(nullptr), frameConstantsGPUAddress(0),
            mainCmdBuffer(nullptr), mainCounter(nullptr),
            vaseCmdBuffer(nullptr), vaseCounter(nullptr) {
        }
    }; // RenderDebugParams

public:
    Sponza();
    virtual ~Sponza();

    bool                        Init(const InitParams&);
    void                        SubmitIndirect(const SubmitIndirectParams&);
    void                        RenderDebugAABB(const RenderDebugParams&);
    void                        OnGUI();

public:
    void                        SetPosition(const DirectX::XMFLOAT3&);
    void                        SetPosition(float, float, float);

    DirectX::XMMATRIX&          GetWorldMatrix();
    D3D12_GPU_DESCRIPTOR_HANDLE GetInstanceDataGPUHandle() const;
    UINT                        GetMasterIndirectDescriptorIndex() const;
    ID3D12Resource*             GetMainIndirectBuffer() const;
    ID3D12Resource*             GetVaseIndirectBuffer() const;
    ID3D12Resource*             GetInstanceDataBuffer() const;
    UINT                        GetMainIndirectCount() const;
    UINT                        GetVaseIndirectCount() const;
    UINT                        GetVisibleCount() const;
    UINT                        GetInstanceDataDescriptorIndex() const;

private:
    bool BuildInstanceDataBuffer(ID3D12Device*, const int, const float, bool);
    bool BuildIndirectBuffers(ID3D12Device*, ID3D12RootSignature*, const int);
    bool BuildDebugCommandSignature(ID3D12Device*);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_instanceDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_commandSignature;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_masterIndirectBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_mainIndirectBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_vaseIndirectBuffer;
    Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_debugCommandSignature;

    D3D12_VERTEX_BUFFER_VIEW                       m_instanceDataSRV;
    ID3D12RootSignature*                           m_rootSignature;
    ID3D12RootSignature*                           m_debugRootSignature;
    ID3D12PipelineState*                           m_psoSolidCull;
    ID3D12PipelineState*                           m_psoSolidNoCull;
    ID3D12PipelineState*                           m_psoWireCull;
    ID3D12PipelineState*                           m_psoWireNoCull;
    ID3D12PipelineState*                           m_psoDepthSolid;
    ID3D12PipelineState*                           m_psoDepthAlpha;
    ID3D12PipelineState*                           m_psoShadowSolid;
    ID3D12PipelineState*                           m_psoShadowAlpha;
    ID3D12PipelineState*                           m_psoDebug;

    DescriptorHeapAllocator*                       m_heapAllocator;

    bool                                           m_enableWireframe;

    UINT                                           m_instanceDataDescriptorIndex;
    UINT                                           m_masterIndirectDescriptorIndex;
    UINT                                           m_mainIndirectCount;
    UINT                                           m_vaseIndirectCount;

    bool                                           m_freezeCulling;
    bool                                           m_showDebugAABB;
    Transform                                      m_Transform;
}; // Sponza
