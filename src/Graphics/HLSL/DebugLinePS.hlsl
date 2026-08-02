// DebugLinePS.hlsl
struct PS_IN
{
    float4 positionCS : SV_POSITION;
    float3 color : COLOR;
}; // PS_IN

float4 main(PS_IN input) : SV_TARGET
{
    return float4(input.color, 1.0f);
} // main