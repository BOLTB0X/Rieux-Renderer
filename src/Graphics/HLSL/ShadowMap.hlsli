// ShadowMap.hlsli
#ifndef _SHADOW_MAP_HLSLI_
#define _SHADOW_MAP_HLSLI_
#include "Commons.hlsli"

Texture2D<float>      g_ShadowMap : register(t2, space0);
Texture2DArray<float> g_CSMShadowMap : register(t3, space0);
SamplerState          g_ShadowSampler : register(s1);

static const float2 poisson_disk[16] =
{
    float2(-0.94201624f, -0.39906216f), float2(0.94558609f, -0.76890725f),
    float2(-0.09418410f, -0.92938870f), float2(0.34495938f, 0.29387760f),
    float2(-0.91588581f,  0.45771432f), float2(-0.81544232f, -0.87912464f),
    float2(-0.38190847f,  0.37932123f), float2(0.97351111f, 0.45599420f),
    float2(0.53742981f, -0.47373420f), float2(0.23497881f, -0.96788225f),
    float2(-0.82544401f, -0.12328243f), float2(0.71355652f, 0.17178331f),
    float2(0.39912361f, -0.27312111f), float2(0.99955401f, -0.61206121f),
    float2(-0.57321611f,  0.77884112f), float2(-0.31211321f,  0.55621321f)
};

float Calculate_Shadow(float3 positionWS, float3 normalWS)
{
    float4 lightClipPosition = mul(float4(positionWS, 1.0f), LIGHT_VIEW);
    lightClipPosition = mul(lightClipPosition, LIGHT_PROJ);

    if (lightClipPosition.w <= 0.0f)
        return 1.0f;

    float3 lightNDC = lightClipPosition.xyz / lightClipPosition.w;
    if (lightNDC.z < 0.0f || lightNDC.z > 1.0f)
        return 1.0f;

    float2 shadowUV = lightNDC.xy * float2(0.5f, -0.5f) + 0.5f;
    if (any(shadowUV < 0.0f) || any(shadowUV > 1.0f))
        return 1.0f;

    float2 texelSize = 2.0f * SHADOW_SPREAD / SHADOW_MAP_SIZE;
    float normalDotLight = saturate(dot(normalWS, -LIGHT_DIRECTION));
    float bias = SHADOW_BIAS * (1.0f + (1.0f - normalDotLight) * 2.0f);
    float shadow = 0.0f;
    [unroll]
    for (int i = 0; i < 16; ++i)
    {
        float2 sampleUV = shadowUV + poisson_disk[i] * texelSize;
        float shadowDepth = g_ShadowMap.SampleLevel(g_ShadowSampler, sampleUV, 0.0f);
        shadow += lightNDC.z <= shadowDepth + bias ? 1.0f : 0.0f;
    }

    return shadow / 16.0f;
} // CalculateShadow

int SelectCascade(float viewSpaceDepth)
{
    [loop]
    for (uint i = 0; i < CASCADE_COUNT - 1; ++i)
    {
        if (viewSpaceDepth < CASCADE_SPLITS[i])
            return i;
    }
    return CASCADE_COUNT - 1;
}

float Sample_CascadeShadow(float3 positionWS, float3 normalWS, int cascade)
{
    float4 lightClipPosition = mul(float4(positionWS, 1.0f), CASCADE_VIEW_PROJ[cascade]);
    if (lightClipPosition.w <= 0.0f)
        return 1.0f;

    float3 lightNDC = lightClipPosition.xyz / lightClipPosition.w;
    if (lightNDC.z < 0.0f || lightNDC.z > 1.0f)
        return 1.0f;

    float2 shadowUV = lightNDC.xy * float2(0.5f, -0.5f) + 0.5f;
    if (any(shadowUV < 0.0f) || any(shadowUV > 1.0f))
        return 1.0f;

    float2 texelSize = 2.0f * SHADOW_SPREAD / SHADOW_MAP_SIZE;
    float normalDotLight = saturate(dot(normalWS, -LIGHT_DIRECTION));
    float bias = SHADOW_BIAS * (1.0f + (1.0f - normalDotLight) * 2.0f);

    float shadow = 0.0f;
    [unroll]
    for (int i = 0; i < 16; ++i)
    {
        float2 sampleUV = shadowUV + poisson_disk[i] * texelSize;
        float shadowDepth = g_CSMShadowMap.SampleLevel(g_ShadowSampler, float3(sampleUV, cascade), 0.0f);
        shadow += lightNDC.z <= shadowDepth + bias ? 1.0f : 0.0f;
    }
    return shadow / 16.0f;
}

float Calculate_CascadeShadow(float3 positionWS, float3 normalWS, float viewSpaceDepth)
{
    int cascade = SelectCascade(viewSpaceDepth);
    float shadow = Sample_CascadeShadow(positionWS, normalWS, cascade);

    if (cascade >= CASCADE_COUNT - 1)
        return shadow;

    float split = CASCADE_SPLITS[cascade];
    float previousSplit = cascade == 0 ? 0.0f : CASCADE_SPLITS[cascade - 1];
    float blendRange = (split - previousSplit) * 0.10f;
    float blendStart = split - blendRange;

    if (viewSpaceDepth <= blendStart)
        return shadow;

    float nextShadow = Sample_CascadeShadow(positionWS, normalWS, cascade + 1);
    float blend = saturate((viewSpaceDepth - blendStart) / blendRange);
    return lerp(shadow, nextShadow, blend);
} // Calculate_CascadeShadow

#endif // _SHADOW_MAP_HLSLI_
