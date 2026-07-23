#include "Pch.h"
#include "StaticSponza.h"
// Core
#include "RenderQueue.h"
// Resources
#include "Data/PBRMesh.h"
#include "Data/Texture.h"
// Utils
#include "ShaderHelper.h"
#include "DebugHelper.h"
#include "SharedCommons.h"

using namespace Microsoft::WRL;
using namespace DirectX;

StaticSponza::StaticSponza() {
    XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
} // StaticModel

StaticSponza::~StaticSponza() {
} // ~StaticModel

bool StaticSponza::Init(const InitParams& params) {
    if (!AssimpModel::Init(params)) {
        DebugHelper::DebugPrint("StaticSponza 모델 로드 실패");
        return false;
    }

    if (!BuildPSO(params.device, params.rootSig)) {
        DebugHelper::DebugPrint("StaticSponza PSO 생성 실패");
        return false;
    }

    return true;
}

void StaticSponza::Submit(RenderQueue* renderQueue) {
    if (!renderQueue) return;

    // 부모 클래스(AssimpModel)의 protected 멤버인 m_meshes에 직접 접근 가능!
    for (const auto& mesh : m_meshes) {
        if (!mesh) continue;

        DrawCommand cmd{};

        // TODO: 불투명/반투명 분류에 따라 sortKey 설정 로직 추가 가능
        cmd.sortKey = 0;

        // 2단계 스텝에서 적용할 PSO 세팅용 (현재는 nullptr)
        cmd.pso = nullptr;

        cmd.execute = [this, meshPtr = mesh.get()](ID3D12GraphicsCommandList* cmdList) {
            // 1. 여기서 모델의 m_worldMatrix를 상수 버퍼(CBV)나 
            //    RootConstants(32비트 상수)로 GPU에 넘겨주는 작업이 필요합니다.

            // 2. 메시 그리기 호출
            meshPtr->Render(cmdList); // PBRMesh에 Draw 함수가 구현되어 있다고 가정
        };

        renderQueue->Submit(cmd);
    }
} // Submit

const XMFLOAT4X4& StaticSponza::GetWorldMatrix() const { return m_worldMatrix; }

bool StaticSponza::BuildPSO(ID3D12Device* device, ID3D12RootSignature* rootSig) {
    ComPtr<IDxcBlob> vertexShader;
    ComPtr<IDxcBlob> pixelShader;

    // ShaderHelper를 사용해 DXC로 셰이더 컴파일 (엔트리 포인트는 기본값 L"main" 사용)
    // TODO: 실제 셰이더 파일 경로로 수정해주세요.
    if (!ShaderHelper::InitVertexShader(SharedCommons::PBR_VS, &vertexShader)) {
        return false;
    }
    if (!ShaderHelper::InitPixelShader(SharedCommons::SPONZA_PS, &pixelShader)) {
        return false;
    }

    // AssimpLoader의 PBRVertex에 맞춘 인풋 레이아웃
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // PSO 서술자 설정
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = rootSig;

    // IDxcBlob에서 바이트코드 포인터와 크기를 가져와 DX12 파이프라인에 주입
    psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
    psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };

    // 래스터라이저, 블렌드, 뎁스 스텐실 등 기본 상태 세팅
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    // 뎁스 테스트를 켠다면 활성화
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    // Renderer.cpp에서 사용중인 RTV 포맷과 반드시 일치해야 함
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    // DSV(Depth Stencil View) 포맷 (렌더러 쪽에 뎁스 버퍼가 추가된다면 여기에 명시해야 합니다)
    // psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT; 
    psoDesc.SampleDesc.Count = 1;

    // PSO 최종 생성
    if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)))) {
        DebugHelper::DebugPrint("CreateGraphicsPipelineState 실패");
        return false;
    }

    return true;
} // BuildPSO