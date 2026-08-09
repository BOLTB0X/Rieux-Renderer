// Culling.hlsli
#ifndef _CULLING_HLSLI_
#define _CULLING_HLSLI_

struct FrustumCullingCB
{
    float4 frustumPlanes[6];
    uint   totalInstances;
    float3 fPadding;
}; // FrustumCullingCB

struct OcclusionCB
{
    matrix viewProjection;
    float2 screenSize;
    float2 oPadding;
}; // OcclusionCB

// AABB와 프러스텀 교차 판정 함수
bool Check_Box_Visible(float3 aabbMin, float3 aabbMax, matrix world, float4 planes[6])
{
    // 상자의 8개 꼭짓점을 명시적으로 생성
    float3 corners[8] =
    {
        float3(aabbMin.x, aabbMin.y, aabbMin.z), // min, min, min
        float3(aabbMax.x, aabbMin.y, aabbMin.z), // max, min, min
        float3(aabbMin.x, aabbMax.y, aabbMin.z), // min, max, min
        float3(aabbMax.x, aabbMax.y, aabbMin.z), // max, max, min
        float3(aabbMin.x, aabbMin.y, aabbMax.z), // min, min, max
        float3(aabbMax.x, aabbMin.y, aabbMax.z), // max, min, max
        float3(aabbMin.x, aabbMax.y, aabbMax.z), // min, max, max
        float3(aabbMax.x, aabbMax.y, aabbMax.z) // max, max, max
    };
    
    [unroll]
    for (int k = 0; k < 8; ++k)
    {
        corners[k] = mul(float4(corners[k], 1.0f), world).xyz;
    }

    [unroll]
    for (int i = 0; i < 6; ++i)
    {
        float3 normal = planes[i].xyz;
        float d = planes[i].w;
        
        bool isInsideAny = false;

        [unroll]
        for (int j = 0; j < 8; ++j)
        {
            if (dot(normal, corners[j]) + d >= 0.0f)
            {
                isInsideAny = true;
                break;
            }
        } // for (int j = 0; j < 8; ++j)

        // 8개 꼭짓점이 모두 평면 밖(< 0)이라면 완전히 잘려나간 것
        if (!isInsideAny)
        {
            return false;
        }
    } // for (int i = 0; i < 6; ++i)

    return true;
} // CheckBoxVisible

float3 Get_BoxCorner(float3 aabbMin, float3 aabbMax, uint index)
{
    return float3(
        (index & 1) ? aabbMax.x : aabbMin.x,
        (index & 2) ? aabbMax.y : aabbMin.y,
        (index & 4) ? aabbMax.z : aabbMin.z
    );
} // GetBoxCorner

#endif