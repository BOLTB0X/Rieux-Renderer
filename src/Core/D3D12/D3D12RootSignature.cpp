#include "Pch.h"
#include "D3D12RootSignature.h"
// Core
#include "RendererState.h"
// Tools
#include "RootSignatureBuilder.h"
// Utils
#include "DebugHelper.h"

using namespace Microsoft::WRL;

bool D3D12RootSignature::Init(const InitParams& params) {
    if (!params.device) {
        DebugHelper::DebugPrint("D3D12RootSignature 초기화 실패");
        return false;
    }

    const auto& b = params.builder;
    D3D12_ROOT_SIGNATURE_DESC desc = {
        static_cast<UINT>(b.m_rootParameters.size()), b.m_rootParameters.data(),
        static_cast<UINT>(b.m_staticSamplers.size()), b.m_staticSamplers.data(),
        b.m_flags
    };

    Microsoft::WRL::ComPtr<ID3DBlob> signature, error;
    if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
        if (error) {
            DebugHelper::DebugPrint(static_cast<const char*>(error->GetBufferPointer()));
        }
        return false;
    }
    if (FAILED(params.device->CreateRootSignature(0, signature->GetBufferPointer(),
        signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)))) {
        return false;
    }

    m_paramIndexMap = b.m_paramIndexByTag;
    return true;
} // Init

ID3D12RootSignature* D3D12RootSignature::GetRootSignature() const {
	return m_rootSignature.Get();
} // GetID3D12RootSignature

UINT D3D12RootSignature::GetParamIndex(const std::string& tag) const {
    auto it = m_paramIndexMap.find(tag);
    if (it == m_paramIndexMap.end()) {
        DebugHelper::DebugPrint("루트 파라미터 태그를 찾을 수 없음: " + tag);
        return UINT_MAX;
    }
    return it->second;
} // GetParamIndex