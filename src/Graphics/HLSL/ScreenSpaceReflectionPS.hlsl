// ScreenSpaceReflectionPS.hlsl
#include "Commons.hlsli"

Texture2D    LitSceneTex : register(t0);
Texture2D    DepthTex : register(t1);
Texture2D    GBuffer1 : register(t2);
SamplerState PointClampSampler : register(s0);
SamplerState LinearClampSampler : register(s1);

float3 GetViewPosition(float2 uv, float depth)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0f - uv.y) * 2.0f - 1.0f;
    float4 clipSpacePos = float4(x, y, depth, 1.0f);
    float4 viewSpacePos = mul(clipSpacePos, PROJ_INV);
    return viewSpacePos.xyz / viewSpacePos.w;
} // GetViewPosition

float3 GetViewNormal(float2 uv)
{
    float3 worldNormal = GBuffer1.SampleLevel(PointClampSampler, uv, 0).xyz * 2.0f - 1.0f;
    float3 viewNormal = mul(worldNormal, (float3x3) VIEW);
    return normalize(viewNormal);
} // GetViewNormal

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
}; // VS_OUT

float4 main(VS_OUT input) : SV_TARGET
{
    float2 uv = input.uv;

    float depth = DepthTex.SampleLevel(PointClampSampler, uv, 0).r;
    float3 sceneColor = LitSceneTex.SampleLevel(LinearClampSampler, uv, 0).rgb;

    if (depth <= 0.0f)
        return float4(sceneColor, 1.0f);

    float roughness = GBuffer1.SampleLevel(PointClampSampler, uv, 0).a;
    if (roughness > 0.35f)
        return float4(sceneColor, 1.0f);

    float3 viewNormal = GetViewNormal(uv);
    float3 viewPos = GetViewPosition(uv, depth);

    float3 viewDir = normalize(-viewPos);
    float3 reflectDir = normalize(reflect(-viewDir, viewNormal));

    if (reflectDir.z <= 0.0f || dot(reflectDir, viewNormal) <= 0.0f)
        return float4(sceneColor, 1.0f);

    float surfaceBias = max(0.01f, abs(viewPos.z) * 0.001f);
    float3 rayPos = viewPos + viewNormal * surfaceBias;

    const float initialStepSize = 0.5f;
    const float maxTraceDistance = initialStepSize * 64.0f;
    float stepSize = initialStepSize;
    float traveledDistance = 0.0f;
    int maxSteps = 64;

    bool hit = false;
    float2 hitUV = float2(0.0f, 0.0f);

    for (int i = 0; i < maxSteps; ++i)
    {
        rayPos += reflectDir * stepSize;
        traveledDistance += stepSize;

        if (traveledDistance > maxTraceDistance)
            break;

        float4 clipPos = mul(float4(rayPos, 1.0f), PROJ);
        if (clipPos.w <= 0.0f)
            break;

        float2 sampleUV = (clipPos.xy / clipPos.w) * 0.5f + 0.5f;
        sampleUV.y = 1.0f - sampleUV.y;

        if (sampleUV.x < 0.0f || sampleUV.x > 1.0f || sampleUV.y < 0.0f || sampleUV.y > 1.0f)
            break;

        float sampleDepth = DepthTex.SampleLevel(PointClampSampler, sampleUV, 0).r;
        if (sampleDepth <= 0.0f)
            continue;

        float3 sampleViewPos = GetViewPosition(sampleUV, sampleDepth);
        float depthDiff = rayPos.z - sampleViewPos.z;
        float dynamicThickness = max(0.2f, abs(rayPos.z) * 0.02f);

        if (depthDiff > 0.0f && depthDiff < dynamicThickness)
        {
            rayPos -= reflectDir * stepSize;
            stepSize *= 0.5f;

            for (int j = 0; j < 5; ++j)
            {
                rayPos += reflectDir * stepSize;

                clipPos = mul(float4(rayPos, 1.0f), PROJ);
                sampleUV = (clipPos.xy / clipPos.w) * 0.5f + 0.5f;
                sampleUV.y = 1.0f - sampleUV.y;

                sampleDepth = DepthTex.SampleLevel(PointClampSampler, sampleUV, 0).r;
                if (sampleDepth <= 0.0f)
                {
                    stepSize *= 0.5f;
                    continue;
                }

                sampleViewPos = GetViewPosition(sampleUV, sampleDepth);

                if (rayPos.z - sampleViewPos.z > 0.0f)
                {
                    rayPos -= reflectDir * stepSize;
                }
                stepSize *= 0.5f;
            } // for (int j = 0; j < 5; ++j)

            hit = true;
            hitUV = sampleUV;
            break;
        }
    } //  for (int i = 0; i < maxSteps; ++i)
    
    //if (hit)
    //    return float4(1, 0, 0, 1);

    //return float4(0, 0, 0, 1);

    if (hit)
    {
        float3 hitColor = LitSceneTex.SampleLevel(LinearClampSampler, hitUV, 0).rgb;
        float2 fade = smoothstep(0.0f, 0.1f, hitUV) * (1.0f - smoothstep(0.9f, 1.0f, hitUV));
        float screenEdgeFade = fade.x * fade.y;
        float hitDistance = length(rayPos - viewPos);
        float distanceFade = saturate(1.0f - hitDistance / maxTraceDistance);
        float reflectionStrength = saturate((1.0f - roughness) * screenEdgeFade * distanceFade);
        return float4(lerp(sceneColor, hitColor, reflectionStrength), 1.0f);
    }

    return float4(sceneColor, 1.0f);
} // main