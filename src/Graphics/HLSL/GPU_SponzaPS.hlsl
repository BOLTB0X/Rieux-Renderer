// GPU_SponzaPS.hlsl
#include "Commons.hlsli"

struct PS_IN
{
    float4 positionCS : SV_POSITION;
    float3 positionWS : POSITION_WS;
    float3 normalWS : NORMAL_WS;
    float3 tangentWS : TANGENT_WS;
    float3 binormalWS : BINORMAL_WS;
    float2 texcoord : TEXCOORD;
}; // PS_IN

Texture2D    g_Textures[] : register(t0, space1);
SamplerState Sampler : register(s0);

cbuffer MaterialIndicesCB : register(b4)
{
    uint AlbedoIndex;
    uint NormalIndex;
    uint AlphaIndex;
    uint _padding;
}; // MaterialIndicesCB

float4 main(PS_IN input) : SV_TARGET
{
    float4 alphaSample = g_Textures[AlphaIndex].Sample(Sampler, input.texcoord);
    if (alphaSample.r < 0.1f)
        discard;

    float4 albedoColor = g_Textures[AlbedoIndex].Sample(Sampler, input.texcoord);
    float3 normalSample = g_Textures[NormalIndex].Sample(Sampler, input.texcoord).rgb;
    float3 localNormal = normalSample * 2.0f - 1.0f;

    float3 N = normalize(input.normalWS);
    float3 T = normalize(input.tangentWS);
    float3 B = normalize(input.binormalWS);

    T = normalize(T - dot(T, N) * N);
    B = cross(N, T);

    float3 normalWS = normalize(T * localNormal.x + B * localNormal.y + N * localNormal.z);

    float3 L = normalize(-LIGHT_DIRECTION);

    float NdotL = saturate(dot(normalWS, L));
    float3 diffuseTerm = LIGHT_DIFFUSE.rgb * NdotL;
    float3 ambientTerm = LIGHT_AMBIENT.rgb;

    float3 finalColor = albedoColor.rgb * (ambientTerm + diffuseTerm);

    return float4(finalColor, albedoColor.a);
} // main