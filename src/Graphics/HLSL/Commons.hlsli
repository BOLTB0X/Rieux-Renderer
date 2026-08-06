// Commons.hlsli
#ifndef _COMMONS_HLSLI_
#define _COMMONS_HLSLI_

struct Frame
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
}; // Frame

ConstantBuffer<Frame> g_FrameCB : register(b0);

#define VIEW              g_FrameCB.view
#define PROJ              g_FrameCB.projection
#define VIEW_INV          g_FrameCB.viewInv
#define PROJ_INV          g_FrameCB.projInv
#define CAMERA_POSITION   g_FrameCB.cameraPosition
#define CAMERA_FOV        g_FrameCB.cameraFov
#define SCREEN_RESOLUTION g_FrameCB.screenResolution
#define TIME              g_FrameCB.time

struct DirectionalLight
{
    float3 lightDir;
    float  lPadding1;
    
    float4 lightAmbient;
    
    float4 lightDiffuse;
    
    float3 lightLookAt;
    float  lPadding2;
    
    matrix lightViewMatrix;
    
    matrix lightProjectionMatrix;
    
    float  shadowMapWidth;
    float  shadowMapHeight;
    float  shadowBias;
    float  shadowSpread;
    
    float4 lPadding3;
}; // DirectionalLight

ConstantBuffer<DirectionalLight> g_DirectionalLightCB : register(b1);

#define LIGHT_DIRECTION   g_DirectionalLightCB.lightDir
#define LIGHT_AMBIENT     g_DirectionalLightCB.lightAmbient
#define LIGHT_DIFFUSE     g_DirectionalLightCB.lightDiffuse
#define LIGHT_LOOKAT      g_DirectionalLightCB.lightLookAt
#define LIGHT_VIEW        g_DirectionalLightCB.lightViewMatrix
#define LIGHT_PROJ        g_DirectionalLightCB.lightProjectionMatrix

#define SHADOW_MAP_SIZE   float2(g_DirectionalLightCB.shadowMapWidth, g_DirectionalLightCB.shadowMapHeight)
#define SHADOW_BIAS       g_DirectionalLightCB.shadowBias
#define SHADOW_SPREAD     g_DirectionalLightCB.shadowSpread

#endif // _COMMONS_HLSLI_