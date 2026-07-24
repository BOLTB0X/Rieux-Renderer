// PBRModelVS.hlsl
#include "Commons.hlsli"

struct VS_IN
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
};

struct VS_OUT
{
    float4 positionCS : SV_POSITION;
    float3 positionWS : POSITION_WS;
    float3 normalWS : NORMAL_WS;
    float3 tangentWS : TANGENT_WS;
    float3 binormalWS : BINORMAL_WS;
    float2 texcoord : TEXCOORD;
};

cbuffer WorldCB : register(b2)
{
    matrix world;
}; // WorldCB

#define WORLD world

VS_OUT main(VS_IN input)
{
    VS_OUT output;

    float4 worldPos = mul(float4(input.position, 1.0f), WORLD);
    output.positionWS = worldPos.xyz;

    float4 viewPos = mul(worldPos, VIEW);
    output.positionCS = mul(viewPos, PROJ);

    output.normalWS = normalize(mul(input.normal, (float3x3) WORLD));
    output.tangentWS = normalize(mul(input.tangent, (float3x3) WORLD));
    output.binormalWS = normalize(mul(input.binormal, (float3x3) WORLD));

    output.texcoord = input.texcoord;

    return output;
} // main
