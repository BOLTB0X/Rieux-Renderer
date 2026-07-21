// Commons.hlsli
#ifndef _COMMONS_HLSLI_
#define _COMMONS_HLSLI_

cbuffer FrameCB : register(b0)
{
    matrix view;
    matrix projection;
    matrix viewInv;
    matrix projInv;
    float3 cameraPosition;
    float  cameraFov;
    float2 screenResolution;
    float  time;
    float  framePadding;
}; // FrameCB

cbuffer DirectionalLightCB : register(b1)
{
    float3 lightDir;
    float  lightPadding1;
    float4 lightAmbient;
    float4 lightDiffuse;
    float3 lightLookAt;
    float  lightPadding2;
    matrix lightViewMatrix;
    matrix lightProjectionMatrix;
    float  shadowMapWidth;
    float  shadowMapHeight;
    float  shadowBias;
    float  shadowSpread;
    float4 lightPadding3;
}; // DirectionalLightCB

cbuffer WorldCB : register(b2)
{
    matrix world;
}; // WorldCB

#endif // _COMMONS_HLSLI_