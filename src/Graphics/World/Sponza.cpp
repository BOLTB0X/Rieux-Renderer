#include "Pch.h"
#include "Sponza.h"
// Core
#include "RendererState.h"
// Components
#include "RenderQueue.h"
#include "DescriptorHeapAllocator.h"
// D3D12
#include "D3D12/D3D12PipelineState.h"
// Resources
#include "Data/PBRMesh.h"
#include "Data/Texture.h"
// Utils
#include "ShaderHelper.h"
#include "DebugHelper.h"
#include "SharedCommons.h"

using namespace Microsoft::WRL;
using namespace DirectX;

Sponza::Sponza() : AssimpModel(), m_enableWireframe(false) {
    m_worldMatrix = XMMatrixIdentity();
    m_psoSolidCull = nullptr;
    m_psoSolidNoCull = nullptr;
    m_psoWireCull = nullptr;
    m_psoWireNoCull = nullptr;
    m_heapAllocator = nullptr;
} // StaticModel

Sponza::~Sponza() {
    m_psoSolidCull = nullptr;
    m_psoSolidNoCull = nullptr;
    m_psoWireCull = nullptr;
    m_psoWireNoCull = nullptr;
    m_heapAllocator = nullptr;
} // ~Sponza

bool Sponza::Init(const InitParams& params) {
    m_heapAllocator = params.heapAllocator;

    if (!AssimpModel::Init(params)) {
        DebugHelper::DebugPrint("Sponza 모델 로드 실패");
        return false;
    }

    if (!params.psoSolidCull || !params.psoSolidNoCull ||
        !params.psoWireCull || !params.psoWireNoCull || !params.heapAllocator) {
        DebugHelper::DebugPrint("Sponza 초기화 실패");
        return false;
    }

    m_psoSolidCull = params.psoSolidCull;
    m_psoSolidNoCull = params.psoSolidNoCull;
    m_psoWireCull = params.psoWireCull;
    m_psoWireNoCull = params.psoWireNoCull;
    return true;
} // Init

void Sponza::Submit(RenderQueue* renderQueue) {
    if (!renderQueue) {
        return;
    }

    //DebugHelper::DebugPrint("MaterialIndicesIndex = " + std::to_string(RendererState::MaterialIndicesIndex));
    //DebugHelper::DebugPrint("BindlessTexIndex = " + std::to_string(RendererState::BindlessTexIndex));
    for (const auto& mesh : m_meshes) {
        if (!mesh) {
            continue;
        }

        bool isVase = false;
        std::string meshName = mesh->GetName();
        //OutputDebugStringA(("Mesh Name: " + meshName + "\n").c_str());

        if (meshName.find("vase") != std::string::npos ||
            meshName.find("leaf") != std::string::npos ||
            meshName.find("Material__57") != std::string::npos) {
            isVase = true;
        }

        ID3D12PipelineState* targetPSO = nullptr;
        if (m_enableWireframe) {
            targetPSO = isVase ? m_psoWireNoCull : m_psoWireCull;
        }
        else {
            targetPSO = isVase ? m_psoSolidNoCull : m_psoSolidCull;
        }

        DrawCommand cmd{};
        cmd.sortKey = 0;
        cmd.pso = targetPSO;

        cmd.execute = [this, meshPtr = mesh.get()](ID3D12GraphicsCommandList* cmdList) {
            DirectX::XMMATRIX transposedWorld = XMMatrixTranspose(m_worldMatrix);

            cmdList->SetGraphicsRoot32BitConstants(
                RendererState::WorldIndex,
                sizeof(XMMATRIX) / 4,
                &transposedWorld,
                0
            );

            UINT vbIndex = meshPtr->GetVertexBufferSRVIndex();
            cmdList->SetGraphicsRoot32BitConstants(RendererState::VertexBufferIndexParam, 1, &vbIndex, 0);

            unsigned int materialIndex = meshPtr->GetMaterialIndex();
            if (materialIndex < m_materials.size()) {
                const auto& material = m_materials[materialIndex];

                UINT matIndices[4] = {
                    material.albedo->GetSRVIndex(),
                    material.normal->GetSRVIndex(),
                    material.alpha->GetSRVIndex(),
                    0 // padding
                };
                cmdList->SetGraphicsRoot32BitConstants(RendererState::MaterialIndicesIndex, 4, matIndices, 0);
            } // if (materialIndex < m_materials.size())

            meshPtr->Render(cmdList);
        }; //  cmd.execute = [this, meshPtr = mesh.get()](ID3D12GraphicsCommandList* cmdList)

        renderQueue->Submit(cmd);
    } // for (const auto& mesh : m_meshes)
} // Submit

const DirectX::XMMATRIX& Sponza::GetWorldMatrix() const {
    return m_worldMatrix;
} // GetWorldMatrix

void Sponza::OnGUI() {
    ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.6f, 1.0f), "[ Sponza Options ]");
    ImGui::Checkbox("Wireframe Mode", &m_enableWireframe);

    ImGui::Separator();
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