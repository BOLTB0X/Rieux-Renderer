// PBR.hlsli
#ifndef _PBR_HLSLI_
#define _PBR_HLSLI_

#define PI 3.14159265f

struct PBRVertex
{
    float3 position;
    float2 texcoord;
    float3 normal;
    float3 tangent;
    float3 binormal;
}; // PBRVertex

struct IrradianceCB
{
    uint FaceIndex;
    uint OutputSize;
    float SampleDelta; // 라디안 스텝
}; // IrradianceConstants

struct PrefilterCB
{
    float Roughness;
    uint FaceIndex;
    uint OutputSize;
    uint SampleCount;
}; // PrefilterCB

float Distribution_GGX(float3 N, float3 H, float roughness)
{
    roughness = max(roughness, 0.04f);
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = saturate(dot(N, H));
    float denom = (NdotH * NdotH * (a2 - 1.0f) + 1.0f);
    
    return a2 / max(PI * denom * denom, 0.0001f);
} // Distribution_GGX

float Geometry_Schlick_GGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    
    return NdotV / (NdotV * (1.0f - k) + k);
} // Geometry_schlick_GGX

float Geometry_Smith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    return Geometry_Schlick_GGX(NdotV, roughness)
         * Geometry_Schlick_GGX(NdotL, roughness);
} // geometry_smith

float3 Fresnel_Schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
} // Fresnel_Schlick

float3 Evaluate_Direct_PBR(
    float3 albedo,
    float metallic,
    float roughness,
    float3 N,
    float3 V,
    float3 L,
    float3 radiance,
    float shadow)
{
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    if (NdotV <= 0.0f || NdotL <= 0.0f)
        return 0.0f;

    float3 H = normalize(V + L);

    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float3 F = Fresnel_Schlick(saturate(dot(H, V)), F0);
    float NDF = Distribution_GGX(N, H, roughness);
    float G = Geometry_Smith(N, V, L, roughness);

    float3 numerator = NDF * G * F;
    float3 specular = numerator / max(4.0f * NdotV * NdotL, 0.0001f);

    float3 kS = F;
    float3 kD = (1.0f - kS) * (1.0f - metallic);
    float3 diffuse = kD * albedo / PI;

    return (diffuse + specular) * radiance * NdotL * shadow;
} // Evaluate_Direct_PBR

float3 Get_Face_Direction(uint face, float2 uv)
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
} // Get_Face_Direction

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

float3 Importance_Sample_GGX(float2 Xi, float3 N, float roughness)
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
} // Importance_Sample_GGX

#endif
