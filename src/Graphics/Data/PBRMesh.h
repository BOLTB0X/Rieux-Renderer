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

        InitParams() : device(nullptr), uploadCmdList(nullptr), vertices(nullptr), indices(nullptr), 
            materialIndex(0), name(""), heapAllocator(nullptr) {
        } // InitParams
    }; // InitParams

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
    unsigned int       GetMaterialIndex() const;
    UINT               GetIndexCount() const;
    UINT               GetVertexBufferSRVIndex() const;
    const std::string& GetName() const;

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW               m_vbView;
    D3D12_INDEX_BUFFER_VIEW                m_ibView;
    UINT                                   m_indexCount;
    UINT                                   m_materialIndex;
    UINT                                   m_vertexBufferSRVIndex;
    std::string                            m_name;
}; // PBRMesh 