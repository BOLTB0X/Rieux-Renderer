#include "Pch.h"
#include "AssimpModel.h"
#include "Texture.h"
#include "PBRMesh.h"
#include "Tool/AssimpLoader.h"
#include "Managers/TextureManager.h"

AssimpModel::AssimpModel() {
} // AssimpModel

AssimpModel::~AssimpModel() {
} // ~AssimpModel

bool AssimpModel::Init(const InitParams& params) {
    m_meshes.clear();
    m_materials.clear();

    m_AssimpLoader = std::make_unique<AssimpLoader>(params.textureManager);

    AssimpLoader::LoadParams loadParams;
    loadParams.device = params.device;
    loadParams.commandQueue = params.commandQueue;
    loadParams.path = params.path;
    loadParams.outModel = this;
    loadParams.heapAllocator = params.heapAllocator;

    return m_AssimpLoader->LoadMeshModel(loadParams);
} // Init

int AssimpModel::GetMeshCount() const {
    return static_cast<int>(m_meshes.size());
} // GetMeshCount

std::vector<AssimpModel::MaterialInfo> AssimpModel::GetMaterialInfos() const {
    std::vector<MaterialInfo> infos;
    for (const auto& mat : m_materials) {
        MaterialInfo info;
        info.name = mat.name;
        info.hasAlbedo = (mat.albedo != nullptr);
        info.hasNormal = (mat.normal != nullptr);
        info.hasMetallic = (mat.metallic != nullptr);
        info.hasRoughness = (mat.roughness != nullptr);
        info.hasAO = (mat.ao != nullptr);
        info.hasAlpha = (mat.alpha != nullptr);
        info.hasSpecular = (mat.specular != nullptr);
        info.hasEmissive = (mat.emissive != nullptr);
        info.hasDisplacement = (mat.displacement != nullptr);
        info.hasSubsurface = (mat.subsurface != nullptr);
        info.hasSmoothness = (mat.smoothness != nullptr);
        infos.push_back(info);
    }
    return infos;
} // GetMaterialInfos

void AssimpModel::AddMesh(std::unique_ptr<PBRMesh> newMesh) {
    if (newMesh) {
        m_meshes.push_back(std::move(newMesh));
    }
} // AddMesh

void AssimpModel::AddMaterial(const Material& material) {
    m_materials.push_back(material);
} // AddMaterial

const PBRMesh* AssimpModel::GetMesh(int index) const {
    if (index < 0 || index >= static_cast<int>(m_meshes.size())) {
        return nullptr;
    }
    return m_meshes[index].get();
} // GetMesh

const AssimpModel::Material& AssimpModel::GetMaterial(int index) const {
    return m_materials[index];
} // GetMaterial