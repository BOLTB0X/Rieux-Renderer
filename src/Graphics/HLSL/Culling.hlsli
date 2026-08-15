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
    uint   mainCapacity;
    uint   vaseCapacity;
    float2 oPadding;
}; // OcclusionCB

struct ProjectedAABB
{
    float2 uvMin;
    float2 uvMax;
    float  closestZ;
    bool  isValid;
}; // ProjectedAABB

struct PhaseInfo
{
    uint isPhase2;
    uint hasPreviousHiz;
}; // PhaseInfo

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

// AABB 투영 및 NDC 바운딩 박스 계산 함수
ProjectedAABB Get_ProjectedAABB(float3 aabbMin, float3 aabbMax, matrix MVP)
{
    ProjectedAABB result = (ProjectedAABB)0;
    float minX = 1.0f, minY = 1.0f;
    float maxX = -1.0f, maxY = -1.0f;
    result.closestZ = -1.0f;

    [unroll]
    for (uint i = 0; i < 8; ++i)
    {
        float3 corner = Get_BoxCorner(aabbMin, aabbMax, i);
        float4 clipPos = mul(float4(corner, 1.0f), MVP);
        
        if (clipPos.w <= 0.0f)
            clipPos.w = 0.0001f;
        
        float3 ndc = clipPos.xyz / clipPos.w;
        
        if (any(isnan(ndc)) || any(isinf(ndc)))
        {
            return result;
        }

        minX = min(minX, ndc.x);
        minY = min(minY, ndc.y);
        maxX = max(maxX, ndc.x);
        maxY = max(maxY, ndc.y);
        
        result.closestZ = max(result.closestZ, ndc.z);
    }
    
    // 화면 밖으로 완전히 벗어났는지 체크
    if (minX > 1.0f || maxX < -1.0f || minY > 1.0f || maxY < -1.0f)
    {
        result.isValid = false;
        return result;
    }

    result.uvMin = saturate(float2(minX * 0.5f + 0.5f, -maxY * 0.5f + 0.5f));
    result.uvMax = saturate(float2(maxX * 0.5f + 0.5f, -minY * 0.5f + 0.5f));
    result.isValid = true;

    return result;
} // Get_ProjectedAABB

bool Is_Occluded(ProjectedAABB projAABB, float2 screenSize, Texture2D<float> hiZTex, SamplerState pointSampler)
{
    float2 sizeInPixels = (projAABB.uvMax - projAABB.uvMin) * screenSize;
    float objectMipLevel = ceil(log2(max(max(sizeInPixels.x, sizeInPixels.y), 1.0f)));
    float maxMipLevel = floor(log2(max(max(screenSize.x, screenSize.y), 1.0f)));
    float mipLevel = min(objectMipLevel, maxMipLevel);

    float d0 = hiZTex.SampleLevel(pointSampler, float2(projAABB.uvMin.x, projAABB.uvMin.y), mipLevel).r;
    float d1 = hiZTex.SampleLevel(pointSampler, float2(projAABB.uvMax.x, projAABB.uvMin.y), mipLevel).r;
    float d2 = hiZTex.SampleLevel(pointSampler, float2(projAABB.uvMin.x, projAABB.uvMax.y), mipLevel).r;
    float d3 = hiZTex.SampleLevel(pointSampler, float2(projAABB.uvMax.x, projAABB.uvMax.y), mipLevel).r;

    float maxOccluderDepth = min(min(d0, d1), min(d2, d3));

    // 내 물체가 가림막보다 뒤에 있다면 true 반환
    return (projAABB.closestZ < maxOccluderDepth);
} // Is_Occluded

#endif
