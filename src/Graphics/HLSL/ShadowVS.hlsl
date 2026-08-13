// ShadowVS.hlsl
#include "Commons.hlsli"
#include "GPUDriven.hlsli"

struct PBRVertex
{
    float3 position;
    float2 texcoord;
    float3 normal;
    float3 tangent;
    float3 binormal;
}; // PBRVertex

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    uint   alphaIdx : BLENDINDICES0;
}; // VS_OUT

StructuredBuffer<PBRVertex>        g_VertexBuffers[] : register(t0, space2);
StructuredBuffer<MeshInstanceData> g_InstanceData : register(t1, space0);
ConstantBuffer<InstanceCB>         g_InstanceCB : register(b2);

VS_OUT main(uint vertexID : SV_VertexID)
{
    MeshInstanceData inst = g_InstanceData[g_InstanceCB.InstanceIndex];
    PBRVertex vertex = g_VertexBuffers[inst.vertexBufferIndex][vertexID];

    float4 worldPosition = mul(float4(vertex.position, 1.0f), inst.worldMatrix);
    float4 lightPosition = mul(worldPosition, LIGHT_VIEW);

    VS_OUT output;
    output.position = mul(lightPosition, LIGHT_PROJ);
    output.texcoord = vertex.texcoord;
    output.alphaIdx = inst.alphaIndex;
    return output;
} // main
