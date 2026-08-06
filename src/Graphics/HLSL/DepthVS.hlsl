// DepthVS.hlsl
#include "Commons.hlsli"

struct PBRVertex
{
    float3 position;
    float2 texcoord;
    float3 normal;
    float3 tangent;
    float3 binormal;
}; // PBRVertex

struct MeshInstanceData
{
    matrix worldMatrix;
    uint   vertexBufferIndex;
    uint   albedoIndex;
    uint   normalIndex;
    uint   alphaIndex;

    float3 aabbMin;
    float  mPadding1;
    float3 aabbMax;
    float  mPadding2;

    float  isVase;
    float3 mPadding3;
}; // MeshInstanceData

struct InstanceCB
{
    uint InstanceIndex;
}; // InstanceCB

StructuredBuffer<PBRVertex>        g_VertexBuffers[] : register(t0, space2);
StructuredBuffer<MeshInstanceData> g_InstanceData : register(t1, space0);
ConstantBuffer<InstanceCB>         g_InstanceCB : register(b2);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    uint   alphaIdx : BLENDINDICES0;
}; // VS_OUT

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