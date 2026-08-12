// OcclusionCullingCS.hlsl
#include "Culling.hlsli"
#include "GPUDriven.hlsli"

Texture2D<float>                    g_HiZTexture : register(t0);
StructuredBuffer<IndirectCommand>   g_FrustumMainCommands : register(t1);
StructuredBuffer<IndirectCommand>   g_FrustumVaseCommands : register(t2);
ByteAddressBuffer                   g_FrustumMainCount : register(t3);
ByteAddressBuffer                   g_FrustumVaseCount : register(t4);
StructuredBuffer<MeshInstanceData>  g_InstanceData : register(t5);

ConstantBuffer<OcclusionCB>         g_OcclusionCB : register(b0);
ConstantBuffer<PhaseInfo>           g_PhaseCB : register(b1);

RWStructuredBuffer<IndirectCommand> g_FinalMainCommands : register(u0);
RWStructuredBuffer<IndirectCommand> g_FinalVaseCommands : register(u1);
RWByteAddressBuffer                 g_FinalMainCount : register(u2);
RWByteAddressBuffer                 g_FinalVaseCount : register(u3);
RWStructuredBuffer<IndirectCommand> g_CulledMainCommands : register(u4);
RWStructuredBuffer<IndirectCommand> g_CulledVaseCommands : register(u5);
RWByteAddressBuffer                 g_CulledMainCount : register(u6);
RWByteAddressBuffer                 g_CulledVaseCount : register(u7);
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
    } // if -  else

    MeshInstanceData instance = g_InstanceData[cmd.instanceIndex];

    matrix MVP = mul(instance.worldMatrix, g_OcclusionCB.viewProjection);
    ProjectedAABB projAABB = Get_ProjectedAABB(instance.aabbMin, instance.aabbMax, MVP);

    // 컬링 테스트 결과 판정
    bool isVisible = projAABB.isValid && !Is_Occluded(projAABB, g_OcclusionCB.screenSize, g_HiZTexture, g_PointClampSampler);

    if (isVisible)
    {
        // 테스트 통과 Phase 1, Phase 2
        // 무조건 Final 버퍼에 Append 기록
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
    } // if (isVisible)
    else if (g_PhaseCB.isPhase2 == 0)
    {
        // 테스트 실패 & 현재 Phase 1일 때만: 다음 Phase 2를 위해 Culled 버퍼에 기록
        // 만약 Phase 2에서도 실패했다면 더 이상 볼일 없으므로 그냥 Drop
        uint outputIndex;
        if (isMain)
        {
            g_CulledMainCount.InterlockedAdd(0, 1, outputIndex);
            g_CulledMainCommands[outputIndex] = cmd;
        }
        else
        {
            g_CulledVaseCount.InterlockedAdd(0, 1, outputIndex);
            g_CulledVaseCommands[outputIndex] = cmd;
        }
    } // else if (g_PhaseCB.isPhase2 == 0)
} // main