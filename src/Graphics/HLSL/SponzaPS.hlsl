// SponzaPS.hlsl
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

Texture2D AlbedoTex : register(t0);
Texture2D NormalTex : register(t1);
Texture2D AlphaTex : register(t2);
SamplerState Sampler : register(s0);

static const float3 FLAT_ALBEDO = float3(0.7f, 0.7f, 0.7f); // 텍스처 배선 전까지 임시 회색


float4 main(PS_IN input) : SV_TARGET
{
    float3 N = normalize(input.normalWS);
    float3 L = normalize(-LIGHT_DIRECTION);

    float NdotL = saturate(dot(N, L));
    float3 diffuseTerm = LIGHT_DIFFUSE.rgb * NdotL;
    float3 ambientTerm = LIGHT_AMBIENT.rgb;

    float3 color = FLAT_ALBEDO * (ambientTerm + diffuseTerm);
    return float4(color, 1.0f);
    //// Alpha 마스크 컷오프
    //float4 alphaSample = AlphaTex.Sample(Sampler, input.texcoord);
    //if (alphaSample.r < 0.1f)
    //{
    //    discard;
    //}

    //// Albedo 텍스처 샘플링
    //float4 albedoColor = AlbedoTex.Sample(Sampler, input.texcoord);

    //// Normal 맵 샘플링 및 TBN 공간 변환
    //float3 normalSample = NormalTex.Sample(Sampler, input.texcoord).rgb;
    //float3 localNormal = normalSample * 2.0f - 1.0f; // [0, 1] -> [-1, 1]

    //float3 N = normalize(input.normalWS);
    //float3 T = normalize(input.tangentWS);
    //float3 B = normalize(input.binormalWS);

    //// 그람-슈미트 직교화 (Gram-Schmidt Orthogonalization)
    //T = normalize(T - dot(T, N) * N);
    //B = cross(N, T);

    //float3 normalWS = normalize(T * localNormal.x + B * localNormal.y + N * localNormal.z);

    //float3 L = normalize(-LIGHT_DIRECTION);

    //float NdotL = saturate(dot(normalWS, L));
    //float3 diffuseTerm = LIGHT_DIFFUSE.rgb * NdotL;
    //float3 ambientTerm = LIGHT_AMBIENT.rgb;

    //float3 finalColor = albedoColor.rgb * (ambientTerm + diffuseTerm);

    //return float4(finalColor, albedoColor.a);
} // main