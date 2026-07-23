#include "Pch.h"
#include "PBRMesh.h"
// Utils
#include "DebugHelper.h"

using namespace DebugHelper;

PBRMesh::PBRMesh()
    : m_vbView{}, m_ibView{}, m_indexCount(0), m_materialIndex(0) {
} // PBRMesh

PBRMesh::~PBRMesh() {
} // ~PBRMesh

bool PBRMesh::Init(const InitParams& params,
    Microsoft::WRL::ComPtr<ID3D12Resource>& outVertexUpload,
    Microsoft::WRL::ComPtr<ID3D12Resource>& outIndexUpload) {

    if (!params.device || !params.uploadCmdList || !params.vertices || !params.indices) {
        DebugPrint("PBRMesh::Init - 잘못된 파라미터");
        return false;
    }

    m_indexCount = static_cast<UINT>(params.indices->size());
    m_materialIndex = params.materialIndex;

    const UINT vbSize = static_cast<UINT>(sizeof(PBRVertex) * params.vertices->size());
    const UINT ibSize = static_cast<UINT>(sizeof(unsigned int) * params.indices->size());

    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);

    // --------------------------------------------------
    // Vertex Buffer
    // --------------------------------------------------
    CD3DX12_RESOURCE_DESC vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vbSize);
    if (FAILED(params.device->CreateCommittedResource(
        &defaultHeapProps, D3D12_HEAP_FLAG_NONE, &vbDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_vertexBuffer)))) {
        DebugPrint("버텍스 버퍼 생성 실패");
        return false;
    }

    CD3DX12_RESOURCE_DESC vbUploadDesc = CD3DX12_RESOURCE_DESC::Buffer(vbSize);
    if (FAILED(params.device->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &vbUploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&outVertexUpload)))) {
        DebugPrint("버텍스 버퍼 업로드 힙 생성 실패");
        return false;
    }

    D3D12_SUBRESOURCE_DATA vbData = {};
    vbData.pData = params.vertices->data();
    vbData.RowPitch = vbSize;
    vbData.SlicePitch = vbSize;
    UpdateSubresources(params.uploadCmdList, m_vertexBuffer.Get(), outVertexUpload.Get(), 0, 0, 1, &vbData);

    CD3DX12_RESOURCE_BARRIER vbBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    params.uploadCmdList->ResourceBarrier(1, &vbBarrier);

    m_vbView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vbView.StrideInBytes = sizeof(PBRVertex);
    m_vbView.SizeInBytes = vbSize;

    // --------------------------------------------------
    // Index Buffer
    // --------------------------------------------------
    CD3DX12_RESOURCE_DESC ibDesc = CD3DX12_RESOURCE_DESC::Buffer(ibSize);
    if (FAILED(params.device->CreateCommittedResource(
        &defaultHeapProps, D3D12_HEAP_FLAG_NONE, &ibDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_indexBuffer)))) {
        DebugPrint("인덱스 버퍼 생성 실패");
        return false;
    }

    CD3DX12_RESOURCE_DESC ibUploadDesc = CD3DX12_RESOURCE_DESC::Buffer(ibSize);
    if (FAILED(params.device->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &ibUploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&outIndexUpload)))) {
        DebugPrint("인덱스 버퍼 업로드 힙 생성 실패");
        return false;
    }

    D3D12_SUBRESOURCE_DATA ibData = {};
    ibData.pData = params.indices->data();
    ibData.RowPitch = ibSize;
    ibData.SlicePitch = ibSize;
    UpdateSubresources(params.uploadCmdList, m_indexBuffer.Get(), outIndexUpload.Get(), 0, 0, 1, &ibData);

    CD3DX12_RESOURCE_BARRIER ibBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    params.uploadCmdList->ResourceBarrier(1, &ibBarrier);

    m_ibView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_ibView.Format = DXGI_FORMAT_R32_UINT;
    m_ibView.SizeInBytes = ibSize;

    return true;
} // Init

void PBRMesh::BindBuffers(ID3D12GraphicsCommandList* cmdList) {
    cmdList->IASetVertexBuffers(0, 1, &m_vbView);
    cmdList->IASetIndexBuffer(&m_ibView);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
} // BindBuffers

void PBRMesh::Render(ID3D12GraphicsCommandList* cmdList) {
    BindBuffers(cmdList);
    cmdList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
} // RenderBuffer

unsigned int PBRMesh::GetMaterialIndex() const {
    return m_materialIndex;
} // GetMaterialIndex

UINT PBRMesh::GetIndexCount() const {
    return m_indexCount;
} // GetIndexCount