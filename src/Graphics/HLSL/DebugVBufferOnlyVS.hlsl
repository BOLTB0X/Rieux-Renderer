// VertexBufferOnlyDebugVS.hlsl
#include "Commons.hlsli"

struct PBRVertex
{
    float3 position;
    float2 texcoord;
    float3 normal;
    float3 tangent;
    float3 binormal;
}; //

struct InstanceData
{
    matrix world;
    uint vertexBufferIndex;
    uint albedoIndex;
    uint normalIndex;
    uint alphaIndex;
}; // InstanceData

StructuredBuffer<InstanceData> g_Instances : register(t0, space0);
StructuredBuffer<PBRVertex> g_VertexBuffers[] : register(t0, space2);

struct VS_OUT
{
    float4 positionCS : SV_POSITION;
    float3 positionWS : TEXCOORD0;
    float3 normalWS : TEXCOORD1;
    float3 tangentWS : TEXCOORD2;
    float3 binormalWS : TEXCOORD3;
    float2 texcoord : TEXCOORD4;
    nointerpolation uint albedoIndex : TEXCOORD5;
    nointerpolation uint normalIndex : TEXCOORD6;
    nointerpolation uint alphaIndex : TEXCOORD7;
}; // VS_OUT

VS_OUT main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    InstanceData inst = g_Instances[instanceID];
    PBRVertex input = g_VertexBuffers[NonUniformResourceIndex(inst.vertexBufferIndex)][vertexID];

    VS_OUT output = (VS_OUT) 0;
    float4 worldPos = mul(float4(input.position, 1.0f), inst.world);
    float4 viewPos = mul(worldPos, VIEW);
    output.positionCS = mul(viewPos, PROJ);
    output.albedoIndex = 0;
    output.normalIndex = 0;
    output.alphaIndex = 0;
    return output;
} // main