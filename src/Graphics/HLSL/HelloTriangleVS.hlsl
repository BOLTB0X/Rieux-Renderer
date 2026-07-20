// HelloTriangleVS.hlsl
struct PS_IN
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
}; // PS_IN

PS_IN main(float4 position : POSITION, float4 color : COLOR)
{
    PS_IN result;
    result.position = position;
    result.color = color;
    return result;
} // main