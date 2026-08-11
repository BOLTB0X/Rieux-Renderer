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
    {
        return;
    }

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

    // AABB 투영 계산
    matrix MVP = mul(instance.worldMatrix, g_OcclusionCB.viewProjection);
    ProjectedAABB projAABB = Get_ProjectedAABB(instance.aabbMin, instance.aabbMax, MVP);

    // 화면 밖이거나, Hi-Z 테스트에 의해 가려졌다면 컬링
    if (!projAABB.isValid || Is_Occluded(projAABB, g_OcclusionCB.screenSize, g_HiZTexture, g_PointClampSampler))
    {
        return;
    }

    // 테스트 통과 시 최종 버퍼에 기록
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