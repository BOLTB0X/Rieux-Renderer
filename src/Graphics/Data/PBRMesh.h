#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <DirectXMath.h>
#include <string>

class DescriptorHeapAllocator;

class PBRMesh {
public:
    struct PBRVertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT2 texture;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT3 tangent;
        DirectX::XMFLOAT3 binormal;

        PBRVertex() : position(0.0f, 0.0f, 0.0f), texture(0.0f, 0.0f), normal(0.0f, 0.0f, 0.0f),
            tangent(0.0f, 0.0f, 0.0f), binormal(0.0f, 0.0f, 0.0f) {
        } // PBRVertex
    }; // PBRVertex

    struct InitParams {
        ID3D12Device*                    device;
        ID3D12GraphicsCommandList*       uploadCmdList;
        const std::vector<PBRVertex>*    vertices;
        const std::vector<unsigned int>* indices;
        unsigned int                     materialIndex;
        std::string                      name;
        DescriptorHeapAllocator*         heapAllocator;
        DirectX::XMFLOAT3                aabbMin;
        DirectX::XMFLOAT3                aabbMax;
        UINT                             shadowIndexCount;
        UINT                             shadowStartIndex;

        InitParams() : device(nullptr), uploadCmdList(nullptr),
            vertices(nullptr), indices(nullptr), 
            materialIndex(0), name(""), heapAllocator(nullptr),
            aabbMin(0.0f, 0.0f, 0.0f), aabbMax(0.0f, 0.0f, 0.0f),
            shadowIndexCount(0), shadowStartIndex(0) {
        } // InitDefaultParams
    }; // InitDefaultParams

public:
    PBRMesh();
    PBRMesh(const PBRMesh&) = delete;
    PBRMesh& operator=(const PBRMesh&) = delete;
    ~PBRMesh();

    bool Init(const InitParams&,
        Microsoft::WRL::ComPtr<ID3D12Resource>&,
        Microsoft::WRL::ComPtr<ID3D12Resource>&);

    void BindBuffers(ID3D12GraphicsCommandList*);
    void Render(ID3D12GraphicsCommandList*);

public:
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const;
    unsigned int                   GetMaterialIndex() const;
    UINT                           GetIndexCount() const;
    UINT                           GetVertexBufferSRVIndex() const;
    const std::string&             GetName() const;
    const DirectX::XMFLOAT3&       GetAABBMin() const;
    const DirectX::XMFLOAT3&       GetAABBMax() const;
    UINT                           GetShadowIndexCount() const;
    UINT                           GetShadowStartIndex() const;

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW                m_ibView;
    UINT                                   m_indexCount;
    UINT                                   m_materialIndex;
    UINT                                   m_vertexBufferSRVIndex;
    std::string                            m_name;
    DirectX::XMFLOAT3                      m_aabbMin;
    DirectX::XMFLOAT3                      m_aabbMax;
    UINT                                   m_shadowIndexCount;
    UINT                                   m_shadowStartIndex;
}; // PBRMesh 