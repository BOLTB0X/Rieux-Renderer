// HelloTrianglePS.hlsl
struct PS_IN
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
}; // PS_IN

float4 main(PS_IN input) : SV_TARGET
{
    return input.color;
} // main