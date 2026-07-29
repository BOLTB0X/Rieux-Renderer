#include "Pch.h"
#include "D3D12RootSignature.h"
// Core
#include "RendererState.h"
// Utils
#include "DebugHelper.h"

using namespace Microsoft::WRL;

D3D12RootSignature::Builder& D3D12RootSignature::Builder::AddCBV(const std::string& tag,
    UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility) {
    m_paramIndexByTag[tag] = static_cast<UINT>(m_rootParameters.size());

    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.ShaderVisibility = visibility;

    m_rootParameters.push_back(param);
    return *this;
} // AddCBV

D3D12RootSignature::Builder& D3D12RootSignature::Builder::AddConstants(const std::string& tag,
    UINT shaderRegister, UINT num32BitValues, D3D12_SHADER_VISIBILITY visibility) {
    m_paramIndexByTag[tag] = static_cast<UINT>(m_rootParameters.size());

    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    param.Constants.ShaderRegister = shaderRegister;
    param.Constants.Num32BitValues = num32BitValues;
    param.ShaderVisibility = visibility;

    m_rootParameters.push_back(param);
    return *this;
} // AddConstants

D3D12RootSignature::Builder& D3D12RootSignature::Builder::AddSRV(const std::string& tag,
    UINT shaderRegister, UINT registerSpace, D3D12_SHADER_VISIBILITY visibility) {
    m_paramIndexByTag[tag] = static_cast<UINT>(m_rootParameters.size());

    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.Descriptor.RegisterSpace = registerSpace;
    param.ShaderVisibility = visibility;

    m_rootParameters.push_back(param);
    return *this;
} // AddSRV

D3D12RootSignature::Builder& D3D12RootSignature::Builder::AddSRVTable(const std::string& tag,
    UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility, UINT numDescriptors, UINT registerSpace) {
    m_paramIndexByTag[tag] = static_cast<UINT>(m_rootParameters.size());

    D3D12_DESCRIPTOR_RANGE range = {
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV, numDescriptors, shaderRegister, registerSpace,
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
    };
    m_descriptorRanges.push_back(range);

    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges = &m_descriptorRanges.back();
    param.ShaderVisibility = visibility;

    m_rootParameters.push_back(param);
    return *this;
} // AddSRVTable

D3D12RootSignature::Builder& D3D12RootSignature::Builder::AddStaticSampler(
    UINT shaderRegister, D3D12_FILTER filter,
    D3D12_TEXTURE_ADDRESS_MODE addressMode, D3D12_SHADER_VISIBILITY visibility) {
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = filter;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = addressMode;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = shaderRegister;
    sampler.ShaderVisibility = visibility;
    m_staticSamplers.push_back(sampler);

    return *this;
} // AddStaticSampler

D3D12RootSignature::Builder& D3D12RootSignature::Builder::SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags) {
    m_flags = flags;
    return *this;
} // SetFlags

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
} // GetRootSignature

UINT D3D12RootSignature::GetParamIndex(const std::string& tag) const {
    auto it = m_paramIndexMap.find(tag);
    if (it == m_paramIndexMap.end()) {
        DebugHelper::DebugPrint("루트 파라미터 태그를 찾을 수 없음: " + tag);
        return UINT_MAX;
    }
    return it->second;
} // GetParamIndex