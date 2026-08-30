// GPU_DeferredLightingPS.hlsl
#include "Commons.hlsli"
#include "PBR.hlsli"
#include "ShadowMap.hlsli"

Texture2D<float4>   g_GBuffer0 : register(t0); // Albedo.rgb, Metallic.a
Texture2D<float4>   g_GBuffer1 : register(t1); // NormalWS(*0.5+0.5).rgb, Roughness.a
Texture2D<float>    g_Depth : register(t4); // Reverse-Z 깊이
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

float4 main(PS_IN input) : SV_TARGET
{
    int2 pixelCoord = int2(input.positionCS.xy);
    float4 gb0 = g_GBuffer0.Load(int3(pixelCoord, 0));
    float4 gb1 = g_GBuffer1.Load(int3(pixelCoord, 0));
    float depth = g_Depth.Load(int3(pixelCoord, 0));

    if (depth <= 0.0f)
        discard; // Reverse-Z: 0이면 far plane(하늘/배경) - 라이팅 스킵

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

    // --- IBL Diffuse ---
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float3 kS = Fresnel_Schlick(saturate(dot(N, V)), F0);
    float3 kD = (1.0f - kS) * (1.0f - metallic);
    float3 irradiance = g_IrradianceMap.Sample(LinearSampler, N).rgb;
    float3 diffuseIBL = kD * albedo * irradiance;

    // --- IBL Specular (Split-Sum) ---
    float3 R = reflect(-V, N);
    const float MAX_PREFILTER_MIP = 4.0f; // Prefilter mipLevels=5
    float3 prefilteredColor = g_PrefilteredMap.SampleLevel(LinearSampler, R, roughness * MAX_PREFILTER_MIP).rgb;
    float2 brdf = g_BRDFLUT.Sample(LinearSampler, float2(saturate(dot(N, V)), roughness));
    float3 specularIBL = prefilteredColor * (F0 * brdf.x + brdf.y);

    float3 ambientIBL = diffuseIBL + specularIBL;
    float3 finalColor = directTerm + ambientIBL;

    return float4(finalColor, 1.0f);
} // main