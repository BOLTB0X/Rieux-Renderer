#include "Pch.h"
#include "D3D12RootSignature.h"
// Core
#include "RendererState.h"
// Utils
#include "DebugHelper.h"

using namespace Microsoft::WRL;

bool D3D12RootSignature::Init(const InitParams& params) {
	if (!params.device) {
		DebugHelper::DebugPrint("D3D12RootSignature 초기화 실패");
		return false;
	}

	// 각 텍스처 슬롯(t0, t1, t2)을 위한 독립된 레인지 설정 (개수 1개씩)
	D3D12_DESCRIPTOR_RANGE albedoRange = { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }; // t0
	D3D12_DESCRIPTOR_RANGE normalRange = { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }; // t1
	D3D12_DESCRIPTOR_RANGE alphaRange = { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }; // t2

	// 루트 파라미터 구성 (총 6개)
	D3D12_ROOT_PARAMETER rootParameters[6] = {};

	// [0] Frame CB (b0)
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].Descriptor.ShaderRegister = 0;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// [1] Light CB (b1)
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].Descriptor.ShaderRegister = 1;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// [2] World Matrix (b2) - 32비트 루트 상수
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParameters[2].Constants.ShaderRegister = 2;
	rootParameters[2].Constants.Num32BitValues = 16;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// [3] Albedo Tex (t0 슬롯용 테이블)
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[3].DescriptorTable.pDescriptorRanges = &albedoRange;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// [4] Normal Tex (t1 슬롯용 테이블)
	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[4].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[4].DescriptorTable.pDescriptorRanges = &normalRange;
	rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// [5] Alpha Tex (t2 슬롯용 테이블)
	rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[5].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[5].DescriptorTable.pDescriptorRanges = &alphaRange;
	rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// 스태틱 샘플러 설정 (s0)
	D3D12_STATIC_SAMPLER_DESC staticSampler = {};
	staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
	staticSampler.ShaderRegister = 0;
	staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// 루트 시그니처 직렬화 및 생성
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = { _countof(rootParameters), rootParameters, 1, &staticSampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT };

	Microsoft::WRL::ComPtr<ID3DBlob> signature, error;
	if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
		return false;
	}
	if (FAILED(params.device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)))) {
		return false;
	}

	return true;
}

ID3D12RootSignature* D3D12RootSignature::GetRootSignature() const {
	return m_rootSignature.Get();
} // GetRootSignature
