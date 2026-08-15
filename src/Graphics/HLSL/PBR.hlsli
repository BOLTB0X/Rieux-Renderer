// PBR.hlsli
#ifndef _PBR_HLSLI_
#define _PBR_HLSLI_
#include "Commons.hlsli"

struct PBRVertex
{
    float3 position;
    float2 texcoord;
    float3 normal;
    float3 tangent;
    float3 binormal;
}; // PBRVertex

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

#endif
