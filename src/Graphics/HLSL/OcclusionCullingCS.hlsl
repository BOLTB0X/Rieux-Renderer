// OcclusionCullingCS.hlsl
#include "Culling.hlsli"
#include "GPUDriven.hlsli"

Texture2D<float>                   g_HiZTexture : register(t0);
StructuredBuffer<IndirectCommand>  g_FrustumMainCommands : register(t1);
StructuredBuffer<IndirectCommand>  g_FrustumVaseCommands : register(t2);
ByteAddressBuffer                  g_FrustumMainCount : register(t3);
ByteAddressBuffer                  g_FrustumVaseCount : register(t4);
StructuredBuffer<MeshInstanceData> g_InstanceData : register(t5);

ConstantBuffer<OcclusionCB>         g_OcclusionCB : register(b0);
RWStructuredBuffer<IndirectCommand> g_FinalMainCommands : register(u0);
RWStructuredBuffer<IndirectCommand> g_FinalVaseCommands : register(u1);
RWByteAddressBuffer                 g_FinalMainCount : register(u2);
RWByteAddressBuffer                 g_FinalVaseCount : register(u3);
SamplerState                        g_PointClampSampler : register(s0);

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint mainCount = g_FrustumMainCount.Load(0);
    uint vaseCount = g_FrustumVaseCount.Load(0);
    uint totalCount = mainCount + vaseCount;

    if (DTid.x >= totalCount)
        return;

    bool isMain = (DTid.x < mainCount);
    uint readIndex = isMain ? DTid.x : (DTid.x - mainCount);

    IndirectCommand cmd;
    if (isMain)
    {
        cmd = g_FrustumMainCommands[readIndex];
    }
    else
    {
        cmd = g_FrustumVaseCommands[readIndex];
    }

    MeshInstanceData instance = g_InstanceData[cmd.instanceIndex];

    // --------------------------------------------------------------------------
    // AABB 투영 및 NDC 바운딩 박스 계산
    // --------------------------------------------------------------------------
    float minX = 1.0f, minY = 1.0f;
    float maxX = -1.0f, maxY = -1.0f;
    float closestZ = -1.0f; // Reverse-Z 환경에서는 값이 클수록 카메라와 가까움

    matrix MVP = mul(instance.worldMatrix, g_OcclusionCB.viewProjection);

    [unroll]
    for (uint i = 0; i < 8; ++i)
    {
        float3 corner = Get_BoxCorner(instance.aabbMin, instance.aabbMax, i);
        float4 clipPos = mul(float4(corner, 1.0f), MVP);
        
        if (clipPos.w <= 0.0f)
            clipPos.w = 0.0001f;
        
        float3 ndc = clipPos.xyz / clipPos.w;

        minX = min(minX, ndc.x);
        minY = min(minY, ndc.y);
        maxX = max(maxX, ndc.x);
        maxY = max(maxY, ndc.y);
        
        closestZ = max(closestZ, ndc.z); // Reverse-Z
    } // for (uint i = 0; i < 8; ++i)

    if (minX > 1.0f || maxX < -1.0f || minY > 1.0f || maxY < -1.0f)
    {
        return;
    }

    float2 uvMin = saturate(float2(minX * 0.5f + 0.5f, -maxY * 0.5f + 0.5f));
    float2 uvMax = saturate(float2(maxX * 0.5f + 0.5f, -minY * 0.5f + 0.5f));

    // --------------------------------------------------------------------------
    // Hi-Z 뎁스 샘플링 및 비교
    // --------------------------------------------------------------------------
    float2 sizeInPixels = (uvMax - uvMin) * g_OcclusionCB.screenSize;
    
    // 박스가 2x2 텍셀 이하에 들어오도록 AABB 크기에 맞는 Mip 레벨 계산
    float mipLevel = ceil(log2(max(max(sizeInPixels.x, sizeInPixels.y), 1.0f)));

    // 계산된 Mip 레벨에서 4개의 코너를 샘플링하여 가장 먼 깊이, 가장 작은 값 을 찾음
    float d0 = g_HiZTexture.SampleLevel(g_PointClampSampler, float2(uvMin.x, uvMin.y), mipLevel).r;
    float d1 = g_HiZTexture.SampleLevel(g_PointClampSampler, float2(uvMax.x, uvMin.y), mipLevel).r;
    float d2 = g_HiZTexture.SampleLevel(g_PointClampSampler, float2(uvMin.x, uvMax.y), mipLevel).r;
    float d3 = g_HiZTexture.SampleLevel(g_PointClampSampler, float2(uvMax.x, uvMax.y), mipLevel).r;

    // Reverse-Z: 4개의 값 중 가장 작은 값이 배경(가림막) 중 가장 뒤에 있는 값
    float maxOccluderDepth = min(min(d0, d1), min(d2, d3));

    // 내 물체의 가장 가까운 면이 가림막 중 가장 먼 깊이보다 뒤에 있다면 컬링
    if (closestZ < maxOccluderDepth)
    {
        return;
    }

    // --------------------------------------------------------------------------
    // 오클루전 테스트 통과 -> 최종 버퍼에 기록
    // --------------------------------------------------------------------------
    uint outputIndex;
    if (isMain)
    {
        g_FinalMainCount.InterlockedAdd(0, 1, outputIndex);
        g_FinalMainCommands[outputIndex] = cmd;
    }
    else
    {
        g_FinalVaseCount.InterlockedAdd(0, 1, outputIndex);
        g_FinalVaseCommands[outputIndex] = cmd;
    }
} // main