// ShadowMap.hlsli
#ifndef _SHADOW_MAP_HLSLI_
#define _SHADOW_MAP_HLSLI_
#include "Commons.hlsli"

Texture2D<float> g_ShadowMap : register(t2, space0);
SamplerState     ShadowSampler : register(s1);

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

    float2 texelSize = SHADOW_SPREAD / SHADOW_MAP_SIZE;
    float normalDotLight = saturate(dot(normalWS, -LIGHT_DIRECTION));
    float bias = SHADOW_BIAS * (1.0f + (1.0f - normalDotLight) * 2.0f);
    float shadow = 0.0f;

    [unroll]
    for (int y = -2; y <= 2; ++y)
    {
        [unroll]
        for (int x = -2; x <= 2; ++x)
        {
            float2 sampleUV = shadowUV + float2(x, y) * texelSize;
            float shadowDepth = g_ShadowMap.SampleLevel(ShadowSampler, sampleUV, 0.0f);
            shadow += lightNDC.z <= shadowDepth + bias ? 1.0f : 0.0f;
        }
    }

    return shadow / 25.0f;
} // CalculateShadow

#endif // _SHADOW_MAP_HLSLI_
