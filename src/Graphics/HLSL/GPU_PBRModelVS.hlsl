// GPU_PBRModelVS.hlsl
#include "Commons.hlsli"

struct PBRVertex
{
    float3 position;
    float2 texcoord;
    float3 normal;
    float3 tangent;
    float3 binormal;
}; //

StructuredBuffer<PBRVertex> g_VertexBuffers[] : register(t0, space2);

cbuffer WorldCB : register(b2)
{
    matrix wWorld;
}; // WorldCB

#define WORLD wWorld

cbuffer VertexBufferIndexCB : register(b3)
{
    uint vVertexBufferIndex;
}; // VertexBufferIndexCB

#define VERTEX_BUFFER_INDEX vVertexBufferIndex

struct VS_OUT
{
    float4 positionCS : SV_POSITION;
    float3 positionWS : POSITION_WS;
    float3 normalWS : NORMAL_WS;
    float3 tangentWS : TANGENT_WS;
    float3 binormalWS : BINORMAL_WS;
    float2 texcoord : TEXCOORD;
}; // VS_OUT

VS_OUT main(uint vertexID : SV_VertexID)
{
    PBRVertex input = g_VertexBuffers[VERTEX_BUFFER_INDEX][vertexID];

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