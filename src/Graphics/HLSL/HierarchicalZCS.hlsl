// HierarchicalZCS.hlsl
struct HiZ
{
    uint2  InputResolution;
}; // HiZ

ConstantBuffer<HiZ> g_HiZCB : register(b0);
Texture2D<float>    g_InputDepth : register(t0);
RWTexture2D<float>  g_OutputDepth : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (any(DTid.xy >= g_HiZCB.InputResolution))
        return;
    
    // 현재 스레드가 출력할 픽셀을 기반으로 입력 텍스처 좌표 계산
    uint2 srcCoord = DTid.xy * 2;
    
    // 원본 해상도를 벗어나는 스레드 종료
    if (srcCoord.x >= g_HiZCB.InputResolution.x || srcCoord.y >= g_HiZCB.InputResolution.y)
        return;

    // 2x2 픽셀 범위 로드
    // 경계를 넘지 않도록 min으로 방어
    uint3 fetch0 = uint3(min(srcCoord + uint2(0, 0), g_HiZCB.InputResolution - 1), 0);
    uint3 fetch1 = uint3(min(srcCoord + uint2(1, 0), g_HiZCB.InputResolution - 1), 0);
    uint3 fetch2 = uint3(min(srcCoord + uint2(0, 1), g_HiZCB.InputResolution - 1), 0);
    uint3 fetch3 = uint3(min(srcCoord + uint2(1, 1), g_HiZCB.InputResolution - 1), 0);

    float d0 = g_InputDepth.Load(fetch0);
    float d1 = g_InputDepth.Load(fetch1);
    float d2 = g_InputDepth.Load(fetch2);
    float d3 = g_InputDepth.Load(fetch3);

    // Reverse-Z 구조이므로 가장 작은 값/ 가장 먼 깊이값이 오클루전 기준이 됨
    float minDepth = min(min(d0, d1), min(d2, d3));

    // 홀수 해상도일 경우, 잘려나간 우측/하단 1픽셀 라인 추가 검사
    if ((g_HiZCB.InputResolution.x % 2 != 0) && (srcCoord.x + 2 == g_HiZCB.InputResolution.x - 1))
    {
        uint3 fetch4 = uint3(srcCoord.x + 2, fetch0.y, 0);
        uint3 fetch5 = uint3(srcCoord.x + 2, fetch2.y, 0);
        minDepth = min(minDepth, min(g_InputDepth.Load(fetch4), g_InputDepth.Load(fetch5)));
    }
    
    if ((g_HiZCB.InputResolution.y % 2 != 0) && (srcCoord.y + 2 == g_HiZCB.InputResolution.y - 1))
    {
        uint3 fetch6 = uint3(fetch0.x, srcCoord.y + 2, 0);
        uint3 fetch7 = uint3(fetch1.x, srcCoord.y + 2, 0);
        minDepth = min(minDepth, min(g_InputDepth.Load(fetch6), g_InputDepth.Load(fetch7)));
    }

    g_OutputDepth[DTid.xy] = minDepth;
} // main