// DebugAABBVS.hlsl
#include "Commons.hlsli"
#include "GPUDriven.hlsli"

struct Debug_InstanceCB
{
    uint InstanceIndex;
    uint PassType;
}; // Debug_InstanceCB

StructuredBuffer<MeshInstanceData> g_InstanceData : register(t1, space0);
ConstantBuffer<Debug_InstanceCB>   g_InstanceCB : register(b2);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
}; // VS_OUT

static const uint g_BoxIndices[24] =
{
    0, 1, 1, 3, 3, 2, 2, 0, // Bottom
    4, 5, 5, 7, 7, 6, 6, 4, // Top
    0, 4, 1, 5, 2, 6, 3, 7  // Pillars
}; // g_BoxIndices

VS_OUT main(uint vertexID : SV_VertexID)
{
    VS_OUT output;
    
    MeshInstanceData inst = g_InstanceData[g_InstanceCB.InstanceIndex];
    uint cornerIdx = g_BoxIndices[vertexID % 24];
    
    float3 localPos;
    localPos.x = (cornerIdx & 1) ? inst.aabbMax.x : inst.aabbMin.x;
    localPos.y = (cornerIdx & 2) ? inst.aabbMax.y : inst.aabbMin.y;
    localPos.z = (cornerIdx & 4) ? inst.aabbMax.z : inst.aabbMin.z;

    float3 center = (inst.aabbMin + inst.aabbMax) * 0.5f;
    localPos = center + (localPos - center) * 1.001f;

    float4 worldPos = mul(float4(localPos, 1.0f), inst.worldMatrix);
    float4 viewPos = mul(worldPos, VIEW);
    output.position = mul(viewPos, PROJ);
  
    // PassType에 따라 색상 결정
    if (g_InstanceCB.PassType == 0)
    {
        output.color = float3(1.0f, 0.0f, 0.0f); // 통과 못 한 메쉬 -> 빨간색
    }
    else
    {
        output.color = (inst.isVase > 0.5f) ? float3(1.0f, 0.9f, 0.2f) : float3(0.2f, 1.0f, 0.4f);
    }

    return output;
} // main