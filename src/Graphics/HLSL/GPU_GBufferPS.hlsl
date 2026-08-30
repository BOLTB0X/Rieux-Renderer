// GPU_GBufferPS.hlsl
#include "Commons.hlsli"
#include "GPUDriven.hlsli"

struct PS_IN
{
    float4 positionCS : SV_POSITION;
    float3 positionWS : POSITION_WS;
    float3 normalWS : NORMAL_WS;
    float3 tangentWS : TANGENT_WS;
    float3 binormalWS : BINORMAL_WS;
    float2 texcoord : TEXCOORD;
}; // PS_IN

struct PS_OUT
{
    float4 gbuffer0 : SV_TARGET0; // albedo.rgb, metallic
    float4 gbuffer1 : SV_TARGET1; // normalWS.xyz, roughness
}; // PS_OUT

Texture2D g_Textures[] : register(t0, space1);
StructuredBuffer<MeshInstanceData> g_InstanceData : register(t1, space0);
SamplerState Sampler : register(s0);
ConstantBuffer<InstanceCB> g_InstanceCB : register(b2);

PS_OUT main(PS_IN input)
{
    MeshInstanceData inst = g_InstanceData[g_InstanceCB.InstanceIndex];

    float4 alphaSample = g_Textures[NonUniformResourceIndex(inst.alphaIndex)].Sample(Sampler, input.texcoord);
    if (alphaSample.r < 0.1f)
        discard;

    float4 albedoColor = g_Textures[NonUniformResourceIndex(inst.albedoIndex)].Sample(Sampler, input.texcoord);
    albedoColor *= inst.albedoFactor;

    float3 normalSample = g_Textures[NonUniformResourceIndex(inst.normalIndex)].Sample(Sampler, input.texcoord).rgb;
    float3 localNormal = normalSample * 2.0f - 1.0f;
    float3 N = normalize(input.normalWS);
    float3 T = normalize(input.tangentWS);
    float3 B = normalize(input.binormalWS);
    T = normalize(T - dot(T, N) * N);
    B = cross(N, T);
    float3 normalWS = normalize(T * localNormal.x + B * localNormal.y + N * localNormal.z);

    float metallic = saturate(inst.metallicFactor);
    if (inst.metallicIndex != 0xFFFFFFFF)
        metallic = saturate(g_Textures[NonUniformResourceIndex(inst.metallicIndex)].Sample(Sampler, input.texcoord).r * inst.metallicFactor);

    float roughness = saturate(inst.roughnessFactor);
    if (inst.roughnessIndex != 0xFFFFFFFF)
        roughness = saturate(g_Textures[NonUniformResourceIndex(inst.roughnessIndex)].Sample(Sampler, input.texcoord).r * inst.roughnessFactor);

    PS_OUT output;
    output.gbuffer0 = float4(albedoColor.rgb, metallic);
    output.gbuffer1 = float4(normalWS * 0.5f + 0.5f, roughness);
    return output;
} // main