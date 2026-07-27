// FlatSponzaPS.hlsl
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

static const float3 FLAT_ALBEDO = float3(0.7f, 0.7f, 0.7f);

float4 main(PS_IN input) : SV_TARGET
{
    float3 N = normalize(input.normalWS);
    float3 L = normalize(-LIGHT_DIRECTION);

    float NdotL = saturate(dot(N, L));
    float3 diffuseTerm = LIGHT_DIFFUSE.rgb * NdotL;
    float3 ambientTerm = LIGHT_AMBIENT.rgb;

    float3 color = FLAT_ALBEDO * (ambientTerm + diffuseTerm);
    return float4(color, 1.0f);
} // main