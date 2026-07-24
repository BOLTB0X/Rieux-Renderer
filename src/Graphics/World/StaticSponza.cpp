#include "Pch.h"
#include "StaticSponza.h"
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

StaticSponza::StaticSponza() : AssimpModel() {
    m_worldMatrix = XMMatrixIdentity();
    m_pso = nullptr;
    m_heapAllocator = nullptr;
} // StaticModel

StaticSponza::~StaticSponza() {
    m_pso = nullptr;
    m_heapAllocator = nullptr;
} // ~StaticSponza

bool StaticSponza::Init(const InitParams& params) {
    if (!AssimpModel::Init(params)) {
        DebugHelper::DebugPrint("StaticSponza 모델 로드 실패");
        return false;
    }

    if (!params.pso) {
        DebugHelper::DebugPrint("StaticSponza 초기화 실패");
        return false;
    }

    m_pso = params.pso;
    m_heapAllocator = params.heapAllocator;
    return true;
} // Init

void StaticSponza::Submit(RenderQueue* renderQueue) {
    if (!renderQueue || !m_pso || !m_heapAllocator) {
        return;
    }

    for (const auto& mesh : m_meshes) {
        if (!mesh) continue;

        DrawCommand cmd{};
        cmd.sortKey = 0;
        cmd.pso = m_pso;

        cmd.execute = [this, meshPtr = mesh.get()](ID3D12GraphicsCommandList* cmdList) {
            DirectX::XMMATRIX transposedWorld = XMMatrixTranspose(m_worldMatrix);

            // 1. 월드 매트릭스 바인딩 (루트 파라미터 2번 - 상수)
            cmdList->SetGraphicsRoot32BitConstants(
                RendererState::WorldIndex,
                sizeof(XMMATRIX) / 4,
                &transposedWorld,
                0
            );

            unsigned int materialIndex = meshPtr->GetMaterialIndex();
            if (materialIndex < m_materials.size()) {
                const auto& material = m_materials[materialIndex];

                // 2. 알베도 텍스처 바인딩 (루트 파라미터 3번 - 테이블)
                if (material.albedo) {
                    UINT idx = material.albedo->GetSRVIndex();
                    if (idx != UINT_MAX) {
                        cmdList->SetGraphicsRootDescriptorTable(
                            RendererState::Tex0Index,
                            m_heapAllocator->GetGPUHandle(idx)
                        );
                    }
                }

                // 3. 노멀 텍스처 바인딩 (루트 파라미터 4번 - 테이블)
                if (material.normal) {
                    UINT idx = material.normal->GetSRVIndex();
                    if (idx != UINT_MAX) {
                        cmdList->SetGraphicsRootDescriptorTable(
                            RendererState::Tex1Index,
                            m_heapAllocator->GetGPUHandle(idx)
                        );
                    }
                }

                // 4. 알파 텍스처 바인딩 (루트 파라미터 5번 - 테이블)
                if (material.alpha) {
                    UINT idx = material.alpha->GetSRVIndex();
                    if (idx != UINT_MAX) {
                        cmdList->SetGraphicsRootDescriptorTable(
                            RendererState::Tex2Index,
                            m_heapAllocator->GetGPUHandle(idx)
                        );
                    }
                }
            }

            meshPtr->Render(cmdList);
            };

        renderQueue->Submit(cmd);
    }
} // Submit

const DirectX::XMMATRIX& StaticSponza::GetWorldMatrix() const {
    return m_worldMatrix;
} // GetWorldMatrix