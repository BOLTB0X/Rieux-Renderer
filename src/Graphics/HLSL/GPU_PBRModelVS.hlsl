// GPU_PBRModelVS.hlsl
#include "Commons.hlsli"
#include "GPUDriven.hlsli"
#include "PBR.hlsli"

struct VS_OUT
{
    float4 positionCS : SV_POSITION;
    float3 positionWS : POSITION_WS;
    float3 normalWS : NORMAL_WS;
    float3 tangentWS : TANGENT_WS;
    float3 binormalWS : BINORMAL_WS;
    float2 texcoord : TEXCOORD;
}; // VS_OUT

StructuredBuffer<PBRVertex>        g_VertexBuffers[] : register(t0, space2);
StructuredBuffer<MeshInstanceData> g_InstanceData : register(t1, space0);
ConstantBuffer<InstanceCB>         g_InstanceCB : register(b2);

VS_OUT main(uint vertexID : SV_VertexID)
{
    MeshInstanceData inst = g_InstanceData[g_InstanceCB.InstanceIndex];
    PBRVertex input = g_VertexBuffers[inst.vertexBufferIndex][vertexID];
    
    VS_OUT output;
    float4 worldPos = mul(float4(input.position, 1.0f), inst.worldMatrix);
    output.positionWS = worldPos.xyz;

    float4 viewPos = mul(worldPos, VIEW);
    output.positionCS = mul(viewPos, PROJ);

    output.normalWS = normalize(mul(input.normal, (float3x3) inst.worldMatrix));
    output.tangentWS = normalize(mul(input.tangent, (float3x3) inst.worldMatrix));
    output.binormalWS = normalize(mul(input.binormal, (float3x3) inst.worldMatrix));
    output.texcoord = input.texcoord;

    return output;
} // main