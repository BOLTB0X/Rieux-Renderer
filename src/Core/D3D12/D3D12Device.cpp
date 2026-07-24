#include "Pch.h"
#include "D3D12Device.h"
// Utils
#include "DebugHelper.h"

using namespace Microsoft::WRL;

bool D3D12Device::Init() {
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)))) {
        DebugHelper::DebugPrint("DXGI 팩토리 생성 실패");
        return false;
    }

    ComPtr<IDXGIAdapter1> adapter1;
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &adapter1); ++adapterIndex) {
        DXGI_ADAPTER_DESC1 desc;
        adapter1->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }
        if (SUCCEEDED(D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)))) {
            break;
        }
    }

    if (!m_device) {
        DebugHelper::DebugPrint("사용 가능한 DX12 호환 그래픽 장치를 찾지 못함");
        return false;
    }

    adapter1.As(&m_iDXGIAdapter3);
    return true;
} // Init

ID3D12Device*  D3D12Device::GetDevice() const { return m_device.Get(); }
IDXGIAdapter3* D3D12Device::GetAdapter() const { return m_iDXGIAdapter3.Get(); }