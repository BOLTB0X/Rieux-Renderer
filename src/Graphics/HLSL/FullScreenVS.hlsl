// FullScreenVS.hlsl
struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
}; // VS_OUT

VS_OUT main(uint vertexID : SV_VertexID)
{
    VS_OUT output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.pos = float4(output.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
} // main