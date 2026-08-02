// DebugLineVS.hlsl
#include "Commons.hlsli"

struct VS_IN
{
    float3 position : POSITION;
    float3 color : COLOR;
}; // VS_IN

struct VS_OUT
{
    float4 positionCS : SV_POSITION;
    float3 color : COLOR;
}; // VS_OUT

VS_OUT main(VS_IN input)
{
    VS_OUT output;
    float4 viewPos = mul(float4(input.position, 1.0f), VIEW);
    output.positionCS = mul(viewPos, PROJ);
    output.color = input.color;
    return output;
} // main