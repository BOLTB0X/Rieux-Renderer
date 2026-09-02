// GPU_DeferredLightingPS.hlsl
#include "Commons.hlsli"
#include "PBR.hlsli"
#include "ShadowMap.hlsli"

Texture2D<float4>   g_GBuffer0 : register(t0);
Texture2D<float4>   g_GBuffer1 : register(t1);
Texture2D<float>    g_Depth : register(t4);
TextureCube<float4> g_IrradianceMap : register(t5);
TextureCube<float4> g_PrefilteredMap : register(t6);
Texture2D<float2>   g_BRDFLUT : register(t7);
SamplerState        LinearSampler : register(s0);

struct PS_IN
{
    float4 positionCS : SV_POSITION;
    float2 texcoord : TEXCOORD0;
}; // PS_IN

float3 ReconstructWorldPos(float2 uv, float depth)
{
    float4 clipPos = float4(uv.x * 2.0f - 1.0f, (1.0f - uv.y) * 2.0f - 1.0f, depth, 1.0f);
    float4 worldPos = mul(clipPos, PROJ_INV);
    worldPos = mul(worldPos, VIEW_INV);
    return worldPos.xyz / worldPos.w;
} // ReconstructWorldPos

float3 ParallaxCorrection(float3 dir, float3 positionWS, float3 probePosition, float3 boxMin, float3 boxMax)
{
    float3 safeDir = dir;
    safeDir.x = abs(safeDir.x) < 0.00001f ? (safeDir.x < 0.0f ? -0.00001f : 0.00001f) : safeDir.x;
    safeDir.y = abs(safeDir.y) < 0.00001f ? (safeDir.y < 0.0f ? -0.00001f : 0.00001f) : safeDir.y;
    safeDir.z = abs(safeDir.z) < 0.00001f ? (safeDir.z < 0.0f ? -0.00001f : 0.00001f) : safeDir.z;

    float3 firstPlaneIntersect = (boxMax - positionWS) / safeDir;
    float3 secondPlaneIntersect = (boxMin - positionWS) / safeDir;

    float3 furthestPlane = max(firstPlaneIntersect, secondPlaneIntersect);
    float distance = min(min(furthestPlane.x, furthestPlane.y), furthestPlane.z);

    float3 intersectPosition = positionWS + safeDir * distance;
    return intersectPosition - probePosition;
} // ParallaxCorrection

float CalculateProbeWeight(float3 positionWS, float3 influenceMin, float3 influenceMax, float blendDistance)
{
    float3 fadeMin = smoothstep(influenceMin, influenceMin + blendDistance, positionWS);
    float3 fadeMax = 1.0f - smoothstep(influenceMax - blendDistance, influenceMax, positionWS);
    
    float3 fade = fadeMin * fadeMax;
    return fade.x * fade.y * fade.z;
} // CalculateProbeWeight

float4 main(PS_IN input) : SV_TARGET
{
    int2 pixelCoord = int2(input.positionCS.xy);
    float4 gb0 = g_GBuffer0.Load(int3(pixelCoord, 0));
    float4 gb1 = g_GBuffer1.Load(int3(pixelCoord, 0));
    float depth = g_Depth.Load(int3(pixelCoord, 0));

    if (depth <= 0.0f)
        discard;

    float3 albedo = gb0.rgb;
    float metallic = gb0.a;
    float3 normalWS = normalize(gb1.rgb * 2.0f - 1.0f);
    float roughness = gb1.a;

    float3 positionWS = ReconstructWorldPos(input.texcoord, depth);
    float3 V = normalize(CAMERA_POSITION - positionWS);
    float3 N = normalWS;
    float3 L = normalize(-LIGHT_DIRECTION);

    // --- Direct Lighting  ---
    float viewSpaceDepth = mul(float4(positionWS, 1.0f), VIEW).z;
    float shadowFactor = Calculate_CascadeShadow(positionWS, N, viewSpaceDepth);
    float3 directTerm = Evaluate_Direct_PBR(
        albedo, metallic, roughness, N, V, L, LIGHT_DIFFUSE.rgb, shadowFactor);
    
    float3 probePos = PROBE_POSITION;
    
    // 시차 보정용 (Projection Box)
    float3 projBoxMin = PROBE_BOX_MIN;
    float3 projBoxMax = PROBE_BOX_MAX;
    
    // 블렌딩용 (Influence Box) - 새로 추가된 매크로
    float3 influenceBoxMin = PROBE_INFLUENCE_MIN;
    float3 influenceBoxMax = PROBE_INFLUENCE_MAX;
    
    float probeWeight = CalculateProbeWeight(positionWS, influenceBoxMin, influenceBoxMax, PROBE_BLEND_DISTANCE);

    // --- IBL Diffuse ---
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float3 kS = Fresnel_Schlick(saturate(dot(N, V)), F0);
    float3 kD = (1.0f - kS) * (1.0f - metallic);
    
    float3 correctedN = ParallaxCorrection(N, positionWS, probePos, projBoxMin, projBoxMax);
    float3 irradiance = g_IrradianceMap.Sample(LinearSampler, correctedN).rgb;
    float3 diffuseIBL = kD * albedo * irradiance;

    // --- IBL Specular ---
    float3 R = reflect(-V, N);
    float3 correctedR = ParallaxCorrection(R, positionWS, probePos, projBoxMin, projBoxMax);
    
    const float MAX_PREFILTER_MIP = 4.0f;
    const float SSR_ROUGHNESS_CUTOFF = 0.35f;
    
    float3 prefilteredColor = g_PrefilteredMap.SampleLevel(LinearSampler, correctedR, roughness * MAX_PREFILTER_MIP).rgb;
    float2 brdf = g_BRDFLUT.Sample(LinearSampler, float2(saturate(dot(N, V)), roughness));
    float3 specularIBL = prefilteredColor * (F0 * brdf.x + brdf.y);
    
    if (roughness < SSR_ROUGHNESS_CUTOFF)
    {
        float ssrFadeWeight = roughness / SSR_ROUGHNESS_CUTOFF;
        specularIBL *= lerp(0.15f, 1.0f, ssrFadeWeight);
    }

    // 최종 환경광 계산 및 적용
    float3 ambientIBL = (diffuseIBL + specularIBL) * shadowFactor;
    float3 globalAmbient = albedo * LIGHT_AMBIENT.rgb * shadowFactor;
    
    float3 finalAmbient = lerp(globalAmbient, ambientIBL, probeWeight);
    float3 finalColor = directTerm + finalAmbient;
    
    //return float4(probeWeight.xxx, 1.0f);
    return float4(finalColor, 1.0f);
} // main