// GPUDriven.hlsli
#ifndef _GPU_DRIVEN_HLSLI_
#define _GPU_DRIVEN_HLSLI_

struct IndexBufferView
{
    uint2 address;
    uint  size;
    uint  format; // DXGI_FORMAT (R16_UINT = 57, R32_UINT = 42 등)
}; // IndexBufferView

struct DrawIndexedArguments
{
    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int  BaseVertexLocation;
    uint StartInstanceLocation;
}; // DrawIndexedArguments

struct IndirectCommand
{
    IndexBufferView      indexBufferView;
    uint                 instanceIndex;
    DrawIndexedArguments drawArgs;
}; // IndirectCommand

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
    float3  mPadding3;
}; // MeshInstanceData

struct InstanceCB
{
    uint InstanceIndex;
}; // InstanceCB

#endif