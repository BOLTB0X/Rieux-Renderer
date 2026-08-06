// FrustumCullingCS.hlsl
#include "Culling.hlsli"

StructuredBuffer<MeshInstanceData>  g_MasterInstanceData : register(t0);
StructuredBuffer<IndirectCommand>   g_MasterCommands : register(t1);
RWStructuredBuffer<IndirectCommand> g_VisibleMainCommands : register(u0);
RWStructuredBuffer<IndirectCommand> g_VisibleVaseCommands : register(u1);
RWByteAddressBuffer                 g_MainCount : register(u2);
RWByteAddressBuffer                 g_VaseCount : register(u3);

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint meshIndex = dispatchThreadID.x;
    if (meshIndex >= TOTAL_INSTANCES)
        return;
    
    MeshInstanceData instance = g_MasterInstanceData[meshIndex];
    
    bool visible = Check_Box_Visible(instance.aabbMin, instance.aabbMax, instance.worldMatrix, FRUSTUM_PLANES);
    if (!visible)
        return;

    if (instance.isVase > 0.5f)
    {
        uint idx;
        g_VaseCount.InterlockedAdd(0, 1, idx);
        g_VisibleVaseCommands[idx] = g_MasterCommands[meshIndex];
    }
    else
    {
        uint idx;
        g_MainCount.InterlockedAdd(0, 1, idx);
        g_VisibleMainCommands[idx] = g_MasterCommands[meshIndex];
    }
} // main