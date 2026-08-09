// LinearZTransPS.hlsl
struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
}; // PS_IN

struct CameraClip
{
    float near;
    float far;
}; // CameraClip

Texture2D<float>           g_Tex : register(t0);
SamplerState               g_Sampler : register(s0);
ConstantBuffer<CameraClip> g_CameraClipCB : register(b0);

float4 main(PS_IN input) : SV_TARGET
{
    float depth = g_Tex.SampleLevel(g_Sampler, input.uv, 0);
    float linearDepth =
        (g_CameraClipCB.near * g_CameraClipCB.far) / (g_CameraClipCB.near + depth * (g_CameraClipCB.far - g_CameraClipCB.near));
    float debugVal = linearDepth / g_CameraClipCB.far;

    return float4(debugVal, debugVal, debugVal, 1.0f);
} // main