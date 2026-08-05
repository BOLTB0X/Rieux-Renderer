// VertexBufferIndexDebugPS.hlsl
struct PS_IN
{
    float4 positionCS : SV_POSITION;
    float3 dbgColor : TEXCOORD8;
};

float4 main(PS_IN input) : SV_TARGET
{
    return float4(input.dbgColor, 1.0f);
}