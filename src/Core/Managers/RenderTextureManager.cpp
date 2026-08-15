#include "Pch.h"
#include "RenderTextureManager.h"
// Components
#include "DescriptorHeapAllocator.h"
// Utils
#include "DebugHelper.h"
#include "SharedCommons.h"

using namespace DebugHelper;

RenderTextureManager::RenderTextureManager()
    : m_device(nullptr), m_sharedDescriptorAllocator(nullptr) {
} // RenderTextureManager

RenderTextureManager::~RenderTextureManager() {
    m_device = nullptr;
} // ~RenderTextureManager

bool RenderTextureManager::Init(const InitParams& params) {
    if (!params.device || !params.sharedDescriptorAllocator) {
        DebugPrint("RenderTextureManager::Init - 잘못된 파라미터");
        return false;
    }

    m_device = params.device;
    m_sharedDescriptorAllocator = params.sharedDescriptorAllocator;

    m_rtvAllocator = std::make_unique<DescriptorHeapAllocator>();
    if (!m_rtvAllocator->Init(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, params.rtvCapacity, false)) {
        DebugPrint("RenderTextureManager - RTV 풀 생성 실패");
        return false;
    }

    m_dsvAllocator = std::make_unique<DescriptorHeapAllocator>();
    if (!m_dsvAllocator->Init(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, params.dsvCapacity, false)) {
        DebugPrint("RenderTextureManager - DSV 풀 생성 실패");
        return false;
    }

    auto m_depthTex = std::make_shared<RenderTexture>();

    RenderTexture::InitParams texParams;
    texParams.device = m_device;
    texParams.rtvAllocator = m_rtvAllocator.get();
    texParams.dsvAllocator = m_dsvAllocator.get();
    texParams.sharedDescriptorAllocator = m_sharedDescriptorAllocator;
    texParams.width = SharedCommons::SCREEN_WIDTH;
    texParams.height = SharedCommons::SCREEN_HEIGHT;
    texParams.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    texParams.type = RenderTexture::RenderTextureType::Depth;

    if (!m_depthTex->Init(texParams)) {
        DebugPrint("RenderTexture 생성 실패: " + SharedCommons::KEY_DEPTH_RENDER_TEXTURE);
        return nullptr;
    }

    m_renderTextures[SharedCommons::KEY_DEPTH_RENDER_TEXTURE] = m_depthTex;

    UINT maxDimension = std::max(SharedCommons::SCREEN_WIDTH, SharedCommons::SCREEN_HEIGHT);
    UINT hizMipLevels = static_cast<UINT>(std::floor(std::log2(maxDimension))) + 1;

    texParams.device = m_device;
    texParams.rtvAllocator = m_rtvAllocator.get();
    texParams.dsvAllocator = m_dsvAllocator.get();
    texParams.sharedDescriptorAllocator = m_sharedDescriptorAllocator;
    texParams.width = SharedCommons::SCREEN_WIDTH;
    texParams.height = SharedCommons::SCREEN_HEIGHT;
    texParams.type = RenderTexture::RenderTextureType::UAV;
	texParams.format = DXGI_FORMAT_R32_FLOAT;
	texParams.mipLevels = hizMipLevels;

    auto m_hzTex = std::make_shared<RenderTexture>();
    if (!m_hzTex->Init(texParams)) {
        DebugPrint("RenderTexture 생성 실패: " + SharedCommons::KEY_HIZ_DEPTH_RENDER_TEXTURE);
        return nullptr;
    }

    m_renderTextures[SharedCommons::KEY_HIZ_DEPTH_RENDER_TEXTURE] = m_hzTex;

    auto shadowTex = std::make_shared<RenderTexture>();
    RenderTexture::InitParams shadowParams;
    shadowParams.device = m_device;
    shadowParams.rtvAllocator = m_rtvAllocator.get();
    shadowParams.dsvAllocator = m_dsvAllocator.get();
    shadowParams.sharedDescriptorAllocator = m_sharedDescriptorAllocator;
    shadowParams.width = static_cast<UINT>(SharedCommons::SHADOWMAP_WIDTH);
    shadowParams.height = static_cast<UINT>(SharedCommons::SHADOWMAP_HEIGHT);
    shadowParams.type = RenderTexture::RenderTextureType::Depth;
    shadowParams.depthClearValue = 1.0f;

    if (!shadowTex->Init(shadowParams)) {
        DebugPrint("RenderTexture 생성 실패: " + SharedCommons::KEY_SHADOW_MAP_RENDER_TEXTURE);
        return false;
    }

    m_renderTextures[SharedCommons::KEY_SHADOW_MAP_RENDER_TEXTURE] = shadowTex;

    return true;
} // Init

std::shared_ptr<RenderTexture> RenderTextureManager::CreateRenderTexture(
    const std::string& name, UINT width, UINT height,
    RenderTexture::RenderTextureType type, DXGI_FORMAT format, UINT mipLevels) {

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_renderTextures.find(name);
    if (it != m_renderTextures.end()) {
        return it->second;
    }

    auto renderTexture = std::make_shared<RenderTexture>();

    RenderTexture::InitParams texParams;
    texParams.device = m_device;
    texParams.rtvAllocator = m_rtvAllocator.get();
    texParams.dsvAllocator = m_dsvAllocator.get();
    texParams.sharedDescriptorAllocator = m_sharedDescriptorAllocator;
    texParams.width = width;
    texParams.height = height;
    texParams.format = format;
    texParams.type = type;
    texParams.mipLevels = mipLevels;

    if (!renderTexture->Init(texParams)) {
        DebugPrint("RenderTexture 생성 실패: " + name);
        return nullptr;
    }

    m_renderTextures[name] = renderTexture;
    return renderTexture;
} // CreateRenderTexture

std::shared_ptr<RenderTexture> RenderTextureManager::GetRenderTexture(const std::string& name) const {
    auto it = m_renderTextures.find(name);
    return (it != m_renderTextures.end()) ? it->second : nullptr;
} // GetRenderTexture

void RenderTextureManager::OnGUI() {
    ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.6f, 1.0f), "[ Render Textures ]");
    ImGui::Separator();

    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& [name, tex] : m_renderTextures) {
        if (!tex || tex->GetType() != RenderTexture::RenderTextureType::Normal) continue;

        UINT srvIndex = tex->GetSRVIndex();
        if (srvIndex == UINT_MAX) continue;

        ImGui::Text("%s (%ux%u)", name.c_str(), tex->GetWidth(), tex->GetHeight());

        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_sharedDescriptorAllocator->GetGPUHandle(srvIndex);

        float aspect = static_cast<float>(tex->GetHeight()) / static_cast<float>(tex->GetWidth());
        float previewWidth = 320.0f;
        ImGui::Image((ImTextureID)gpuHandle.ptr, ImVec2(previewWidth, previewWidth * aspect));
        ImGui::Spacing();
    }
} // OnGUI
