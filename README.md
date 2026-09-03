# [Cascaded Shadow Maps (CSM)](https://github.com/BOLTB0X/Rieux-Renderer/tree/CSM/src)

<div align="center">
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/CSM/t_%EC%89%90%EB%8F%84%EC%9A%B0%EB%A7%B52.png?raw=true" width="180" style="border:1px solid #ddd; border-radius:4px;" />
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/CSM/t_%EC%89%90%EB%8F%84%EC%9A%B0%EB%A7%B53.png?raw=true" width="180" style="border:1px solid #ddd; border-radius:4px;" />
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/CSM/t_%EC%89%90%EB%8F%84%EC%9A%B0%EB%A7%B54.png?raw=true" width="180" style="border:1px solid #ddd; border-radius:4px;" />
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/CSM/t_%EC%89%90%EB%8F%84%EC%9A%B0%EB%A7%B51.png?raw=true" width="180" style="border:1px solid #ddd; border-radius:4px;" />
  <br>
  <p><strong> Cascade Level 0 | 1 | 2 | 3 </strong></p>
</div>

이 문제를 해결하기 위해 메인 카메라의 시야 절두체(View Frustum)를 깊이(Z)를 기준으로 여러 개의 구역(Cascade)으로 분할하고,

카메라와 가까운 구역일수록 더 높은 해상도의 그림자 맵을 할당하는 **Cascaded Shadow Maps(CSM)** 기법을 파이프라인에 통합

```text
Main Camera View Frustum
        ↓
Split into N Cascades (e.g., 4 Levels)
        ↓
Shadow Frustum Culling ──→ [Compute Shader] 각 캐스케이드별 독립적인 GPU 컬링 수행
        ↓
Shadow Pass ─────────────→ [Texture2DArray] 분리된 ShadowShader를 통해 깊이 맵에 기록
        ↓
Deferred Lighting Pass ──→ 픽셀의 깊이(Depth)를 기반으로 적절한 Cascade 인덱스를 찾아 PCF 그림자 연산
```

Rieux Renderer는 기존 GPU-Driven 구조의 이점을 극대화하기 위해, 씬 전체를 그림자 맵에 그리지 않고 각 Cascade의 Frustum에 해당하는 인스턴스만 정밀하게 추려내는 'Shadow Frustum Culling'을 구현했습니다.

#### 1) **아키텍처 & 핵심 로직**

- **CSM 버퍼 및 클래스 구조**

    | Buffer | Format | Type | 목적 및 특징 |
    | :--- | :--- | :--- | :--- |
    | **CSM Shadow Map** | `D32_FLOAT` | Texture2DArray | N개의 Cascade 깊이 맵을 배열로 묶어(`g_CSMShadowMap`) 바인딩 오버헤드를 최소화합니다. |
    | **Poisson Disk Array** | `float2[16]` | Static Const | 16개의 무작위 샘플링 오프셋을 하드코딩하여, 규칙적인 패턴을 띄는 일반 PCF의 단점을 극복하고 부드러운 그림자(Soft Shadow)를 만듬 |


- **초기화 일관성** : 엔진 내 클래스 네이밍 규칙에 따라 불필요한 접미사를 배제하고 `CascadedShadowMap` 생성 시 `Init()` 메서드로 직관적인 초기화를 수행

- **Cascade 분할 전략** : 단순 선형 분할(Linear)이나 로그 분할(Logarithmic)의 단점을 보완하기 위해 두 방식을 혼합한 Practical Split Scheme을 사용하여, 근경에서는 그림자의 디테일을 보존하고 원경에서는 넓은 영역을 커버

- **GPU-Driven Shadow Culling** : CSM은 N번의 드로우 콜을 유발하므로 병목이 발생하기 쉬움,
   - 이를 방지하기 위해 씬 메인 카메라용 Frustum Culling과 별개로, 빛의 시점에서 바라본 각 캐스케이드별 Shadow Frustum Culling([ShadowFrustumCuller.h](https://github.com/BOLTB0X/Rieux-Renderer/blob/main/src/Graphics/Techniques/ShadowFrustumCuller.h)/[.cpp](https://github.com/BOLTB0X/Rieux-Renderer/blob/main/src/Graphics/Techniques/ShadowFrustumCuller.cpp))을 Compute Shader로 병렬 처리하여 그림자 렌더링 대상을 최소화합니다

---

#### 2) **Deferred Lighting — CSM 섀도우 판별**

```cpp
// ShadowMap.hlsli (일부 발췌)
int SelectCascade(float viewSpaceDepth)
{
    [loop]
    for (uint i = 0; i < CASCADE_COUNT - 1; ++i)
    {
        if (viewSpaceDepth < CASCADE_SPLITS[i]) return i;
    }
    return CASCADE_COUNT - 1;
}

float Sample_CascadeShadow(float3 positionWS, float3 normalWS, int cascade)
{
    // ... Light NDC 변환 및 Normal-based Bias 계산 생략 ...

    float shadow = 0.0f;
    [unroll]
    for (int i = 0; i < 16; ++i) // Poisson Disk 16-Tap PCF
    {
        float2 sampleUV = shadowUV + poisson_disk[i] * texelSize;
        float shadowDepth = g_CSMShadowMap.SampleLevel(g_ShadowSampler, float3(sampleUV, cascade), 0.0f);
        shadow += lightNDC.z <= shadowDepth + bias ? 1.0f : 0.0f;
    }
    return shadow / 16.0f;
}

float Calculate_CascadeShadow(float3 positionWS, float3 normalWS, float viewSpaceDepth)
{
    int cascade = SelectCascade(viewSpaceDepth);
    float shadow = Sample_CascadeShadow(positionWS, normalWS, cascade);

    if (cascade >= CASCADE_COUNT - 1) return shadow;

    // 캐스케이드 경계선(Seam)을 부드럽게 연결하는 Blending 로직
    float split = CASCADE_SPLITS[cascade];
    float previousSplit = cascade == 0 ? 0.0f : CASCADE_SPLITS[cascade - 1];
    float blendRange = (split - previousSplit) * 0.10f; // 구간의 10%를 블렌드 영역으로 설정
    float blendStart = split - blendRange;

    if (viewSpaceDepth <= blendStart) return shadow;

    float nextShadow = Sample_CascadeShadow(positionWS, normalWS, cascade + 1);
    float blend = saturate((viewSpaceDepth - blendStart) / blendRange);
    return lerp(shadow, nextShadow, blend);
}
```

- [`ShadowMap.hlsli`](https://github.com/BOLTB0X/Rieux-Renderer/blob/main/src/Graphics/HLSL/ShadowMap.hlsli)

Deferred 파이프라인에서 화면을 채우는 픽셀 셰이더 1회 실행 시, 복원된 World Position과 View Depth만으로 배열 형태의 섀도우 맵(`Texture2DArray`) 중 적절한 슬라이스를 찾아 PCF(Percentage Closer Filtering)를 수행하므로 리소스 바인딩 전환 비용이 발생하지 않음

---
