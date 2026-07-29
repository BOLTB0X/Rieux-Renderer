// GPU_PBRModelVS.hlsl
#include "Commons.hlsli"

struct VertexData
{
    float3 Position;
    float2 TexCoord;
    float3 Normal;
    float3 Tangent;
    float3 Binormal;
}; // VertexData

struct GPUMeshData
{
    uint vertexOffset;
    uint indexOffset;
    uint indexCount;
    uint materialIndex;
}; // GPUMeshData

StructuredBuffer<GPUMeshData> g_MeshDatas : register(t0, space0);
StructuredBuffer<VertexData> g_Vertices : register(t1, space0);

cbuffer WorldCB : register(b2, space0)
{
    matrix g_world;
}; // WorldCB

#define WORLD g_world

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    uint   instID : INSTANCE_ID;
}; // VS_OUT

// 수동 Fetch Vertex Shader
VS_OUT main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VS_OUT output;

    GPUMeshData meshInfo = g_MeshDatas[instanceID];

    uint globalVertexIdx = meshInfo.vertexOffset + vertexID;
    VertexData v = g_Vertices[globalVertexIdx];

    float4 worldPos = mul(float4(v.Position, 1.0f), WORLD);
    float4 viewPos = mul(worldPos, VIEW);
    output.pos = mul(viewPos, PROJ);
    
    output.uv = v.TexCoord;
    output.normal = mul(v.Normal, (float3x3) WORLD);
    output.instID = instanceID;

    return output;
} // main