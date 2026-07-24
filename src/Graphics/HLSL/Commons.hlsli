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

#define VIEW              view
#define PROJ              projection
#define VIEW_INV          viewInv
#define PROJ_INV          projInv
#define CAMERA_POSITION   cameraPosition
#define CAMERA_FOV        cameraFov
#define SCREEN_RESOLUTION screenResolution
#define TIME              time

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

#define LIGHT_DIRECTION   lightDir
#define LIGHT_AMBIENT     lightAmbient
#define LIGHT_DIFFUSE     lightDiffuse
#define LIGHT_LOOKAT      lightLookAt
#define LIGHT_VIEW        lightViewMatrix
#define LIGHT_PROJ        lightProjectionMatrix

#define SHADOW_MAP_SIZE   float2(shadowMapWidth, shadowMapHeight)
#define SHADOW_BIAS       shadowBias
#define SHADOW_SPREAD     shadowSpread


#endif // _COMMONS_HLSLI_