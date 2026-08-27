#pragma once
#include <directxmath.h>
#include "SharedCommons.h"

namespace GPUCommons {
    struct VertexData {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT2 texCoord;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT3 tangent;
        DirectX::XMFLOAT3 binormal;
    }; // VertexData

    struct GPUMaterialData {
        DirectX::XMFLOAT4 albedoFactor;

        float             metallicFactor;
        float             roughnessFactor;
        float             emissiveFactor;
        float             alphaCutoff;

        int               albedoTexIdx;
        int               normalTexIdx;
        int               metallicTexIdx;
        int               roughnessTexIdx;

        int               alphaTexIdx;
        int               pad0;
        int               pad1;
        int               pad2;

        GPUMaterialData()
            : albedoFactor(SharedCommons::ALBEDO_FACTOR),
            metallicFactor(SharedCommons::METALLIC_FACTOR),
            roughnessFactor(SharedCommons::ROUGH_FACTOR),
            emissiveFactor(SharedCommons::EMISS_FACTOR),
            alphaCutoff(0.5f),
            albedoTexIdx(-1), normalTexIdx(-1), metallicTexIdx(-1), roughnessTexIdx(-1),
            alphaTexIdx(-1), pad0(0), pad1(0), pad2(0) {
        }
    }; // GPUMaterialData

    struct GPUMeshData {
        uint32_t vertexOffset;
        uint32_t indexOffset;
        uint32_t indexCount;
        uint32_t materialIndex;

        GPUMeshData() : vertexOffset(0), indexOffset(0), indexCount(0), materialIndex(0) {
        }
    }; // GPUMeshData

    struct MeshInstanceData {
        DirectX::XMMATRIX worldMatrix;
        uint32_t          vertexBufferIndex;
        uint32_t          albedoIndex;
        uint32_t          normalIndex;
        uint32_t          alphaIndex;

        DirectX::XMFLOAT3 aabbMin;
        float             padding1;
        DirectX::XMFLOAT3 aabbMax;
        float             padding2;

        float             isVase;
        uint32_t          metallicIndex;
        uint32_t          roughnessIndex;
        float             metallicFactor;

        float             roughnessFactor;
        uint32_t          shadowIndexCount;
        uint32_t          shadowStartIndex;
        float             padding3;

        DirectX::XMFLOAT4 albedoFactor;

        MeshInstanceData()
            : worldMatrix(DirectX::XMMatrixIdentity()),
            vertexBufferIndex(0),
            albedoIndex(0), normalIndex(0), alphaIndex(0),
            aabbMin(0.0f, 0.0f, 0.0f), padding1(0.0f),
            aabbMax(0.0f, 0.0f, 0.0f), padding2(0.0f),
            isVase(0.0f), metallicIndex(UINT_MAX), roughnessIndex(UINT_MAX),
            metallicFactor(SharedCommons::METALLIC_FACTOR),
            roughnessFactor(SharedCommons::ROUGH_FACTOR),
            shadowIndexCount(0), shadowStartIndex(0), padding3(0.0f),
            albedoFactor(SharedCommons::ALBEDO_FACTOR) {
		}
    }; // MeshInstanceData

    static_assert(sizeof(MeshInstanceData) == 160, "MeshInstanceData layout mismatch");

    struct GPUIndexBufferView {
        uint32_t address[2];
        uint32_t size;
        uint32_t format;

        GPUIndexBufferView() 
            : address{ 0, 0 }, size(0), format(0) {
		}
    }; // GPUIndexBufferView

    struct IndirectCommand {
        D3D12_INDEX_BUFFER_VIEW      indexBufferView;
        UINT                         instanceIndex;
        D3D12_DRAW_INDEXED_ARGUMENTS drawArgs;

        IndirectCommand() : indexBufferView{}, instanceIndex(0), drawArgs{} {
        }
    }; // IndirectCommand

} // struct

namespace GPUCommons {
    static const UINT MAX_CASCADES = 4;

    __declspec(align(256)) struct MatrixCB {
        DirectX::XMMATRIX world;
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX projection;

        MatrixCB()
            : world(DirectX::XMMatrixIdentity()),
            view(DirectX::XMMatrixIdentity()),
            projection(DirectX::XMMatrixIdentity()) {
        }
    }; // MatrixCB

    __declspec(align(256)) struct WorldCB {
        DirectX::XMMATRIX world;

        WorldCB() {
            world = DirectX::XMMatrixIdentity();
        }
    }; // WorldCB

    __declspec(align(256)) struct FrameCB {
        DirectX::XMMATRIX view;

        DirectX::XMMATRIX projection;

        DirectX::XMMATRIX viewInv;

        DirectX::XMMATRIX projInv;

        DirectX::XMFLOAT3 cameraPosition;
        float             cameraFov;

        DirectX::XMFLOAT2 screenResolution;
        float             time;
        float             padding;

        FrameCB() :
            view(DirectX::XMMatrixIdentity()),
            projection(DirectX::XMMatrixIdentity()),
            viewInv(DirectX::XMMatrixIdentity()),
            projInv(DirectX::XMMatrixIdentity()),
            cameraPosition(0.0f, 0.0f, 0.0f), cameraFov(0.0f),
            screenResolution((float)SharedCommons::SCREEN_WIDTH, (float)SharedCommons::SCREEN_HEIGHT),
            time(0.0f), padding(0.0f) {
        }
    }; // FrameCB

    __declspec(align(256)) struct DirectionalLightCB {
        DirectX::XMFLOAT3 direction;
        float             padding1;

        DirectX::XMFLOAT4 ambient;

        DirectX::XMFLOAT4 diffuse;

        DirectX::XMFLOAT3 lookAt;
        float             padding2;

        DirectX::XMMATRIX lightViewMatrix;

        DirectX::XMMATRIX lightProjectionMatrix;

        float             shadowMapWidth;
        float             shadowMapHeight;
        float             shadowBias;
        float             shadowSpread;

        DirectX::XMMATRIX cascadeViewProj[MAX_CASCADES];

        DirectX::XMFLOAT4 cascadeSplits;

        UINT              cascadeCount;
        DirectX::XMFLOAT3 padding3;

        DirectionalLightCB() :
            direction(0.0f, -1.0f, 0.0f), padding1(0.0f),
            ambient(0.2f, 0.2f, 0.2f, 1.0f),
            diffuse(1.0f, 1.0f, 1.0f, 1.0f),
            lookAt(0.0f, 0.0f, 0.0f), padding2(0.0f),
            lightViewMatrix(DirectX::XMMatrixIdentity()),
            lightProjectionMatrix(DirectX::XMMatrixIdentity()),
            shadowMapWidth(0.0f), shadowMapHeight(0.0f), shadowBias(0.0f), shadowSpread(0.0f),
            cascadeSplits(0, 0, 0, 0), cascadeCount(0), padding3(0.0f, 0.0f, 0.0f) {
            for (UINT i = 0; i < MAX_CASCADES; ++i) cascadeViewProj[i] = DirectX::XMMatrixIdentity();
        }
    }; // DirectionalLightCB

    __declspec(align(256)) struct FrustumCullingCB {
        DirectX::XMFLOAT4 planes[6];
        UINT              totalInstances;
        DirectX::XMFLOAT3 padding;
    }; // FrustumCullingCB

    __declspec(align(256)) struct OcclusionCullingCB {
        DirectX::XMMATRIX viewProj;
        float             screenWidth;
        float             screenHeight;
        uint32_t          mainCapacity;
        uint32_t          vaseCapacity;
        DirectX::XMFLOAT2 padding;

        OcclusionCullingCB() : viewProj(DirectX::XMMatrixIdentity())
            , screenWidth(0.0f), screenHeight(0.0f), mainCapacity(0), vaseCapacity(0), padding(0.0f, 0.0f) {
        }
    }; // OcclusionCullingCB
} // CBs
