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

    float3 probePosition;
    float  probeBlendDistance;
    float3 probeBoxMin;
    float  padding2;
    float3 probeBoxMax;
    float  padding3;
    
    float3 influenceBoxMin;
    float   padding4;
    float3 influenceBoxMax;
    float   padding5;
}; // Frame

ConstantBuffer<Frame> g_FrameCB : register(b0);

#define VIEW                 g_FrameCB.view
#define PROJ                 g_FrameCB.projection
#define VIEW_INV             g_FrameCB.viewInv
#define PROJ_INV             g_FrameCB.projInv
#define CAMERA_POSITION      g_FrameCB.cameraPosition
#define CAMERA_FOV           g_FrameCB.cameraFov
#define SCREEN_RESOLUTION    g_FrameCB.screenResolution
#define TIME                 g_FrameCB.time
#define PROBE_POSITION       g_FrameCB.probePosition
#define PROBE_BLEND_DISTANCE g_FrameCB.probeBlendDistance
#define PROBE_BOX_MIN        g_FrameCB.probeBoxMin
#define PROBE_BOX_MAX        g_FrameCB.probeBoxMax
#define PROBE_INFLUENCE_MIN  g_FrameCB.influenceBoxMin
#define PROBE_INFLUENCE_MAX  g_FrameCB.influenceBoxMax

#define MAX_CASCADES         4

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
    
    matrix cascadeViewProj[MAX_CASCADES];
    float4 cascadeSplits;
    uint   cascadeCount;
    float3 cPadding;
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

#define CASCADE_VIEW_PROJ g_DirectionalLightCB.cascadeViewProj
#define CASCADE_SPLITS    g_DirectionalLightCB.cascadeSplits
#define CASCADE_COUNT     g_DirectionalLightCB.cascadeCount

#define PI                3.14159265f

#endif // _COMMONS_HLSLI_