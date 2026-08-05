#pragma once
// Resources
#include "Data/AssimpModel.h"
#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3d12.h>

class Frustum;
class FrustumCuller;

class Sponza : public AssimpModel {
public:
    struct InitParams : public AssimpModel::InitParams {
        ID3D12RootSignature* rootSignature = nullptr;
        ID3D12PipelineState* psoSolidCull = nullptr;
        ID3D12PipelineState* psoSolidNoCull = nullptr;
        ID3D12PipelineState* psoWireCull = nullptr;
        ID3D12PipelineState* psoWireNoCull = nullptr;
    }; // InitDefaultParams

    struct SubmitIndirectParams {
        ID3D12GraphicsCommandList* cmdList;
		D3D12_GPU_VIRTUAL_ADDRESS  frameConstantsGPUAddress;
		D3D12_GPU_VIRTUAL_ADDRESS  lightConstantsGPUAddress;

        ID3D12Resource*            mainVisibleCommandsBuffer;
        ID3D12Resource*            mainCounterBuffer;

        ID3D12Resource*            vaseVisibleCommandsBuffer;
        ID3D12Resource*            vaseCounterBuffer;

        SubmitIndirectParams()
            : cmdList(nullptr), frameConstantsGPUAddress(0), lightConstantsGPUAddress(0),
            mainVisibleCommandsBuffer(nullptr), mainCounterBuffer(nullptr),
            vaseVisibleCommandsBuffer(nullptr), vaseCounterBuffer(nullptr) {
        }
    }; // SubmitIndirectParams

public:
    Sponza();
    virtual ~Sponza();

    bool                        Init(const InitParams&);
    void                        SubmitIndirect(const SubmitIndirectParams&);
    const DirectX::XMMATRIX&    GetWorldMatrix() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetInstanceDataGPUHandle() const;
    UINT                        GetMasterIndirectDescriptorIndex() const;
    ID3D12Resource*             GetMainIndirectBuffer() const;
    ID3D12Resource*             GetVaseIndirectBuffer() const;
    ID3D12Resource*             GetInstanceDataBuffer() const;
    UINT                        GetMainIndirectCount() const;
    UINT                        GetVaseIndirectCount() const;
    UINT                        GetVisibleCount() const;
    UINT                        GetInstanceDataDescriptorIndex() const;
    void                        OnGUI();

private:
    bool BuildInstanceDataBuffer(ID3D12Device*);
    bool BuildIndirectBuffers(ID3D12Device*, ID3D12RootSignature*);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_instanceDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_commandSignature;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_masterIndirectBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_mainIndirectBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_vaseIndirectBuffer;

    D3D12_VERTEX_BUFFER_VIEW                       m_instanceDataSRV;
    DirectX::XMMATRIX                              m_worldMatrix;
    ID3D12RootSignature*                           m_rootSignature;
    ID3D12PipelineState*                           m_psoSolidCull;
    ID3D12PipelineState*                           m_psoSolidNoCull;
    ID3D12PipelineState*                           m_psoWireCull;
    ID3D12PipelineState*                           m_psoWireNoCull;
    DescriptorHeapAllocator*                       m_heapAllocator;
    bool                                           m_enableWireframe;
    UINT                                           m_instanceDataDescriptorIndex;
    UINT                                           m_masterIndirectDescriptorIndex;
    UINT                                           m_mainIndirectCount;
    UINT                                           m_vaseIndirectCount;
    bool                                           m_freezeCulling;
}; // Sponza