// PrefilterEnvironmentCS.hlsl
#include "Commons.hlsli"

TextureCube<float4>      g_SourceCubemap : register(t0);
SamplerState             g_LinearSampler : register(s0);
RWTexture2DArray<float4> g_OutputMipFace : register(u0); // 디스패치마다 특정 (mip,face) 슬라이스 하나

cbuffer PrefilterCB : register(b0)
{
    float Roughness;
    uint  FaceIndex;
    uint  OutputSize; // 이 밉의 해상도 (예: 밉0=256, 밉1=128, ...)
    uint  SampleCount; // 러프니스 높을수록 샘플 수 늘려도 됨 (예: 32~256)
}; // PrefilterCB

float3 GetFaceDirection(uint face, float2 uv) // uv: [-1,1] 범위
{
    // D3D 표준 큐브맵 면 방향 매핑 (+X -X +Y -Y +Z -Z)
    switch (face)
    {
        case 0:
            return normalize(float3(1.0f, -uv.y, -uv.x));
        case 1:
            return normalize(float3(-1.0f, -uv.y, uv.x));
        case 2:
            return normalize(float3(uv.x, 1.0f, uv.y));
        case 3:
            return normalize(float3(uv.x, -1.0f, -uv.y));
        case 4:
            return normalize(float3(uv.x, -uv.y, 1.0f));
        default:
            return normalize(float3(-uv.x, -uv.y, -1.0f));
    }
} // GetFaceDirection

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f;
} // RadicalInverse_VdC

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
} // Hammersley

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    float a = roughness * roughness;
    float phi = 2.0f * PI * Xi.x;
    float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    float3 up = abs(N.z) < 0.999f ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangentX = normalize(cross(up, N));
    float3 tangentY = cross(N, tangentX);

    return normalize(tangentX * H.x + tangentY * H.y + N * H.z);
} // ImportanceSampleGGX

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= OutputSize || dtid.y >= OutputSize)
        return;

    float2 uv = (float2(dtid.xy) + 0.5f) / float(OutputSize) * 2.0f - 1.0f;
    float3 N = GetFaceDirection(FaceIndex, uv);
    float3 R = N;
    float3 V = N;

    float3 prefilteredColor = 0.0f;
    float totalWeight = 0.0f;

    for (uint i = 0; i < SampleCount; ++i)
    {
        float2 Xi = Hammersley(i, SampleCount);
        float3 H = ImportanceSampleGGX(Xi, N, Roughness);
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