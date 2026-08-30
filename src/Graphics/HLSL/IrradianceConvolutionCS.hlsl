// IrradianceConvolutionCS.hlsl
#include "PBR.hlsli"

TextureCube<float4>          g_SourceCubemap : register(t0);
SamplerState                 g_LinearSampler : register(s0);
RWTexture2DArray<float4>     g_OutputFace : register(u0);
ConstantBuffer<IrradianceCB> g_IrradianceCB : register(b0);

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= g_IrradianceCB.OutputSize || dtid.y >= g_IrradianceCB.OutputSize)
        return;

    float2 uv = (float2(dtid.xy) + 0.5f) / float(g_IrradianceCB.OutputSize) * 2.0f - 1.0f;
    float3 N = Get_Face_Direction(g_IrradianceCB.FaceIndex, uv);

    float3 up = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    float3 irradiance = 0.0f;
    uint sampleCount = 0;

    for (float phi = 0.0f; phi < 2.0f * PI; phi += g_IrradianceCB.SampleDelta)
    {
        for (float theta = 0.0f; theta < 0.5f * PI; theta += g_IrradianceCB.SampleDelta)
        {
            float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

            irradiance += g_SourceCubemap.SampleLevel(g_LinearSampler, sampleVec, 0.0f).rgb * cos(theta) * sin(theta);
            ++sampleCount;
        }
    }

    irradiance = PI * irradiance / float(sampleCount);
    g_OutputFace[uint3(dtid.xy, 0)] = float4(irradiance, 1.0f);
} // main