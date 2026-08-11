// VertexBufferIndexDebugVS.hlsl
#include "Commons.hlsli"

struct PBRVertex
{
    float3 position;
    float2 texcoord;
    float3 normal;
    float3 tangent;
    float3 binormal;
};

struct InstanceData
{
    matrix world;
    uint vertexBufferIndex;
    uint albedoIndex;
    uint normalIndex;
    uint alphaIndex;
};

StructuredBuffer<InstanceData> g_Instances : register(t0, space0);
StructuredBuffer<PBRVertex> g_VertexBuffers[] : register(t0, space2);

struct VS_OUT
{
    float4 positionCS : SV_POSITION;
    float3 dbgColor : TEXCOORD8;
};

VS_OUT main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    InstanceData inst = g_Instances[instanceID];
    uint vbIdx = inst.vertexBufferIndex;
    
    PBRVertex realInput = g_VertexBuffers[NonUniformResourceIndex(vbIdx)][vertexID];

    // 테스트용 강제 화면 출력 삼각형
    float2 positions[3] = { float2(-0.5f, -0.5f), float2(0.5f, -0.5f), float2(0.0f, 0.5f) };
    
    VS_OUT output = (VS_OUT) 0;
    float4 localPos = float4(positions[vertexID % 3], 0.0f, 1.0f);
    
    float4 worldPos = mul(localPos, inst.world);
    float4 viewPos = mul(worldPos, VIEW);
    output.positionCS = mul(viewPos, PROJ);


    output.dbgColor = abs(normalize(realInput.position));
    output.dbgColor = realInput.normal * 0.5f + 0.5f; 
    
    return output;
}