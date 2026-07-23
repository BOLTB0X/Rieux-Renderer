#pragma once
#include <directxmath.h>
#include "SharedCommons.h"

namespace SharedCBs {

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
        DirectX::XMFLOAT4 padding3;

        DirectionalLightCB() :
            direction(0.0f, -1.0f, 0.0f), padding1(0.0f),
            ambient(0.2f, 0.2f, 0.2f, 1.0f),
            diffuse(1.0f, 1.0f, 1.0f, 1.0f),
            lookAt(0.0f, 0.0f, 0.0f), padding2(0.0f),
            lightViewMatrix(DirectX::XMMatrixIdentity()),
            lightProjectionMatrix(DirectX::XMMatrixIdentity()),
            shadowMapWidth(0.0f),
            shadowMapHeight(0.0f), shadowBias(0.0f), shadowSpread(0.0f),
            padding3(0.0f, 0.0f, 0.0f, 0.0f) {
        }
    }; // DirectionalLightCB
} // SharedCBs