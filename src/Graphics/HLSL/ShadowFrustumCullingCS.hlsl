// ShadowFrustumCullingCS.hlsl
#include "Culling.hlsli"
#include "GPUDriven.hlsli"

#define NUM_CASCADES 4

struct ShadowCullingData
{
    uint   mainInstances;
    uint   vaseInstances;
    float2 padding;
    float4 cascadePlanes[NUM_CASCADES][6]; // [4개 캐스케이드][6개 평면]
}; // ShadowCullingData

// Input SRVs & CBV
StructuredBuffer<MeshInstanceData>  g_MasterInstanceData : register(t0);
StructuredBuffer<IndirectCommand>   g_MasterCommands : register(t1);
ConstantBuffer<ShadowCullingData>   g_ShadowCullingCB : register(b0);

// Main Output UAVs
RWStructuredBuffer<IndirectCommand> g_MainVisibleCascade0 : register(u0);
RWStructuredBuffer<IndirectCommand> g_MainVisibleCascade1 : register(u1);
RWStructuredBuffer<IndirectCommand> g_MainVisibleCascade2 : register(u2);
RWStructuredBuffer<IndirectCommand> g_MainVisibleCascade3 : register(u3);

// Vase Output UAVs
RWStructuredBuffer<IndirectCommand> g_VaseVisibleCascade0 : register(u4);
RWStructuredBuffer<IndirectCommand> g_VaseVisibleCascade1 : register(u5);
RWStructuredBuffer<IndirectCommand> g_VaseVisibleCascade2 : register(u6);
RWStructuredBuffer<IndirectCommand> g_VaseVisibleCascade3 : register(u7);

// Counters UAVs
RWByteAddressBuffer                 g_MainDrawCounts : register(u8);
RWByteAddressBuffer                 g_VaseDrawCounts : register(u9);

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint globalIndex = dispatchThreadID.x;
    uint totalInstances = g_ShadowCullingCB.mainInstances + g_ShadowCullingCB.vaseInstances;

    if (globalIndex >= totalInstances)
    {
        return;
    }

    bool isVase = (globalIndex >= g_ShadowCullingCB.mainInstances);

    MeshInstanceData instance = g_MasterInstanceData[globalIndex];
    IndirectCommand cmd = g_MasterCommands[globalIndex];
    
    cmd.drawArgs.IndexCountPerInstance = instance.shadowIndexCount;
    cmd.drawArgs.StartIndexLocation = instance.shadowStartIndex;

    // Cascade 0~3 순회 검사
    [unroll]
    for (uint c = 0; c < NUM_CASCADES; ++c)
    {
        if (Check_Box_Visible(instance.aabbMin, instance.aabbMax, instance.worldMatrix, g_ShadowCullingCB.cascadePlanes[c]))
        {
            uint writeIndex;
            uint byteOffset = c * 4; // Cascade 인덱스에 따른 Counter Offset (0, 4, 8, 12)

            if (!isVase)
            {
                g_MainDrawCounts.InterlockedAdd(byteOffset, 1, writeIndex);
                if (c == 0)
                    g_MainVisibleCascade0[writeIndex] = cmd;
                else if (c == 1)
                    g_MainVisibleCascade1[writeIndex] = cmd;
                else if (c == 2)
                    g_MainVisibleCascade2[writeIndex] = cmd;
                else if (c == 3)
                    g_MainVisibleCascade3[writeIndex] = cmd;
            }
            else
            {
                g_VaseDrawCounts.InterlockedAdd(byteOffset, 1, writeIndex);
                if (c == 0)
                    g_VaseVisibleCascade0[writeIndex] = cmd;
                else if (c == 1)
                    g_VaseVisibleCascade1[writeIndex] = cmd;
                else if (c == 2)
                    g_VaseVisibleCascade2[writeIndex] = cmd;
                else if (c == 3)
                    g_VaseVisibleCascade3[writeIndex] = cmd;
            }
        }
    } // for (uint c = 0; c < NUM_CASCADES; ++c)
} // main