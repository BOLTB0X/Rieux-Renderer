// DepthVS.hlsl
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
    VS_OUT output;
    
    MeshInstanceData inst = g_InstanceData[g_InstanceCB.InstanceIndex];
    PBRVertex v = g_VertexBuffers[inst.vertexBufferIndex][vertexID];

    float4 worldPos = mul(float4(v.position, 1.0f), inst.worldMatrix);
    float4 viewPos = mul(worldPos, VIEW);
    float4 clipPos = mul(viewPos, PROJ);
    
    output.position = clipPos;
    output.texcoord = v.texcoord;
    output.alphaIdx = inst.alphaIndex;

    return output;
} // main