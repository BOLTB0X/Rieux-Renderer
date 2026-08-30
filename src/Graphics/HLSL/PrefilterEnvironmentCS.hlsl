// PrefilterEnvironmentCS.hlsl
#include "PBR.hlsli"

TextureCube<float4>         g_SourceCubemap : register(t0);
SamplerState                g_LinearSampler : register(s0);
RWTexture2DArray<float4>    g_OutputMipFace : register(u0);
ConstantBuffer<PrefilterCB> g_PrefilterCB : register(b0);

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= g_PrefilterCB.OutputSize || dtid.y >= g_PrefilterCB.OutputSize)
        return;

    float2 uv = (float2(dtid.xy) + 0.5f) / float(g_PrefilterCB.OutputSize) * 2.0f - 1.0f;
    float3 N = Get_Face_Direction(g_PrefilterCB.FaceIndex, uv);
    float3 R = N;
    float3 V = N;

    float3 prefilteredColor = 0.0f;
    float totalWeight = 0.0f;

    for (uint i = 0; i < g_PrefilterCB.SampleCount; ++i)
    {
        float2 Xi = Hammersley(i, g_PrefilterCB.SampleCount);
        float3 H = Importance_Sample_GGX(Xi, N, g_PrefilterCB.Roughness);
        float3 L = normalize(2.0f * dot(V, H) * H - V);

        float NdotL = saturate(dot(N, L));
        if (NdotL > 0.0f)
        {
            prefilteredColor += g_SourceCubemap.SampleLevel(g_LinearSampler, L, 0.0f).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor = totalWeight > 0.0f ? prefilteredColor / totalWeight : 0.0f;
    g_OutputMipFace[uint3(dtid.xy, 0)] = float4(prefilteredColor, 1.0f);
} // main