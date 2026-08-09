#include "Pch.h"
#include "Sponza.h"
// Core
#include "RendererState.h"
// D3D12
#include "D3D12PipelineState.h"
// Data
#include "PBRMesh.h"
#include "Texture.h"
// Tools
#include "DescriptorHeapAllocator.h"
// Utils
#include "ShaderHelper.h"
#include "DebugHelper.h"
#include "SharedCommons.h"
#include "GPUCommons.h"

using namespace Microsoft::WRL;
using namespace DirectX;

Sponza::Sponza() : AssimpModel(), m_enableWireframe(false), m_instanceDataSRV{}, m_instanceDataDescriptorIndex(0),
    m_masterIndirectDescriptorIndex(0), m_mainIndirectCount(0), m_vaseIndirectCount(0), m_freezeCulling(false) {
	m_Transform.SetPosition(0.0f, 0.0f, 0.0f);
	m_rootSignature = nullptr;
    m_psoSolidCull = nullptr;
    m_psoSolidNoCull = nullptr;
    m_psoWireCull = nullptr;
    m_psoWireNoCull = nullptr;
    m_heapAllocator = nullptr;
} // Sponza

Sponza::~Sponza() {
	m_rootSignature = nullptr;
    m_psoSolidCull = nullptr;
    m_psoSolidNoCull = nullptr;
    m_psoWireCull = nullptr;
    m_psoWireNoCull = nullptr;
    m_heapAllocator = nullptr;
} // ~Sponza

bool Sponza::Init(const InitParams& params) {
    if (!params.psoSolidCull || !params.psoSolidNoCull || !params.psoWireCull || !params.psoWireNoCull
        || !params.heapAllocator || !params.rootSignature) {
        DebugHelper::DebugPrint("Sponza 초기화 실패");
        return false;
    }

	m_rootSignature = params.rootSignature;
    m_heapAllocator = params.heapAllocator;

    if (!AssimpModel::Init(params)) {
        DebugHelper::DebugPrint("Sponza 모델 로드 실패");
        return false;
    }

    if (!BuildInstanceDataBuffer(params.device, 1, 0.0f, false)) {
        return false;
    }

    if (!BuildIndirectBuffers(params.device, params.rootSignature, 1)) {
        DebugHelper::DebugPrint("Sponza 인다이렉트 버퍼 생성 실패");
        return false;
    }

    m_psoSolidCull = params.psoSolidCull;
    m_psoSolidNoCull = params.psoSolidNoCull;
    m_psoWireCull = params.psoWireCull;
    m_psoWireNoCull = params.psoWireNoCull;
	m_psoDepthSolid = params.psoDepthSolid;
	m_psoDepthAlpha = params.psoDepthAlpha;
    return true;
} // Init

void Sponza::SubmitIndirect(const SubmitIndirectParams& params) {
    ID3D12GraphicsCommandList* cmdList = params.cmdList;

    cmdList->SetGraphicsRootSignature(m_rootSignature);
    cmdList->SetGraphicsRootConstantBufferView(RendererState::FrameCBIndex, params.frameConstantsGPUAddress);
    cmdList->SetGraphicsRootConstantBufferView(RendererState::LightCBIndex, params.lightConstantsGPUAddress);
    cmdList->SetGraphicsRootDescriptorTable(RendererState::InstanceDataIndex, GetInstanceDataGPUHandle());
    cmdList->SetGraphicsRootDescriptorTable(RendererState::BindlessTexIndex, m_heapAllocator->GetGPUHandle(0));
    cmdList->SetGraphicsRootDescriptorTable(RendererState::BindlessBufIndex, m_heapAllocator->GetGPUHandle(0));

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D12PipelineState* mainPSO = m_enableWireframe ? m_psoWireCull : m_psoSolidCull;
    ID3D12PipelineState* vasePSO = m_enableWireframe ? m_psoWireNoCull : m_psoSolidNoCull;

    if (params.mainVisibleCommandsBuffer && m_mainIndirectCount > 0) {
        cmdList->SetPipelineState(mainPSO);
        cmdList->ExecuteIndirect(
            m_commandSignature.Get(),
            m_mainIndirectCount,
            params.mainVisibleCommandsBuffer,
            0, 
            params.mainCounterBuffer,
            0
        );
    }

    if (params.vaseVisibleCommandsBuffer && m_vaseIndirectCount > 0) {
        cmdList->SetPipelineState(vasePSO);
        cmdList->ExecuteIndirect(
            m_commandSignature.Get(),
            m_vaseIndirectCount,
            params.vaseVisibleCommandsBuffer,
            0,
            params.vaseCounterBuffer,
            0
        );
    }
} // SubmitIndirect

void Sponza::SubmitIndirectDepth(const SubmitIndirectParams& params) {
    if (!params.cmdList) return;

    ID3D12GraphicsCommandList* cmdList = params.cmdList;

    cmdList->SetGraphicsRootSignature(m_rootSignature);
    cmdList->SetGraphicsRootConstantBufferView(RendererState::FrameCBIndex, params.frameConstantsGPUAddress);
    cmdList->SetGraphicsRootConstantBufferView(RendererState::LightCBIndex, params.lightConstantsGPUAddress);
    cmdList->SetGraphicsRootDescriptorTable(RendererState::InstanceDataIndex, GetInstanceDataGPUHandle());
    cmdList->SetGraphicsRootDescriptorTable(RendererState::BindlessTexIndex, m_heapAllocator->GetGPUHandle(0));
    cmdList->SetGraphicsRootDescriptorTable(RendererState::BindlessBufIndex, m_heapAllocator->GetGPUHandle(0));

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    if (params.mainVisibleCommandsBuffer && m_mainIndirectCount > 0 && m_psoDepthSolid) {
        cmdList->SetPipelineState(m_psoDepthSolid);
        cmdList->ExecuteIndirect(
            m_commandSignature.Get(),
            m_mainIndirectCount,
            params.mainVisibleCommandsBuffer,
            0,
            params.mainCounterBuffer,
            0
        );
    }

    if (params.vaseVisibleCommandsBuffer && m_vaseIndirectCount > 0 && m_psoDepthAlpha) {
        cmdList->SetPipelineState(m_psoDepthAlpha);
        cmdList->ExecuteIndirect(
            m_commandSignature.Get(),
            m_vaseIndirectCount,
            params.vaseVisibleCommandsBuffer,
            0,
            params.vaseCounterBuffer,
            0
        );
    }
} // SubmitIndirectDepth

void                        Sponza::SetPosition(const XMFLOAT3& pos) { m_Transform.SetPosition(pos); } // SetPosition
void                        Sponza::SetPosition(float x, float y, float z) { m_Transform.SetPosition(x, y, z); } // SetPosition
XMMATRIX&                   Sponza::GetWorldMatrix() { return m_Transform.GetWorldMatrix(); } // GetWorldMatrix
D3D12_GPU_DESCRIPTOR_HANDLE Sponza::GetInstanceDataGPUHandle() const { return m_heapAllocator->GetGPUHandle(m_instanceDataDescriptorIndex); } // GetInstanceDataGPUHandle
UINT                        Sponza::GetVisibleCount() const { return m_mainIndirectCount + m_vaseIndirectCount; } // GetVisibleCount
ID3D12Resource*             Sponza::GetMainIndirectBuffer() const { return m_mainIndirectBuffer.Get(); } // GetMainIndirectBuffer
ID3D12Resource*             Sponza::GetVaseIndirectBuffer() const { return m_vaseIndirectBuffer.Get(); } // GetVaseIndirectBuffer
ID3D12Resource*             Sponza::GetInstanceDataBuffer() const { return m_instanceDataBuffer.Get(); } // GetInstanceDataBuffer
UINT                        Sponza::GetMainIndirectCount() const { return m_mainIndirectCount; } // GetMainIndirectCount
UINT                        Sponza::GetVaseIndirectCount() const { return m_vaseIndirectCount; } // GetVaseIndirectCount
UINT                        Sponza::GetInstanceDataDescriptorIndex() const { return m_instanceDataDescriptorIndex; } // GetInstanceDataDescriptorIndex
UINT                        Sponza::GetMasterIndirectDescriptorIndex() const { return m_masterIndirectDescriptorIndex; } // GetMasterIndirectDescriptorIndex

bool Sponza::BuildInstanceDataBuffer(ID3D12Device* device, const int gridSize, const float spacing, bool isColumn) {
    if (!device) {
        return false;
    }

    std::vector<GPUCommons::MeshInstanceData> instanceDataArray;
    instanceDataArray.reserve(m_meshes.size() * gridSize * gridSize);

    for (int z = 0; z < gridSize; ++z) {
        for (int x = 0; x < gridSize; ++x) {
            float offsetX = (x - (gridSize / 2.0f)) * spacing;
            float offsetZ = (z - (gridSize / 2.0f)) * spacing;

            XMMATRIX gridOffset = XMMatrixTranslation(offsetX, 0.0f, offsetZ);
            XMMATRIX rotation = isColumn ? XMMatrixRotationX(XM_PIDIV2) : XMMatrixIdentity();
            XMMATRIX transform = XMMatrixMultiply(m_Transform.GetWorldMatrix(), gridOffset);
            XMMATRIX finalWorldMatrix = XMMatrixMultiply(transform, rotation);
            XMMATRIX transposedWorld = XMMatrixTranspose(finalWorldMatrix);

            // 해당 위치에 스폰자의 모든 서브 메쉬 배치
            for (const auto& mesh : m_meshes) {
                if (!mesh) continue;

                GPUCommons::MeshInstanceData data{};
                data.worldMatrix = transposedWorld;
                data.vertexBufferIndex = mesh->GetVertexBufferSRVIndex();
                data.aabbMin = mesh->GetAABBMin();
                data.aabbMax = mesh->GetAABBMax();

                unsigned int matIdx = mesh->GetMaterialIndex();
                if (matIdx < m_materials.size()) {
                    data.albedoIndex = m_materials[matIdx].albedo->GetSRVIndex();
                    data.normalIndex = m_materials[matIdx].normal->GetSRVIndex();
                    data.alphaIndex = m_materials[matIdx].alpha->GetSRVIndex();
                }

                std::string meshName = mesh->GetName();
                bool isVase = (meshName.find("vase") != std::string::npos ||
                    meshName.find("leaf") != std::string::npos ||
                    meshName.find("Material__57") != std::string::npos);
                data.isVase = isVase ? 1.0f : 0.0f;

                instanceDataArray.push_back(data);
            } // for (const auto& mesh : m_meshes)
        } // for (int x = 0; x < gridSize; ++x)
    } // for (int z = 0; z < gridSize; ++z)

    UINT bufferSize = static_cast<UINT>(instanceDataArray.size() * sizeof(GPUCommons::MeshInstanceData));
    // Upload Heap으로 버퍼 리소스 생성
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(m_instanceDataBuffer.GetAddressOf())
    );

    void* pData = nullptr;
    m_instanceDataBuffer->Map(0, nullptr, &pData);
    memcpy(pData, instanceDataArray.data(), bufferSize);
    m_instanceDataBuffer->Unmap(0, nullptr);

    // Descriptor Heap 할당 및 SRV(Structured Buffer) 생성
    m_instanceDataDescriptorIndex = m_heapAllocator->Allocate();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = static_cast<UINT>(instanceDataArray.size());
    srvDesc.Buffer.StructureByteStride = sizeof(GPUCommons::MeshInstanceData);
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    device->CreateShaderResourceView(
        m_instanceDataBuffer.Get(),
        &srvDesc,
        m_heapAllocator->GetCPUHandle(m_instanceDataDescriptorIndex)
    );
    return true;
} // BuildInstanceDataBuffer

bool Sponza::BuildIndirectBuffers(ID3D12Device* device, ID3D12RootSignature* rootSignature, const int gridSize) {
    D3D12_INDIRECT_ARGUMENT_DESC args[3] = {};

    args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;

    args[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
    args[1].Constant.RootParameterIndex = RendererState::InstanceIndexParam;
    args[1].Constant.Num32BitValuesToSet = 1;
    args[1].Constant.DestOffsetIn32BitValues = 0;

    args[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

    D3D12_COMMAND_SIGNATURE_DESC csDesc = {};
    csDesc.ByteStride = sizeof(GPUCommons::IndirectCommand);
    csDesc.NumArgumentDescs = _countof(args);
    csDesc.pArgumentDescs = args;

    if (FAILED(device->CreateCommandSignature(&csDesc, rootSignature, IID_PPV_ARGS(&m_commandSignature)))) {
        DebugHelper::DebugPrint("CommandSignature 생성 실패");
        return false;
    }

    std::vector<GPUCommons::IndirectCommand> masterCmds;
    std::vector<GPUCommons::IndirectCommand> mainCmds, vaseCmds;
    UINT instanceIndex = 0;

    for (int z = 0; z < gridSize; ++z) {
        for (int x = 0; x < gridSize; ++x) {
            for (const auto& mesh : m_meshes) {
                if (!mesh) {
                    ++instanceIndex;
                    continue;
                }

                std::string meshName = mesh->GetName();
                bool isVase = (meshName.find("vase") != std::string::npos ||
                    meshName.find("leaf") != std::string::npos ||
                    meshName.find("Material__57") != std::string::npos);

                GPUCommons::IndirectCommand cmd{};
                cmd.instanceIndex = instanceIndex;
                cmd.indexBufferView = mesh->GetIndexBufferView();
                cmd.drawArgs.IndexCountPerInstance = mesh->GetIndexCount();
                cmd.drawArgs.InstanceCount = 1;
                cmd.drawArgs.StartIndexLocation = 0;
                cmd.drawArgs.BaseVertexLocation = 0;
                cmd.drawArgs.StartInstanceLocation = 0;

                masterCmds.push_back(cmd);
                (isVase ? vaseCmds : mainCmds).push_back(cmd);

                ++instanceIndex;
			} // for (const auto& mesh : m_meshes)
        } // for (int x = 0; x < gridSize; ++x)
    } // for (int z = 0; z < gridSize; ++z)

    auto uploadCommands = [device](const std::vector<GPUCommons::IndirectCommand>& cmds, ComPtr<ID3D12Resource>& outBuf) -> bool {
        if (cmds.empty()) return true;
        const UINT bufSize = static_cast<UINT>(sizeof(GPUCommons::IndirectCommand) * cmds.size());
        CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufSize);
        if (FAILED(device->CreateCommittedResource(
            &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&outBuf)))) {
            return false;
        }
        void* mapped = nullptr;
        outBuf->Map(0, nullptr, &mapped);
        memcpy(mapped, cmds.data(), bufSize);
        outBuf->Unmap(0, nullptr);
        return true;
    }; // auto uploadCommands

    if (!uploadCommands(mainCmds, m_mainIndirectBuffer)) {
        return false;
    }
    if (!uploadCommands(vaseCmds, m_vaseIndirectBuffer)) {
        return false;
    }
    if (!uploadCommands(masterCmds, m_masterIndirectBuffer)) {
        return false;
    }

    m_masterIndirectDescriptorIndex = m_heapAllocator->Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = static_cast<UINT>(masterCmds.size());
    srvDesc.Buffer.StructureByteStride = sizeof(GPUCommons::IndirectCommand); // 40바이트
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    device->CreateShaderResourceView(
        m_masterIndirectBuffer.Get(),
        &srvDesc,
        m_heapAllocator->GetCPUHandle(m_masterIndirectDescriptorIndex)
    );


    m_mainIndirectCount = static_cast<UINT>(mainCmds.size());
    m_vaseIndirectCount = static_cast<UINT>(vaseCmds.size());
    return true;
} // BuildIndirectBuffers

void Sponza::OnGUI() {
    ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.6f, 1.0f), "[ Sponza Options ]");
    ImGui::Checkbox("Wireframe Mode", &m_enableWireframe);
    ImGui::Separator();
    ImGui::Text("Visible Mesh: %u / %d", GetVisibleCount(), GetMeshCount());
    ImGui::Separator();
    ImGui::Checkbox("Freeze Culling Frustum", &m_freezeCulling);
    ImGui::Spacing();

    ImGui::TextDisabled("Resource Info");
    ImGui::Text("Total Meshes: "); ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%zu", GetMeshCount());

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "MATERIALS STATUS");
    ImGui::Spacing();

    std::vector<AssimpModel::MaterialInfo> materials = GetMaterialInfos();

    for (size_t i = 0; i < materials.size(); ++i) {
        const auto& mat = materials[i];

        if (ImGui::TreeNode((void*)(intptr_t)i, "Material [%d]: %s", (int)i, mat.name.c_str())) {
            ImGui::BeginGroup();

            auto ShowStatus = [](const char* type, bool isLoaded) {
                ImGui::Text("%-12s:", type);
                ImGui::SameLine();
                if (isLoaded) {
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), " [ LOADED ]");
                }
                else {
                    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), " [ MISSING ]");
                }
                };

            ShowStatus("Albedo", mat.hasAlbedo);
            ShowStatus("Normal", mat.hasNormal);
            ShowStatus("Alpha", mat.hasAlpha);
            ShowStatus("Metallic", mat.hasMetallic);
            ShowStatus("AO", mat.hasAO);
            ShowStatus("Smoothness", mat.hasSmoothness);

            ImGui::EndGroup();
            ImGui::TreePop();
        }
    }
} // OnGUI