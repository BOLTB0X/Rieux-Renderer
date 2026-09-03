### 3.6 Screen Space Reflection

<div align="center">
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/SSR/04.png?raw=true" width="250" style="border:1px solid #ddd; border-radius:4px;" />
  <br>
  <p><strong> SSR </strong></p>
</div>

Environment Probe 기반 IBL 반사는 프로브가 캡처된 고정 시점 기준으로 미리 구워진 정적 데이터

카메라가 그 지점에서 멀어질수록 실제 눈에 보여야 할 반사와 프로브에 저장된 반사 사이에 **시차(Parallax)** 오차가 벌어

Screen Space Reflection(SSR)은 현재 프레임의 G-Buffer(Depth, Normal)와 이미 라이팅이 끝난 Scene Color를 그대로 재활용해, 카메라 시점에 맞는 반사를 실시간으로 근사

```
G-Buffer Depth / Normal
        ↓
View Space Position / Normal 복원
        ↓
Reflection Ray 생성 (Surface Bias 적용)
        ↓
Screen Space Ray Marching (선형 전진 → 이진 탐색 정밀화)
        ↓
Depth Intersection 검사
        ↓
Hit 성공 → Scene Color 샘플링 → 합성
Hit 실패 → Deferred Lighting 결과(Environment Probe IBL 포함) 그대로 사용
```

SSR은 별도의 반사 전용 씬을 렌더링하지 않고, Deferred Lighting Pass가 만든 Scene Color 자체를 반사 대상 텍스처로 재사용

**DeferredLightingPass → EndRender(SRV 전환) → ScreenSpaceReflectionPass로 이어지는 순서 덕분에, SSR이 참조하는 LitSceneTex는 이미 Direct Lighting과 IBL이 합산된 최종 컬러입니다.**

---

#### 1) **아키텍처 & 핵심 로직**

1. **Reflection Ray 생성 — View Space 기준**

    - G-Buffer Normal(월드 공간)을 VIEW 행렬의 3x3 회전 성분만으로 뷰 공간으로 변환한 뒤, 뷰 방향 벡터와 reflect()로 반사 방향을 계산

2. **Self-Intersection 방지 — Surface Bias**

    - 레이 시작점을 표면 그 자체가 아니라 노멀 방향으로 살짝 띄운 지점(Ray Origin = ViewPos + Normal × Bias)에서 출발시켜, 얕은 각도의 반사가 자기 자신의 표면과 즉시 재충돌하는 것을 방지

3. **2단계 Ray Marching — 선형 탐색 후 이진 탐색 정밀화**

    - 1단계: 고정 보폭으로 선형 전진하며 Ray Depth > Scene Depth(레이가 물체 뒤로 들어간 순간)를 탐지
    - 2단계: 탐지된 구간 안에서 5회 이진 탐색으로 픽셀 단위의 정밀한 교차점을 좁힘

4. **거리 비례 Thickness — 원근 왜곡 보정**

    - 원근 나눗셈 특성상 카메라에서 멀어질수록 같은 화면 픽셀이 커버하는 뷰 공간 깊이 범위가 넓어짐. 고정된 두께 허용치(Thickness)를 쓰면 먼 거리에서 교차 판정을 거의 놓치므로, 레이가 전진한 거리에 비례해 허용 두께를 늘림

5. **SSR Fallback — Environment Probe IBL로 자연스럽게 대체**

    - 레이가 화면 밖으로 나가거나, 최대 스텝 안에 교차점을 못 찾으면 Deferred Lighting Pass가 이미 계산해둔 sceneColor(Environment Probe IBL 포함)를 그대로 반환

---

#### 2) **Reflection Ray & Self-Intersection 방지**

```cpp
// ScreenSpaceReflectionPS.hlsl
float3 GetViewPosition(float2 uv, float depth)
{
    float4 clipSpacePos = float4(uv.x * 2.0f - 1.0f, (1.0f - uv.y) * 2.0f - 1.0f, depth, 1.0f);
    float4 viewSpacePos = mul(clipSpacePos, PROJ_INV);
    return viewSpacePos.xyz / viewSpacePos.w;
} // GetViewPosition

float3 GetViewNormal(float2 uv)
{
    float3 worldNormal = GBuffer1.SampleLevel(PointClampSampler, uv, 0).xyz * 2.0f - 1.0f;
    return normalize(mul(worldNormal, (float3x3) VIEW));
} // GetViewNormal

// ... main() 내부
float3 viewNormal = GetViewNormal(uv);
float3 viewPos    = GetViewPosition(uv, depth);
float3 reflectDir = normalize(reflect(normalize(viewPos), viewNormal));

// Surface Bias: 자기 자신과의 재교차 방지
float3 rayPos = viewPos + viewNormal * surfaceBias;
```

- [`ScreenSpaceReflectionPS.hlsl`](https://github.com/BOLTB0X/Rieux-Renderer/blob/SSR/src/Graphics/HLSL/ScreenSpaceReflectionPS.hlsl)

---

#### 3) **2단계 Ray Marching과 거리 비례 Thickness**

<div align="center">
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/SSR/t_ssr-%ED%9E%88%ED%8A%B8-%EB%B9%A8-%EC%95%84%EB%8B%98%ED%8C%8C.png?raw=true" width="250" style="border:1px solid #ddd; border-radius:4px;" />
  <br>
  <p><strong> 레이마칭 Hit </strong></p>
</div>

```cpp
for (int i = 0; i < maxSteps; ++i)
{
    rayPos += reflectDir * stepSize;

    float4 clipPos = mul(float4(rayPos, 1.0f), PROJ);
    float2 sampleUV = (clipPos.xy / clipPos.w) * 0.5f + 0.5f;
    sampleUV.y = 1.0f - sampleUV.y;

    if (sampleUV.x < 0.0f || sampleUV.x > 1.0f || sampleUV.y < 0.0f || sampleUV.y > 1.0f)
        break; // 화면 밖 이탈 → Fallback

    float sampleDepth = DepthTex.SampleLevel(PointClampSampler, sampleUV, 0).r;
    if (sampleDepth <= 0.0f) continue; // Reverse-Z 기준 far plane(하늘) 무시

    float3 sampleViewPos = GetViewPosition(sampleUV, sampleDepth);
    float  depthDiff = rayPos.z - sampleViewPos.z;

    // 거리 비례 Thickness - 원근 나눗셈으로 벌어지는 오차를 보정
    float dynamicThickness = max(minThickness, abs(rayPos.z) * thicknessScale);

    if (depthDiff > 0.0f && depthDiff < dynamicThickness)
    {
        // 이진 탐색으로 교차점 정밀화 (5회 반복, 보폭을 매번 절반으로 축소)
        rayPos -= reflectDir * stepSize;
        stepSize *= 0.5f;
        for (int j = 0; j < 5; ++j)
        {
            rayPos += reflectDir * stepSize;
            // ... 재투영 및 뎁스 비교, 오버슈트 시 되돌리고 stepSize 재차 절반
            stepSize *= 0.5f;
        }
        hit = true;
        break;
    }
}
```

- [`ScreenSpaceReflectionPS.hlsl`](https://github.com/BOLTB0X/Rieux-Renderer/blob/SSR/src/Graphics/HLSL/ScreenSpaceReflectionPS.hlsl)

바닥처럼 카메라와 거의 평행한 표면에서는, 예측된 레이 위치와 실제 씬 깊이가 둘 다 카메라 거리에 대해 거의 선형으로 증가하기 때문에 `depthDiff`가 아주 좁은 구간에서만 조건을 만족

고정 `Thickness` 를 쓰면 이 구간이 화면상 얇은 띠 형태로만 검출되어 반사가 군데군데 끊기는 문제가 발생했고, `Thickness` 를 레이 진행 거리에 비례해 넓히는 방식으로 해결

---

#### 4) **SSR Confidence — 합성 가중치 결합**

```cpp
float2 fade = smoothstep(0.0f, 0.1f, hitUV) * (1.0f - smoothstep(0.9f, 1.0f, hitUV));
float screenEdgeFade = fade.x * fade.y;

float reflectionStrength = (1.0f - roughness) * screenEdgeFade;
return float4(lerp(sceneColor, hitColor, reflectionStrength), 1.0f);
```

```
SSR Confidence
=
RoughnessFade
×
ScreenEdgeFade
×
HitConfidence
SSR Confidence=RoughnessFade×ScreenEdgeFade×HitConfidence
```

러프니스가 낮을수록(매끄러운 표면일수록) SSR의 기여도가 커지고, 러프니스가 높은 표면은 애초에 SSR 컷오프(`roughness > 0.35`)에서 걸러져 Environment Probe IBL이 전담

화면 가장자리에서는 레이가 화면을 벗어나 정보가 없는 것과 실제로 반사가 없는 것을 구분할 수 없으므로, `screenEdgeFade` 로 경계 근처의 반사 강도를 부드럽게 낮춰 끊김을 완화

---

#### 5) **SSR의 한계와 완화**

<div align="center">

| 한계 | 완화 방법 |
| :--- | :--- |
| **화면 밖/뒤쪽 오브젝트 반사 불가** | Environment Probe IBL로 Fallback |
| **얇은 지오메트리를 레이가 건너뛰어 놓침** | 거리 비례 Thickness로 완화 (완전 해결은 아님) |
| **화면 가장자리 반사 왜곡·끊김** | Screen Edge Fade로 시각적 완화 |
| **얕은 각도에서 자기 자신과 재교차** | Surface Bias + 초기 스텝 히트 판정 유예 |
| **카메라 이동 시 Temporal Ghosting 가능성** | (향후 과제) Temporal Reprojection 미적용 상태 |

</div>

---

#### 6) 트러블슈팅

<div align="center">


| 증상 | 원인 | 해결 |
| :--- | :--- | :--- |
| **SSR이 히트해도 반사 결과가 원본 화면과 거의 동일하게 보임(반사가 "안 되는 것"처럼 보임)** | 레이 시작점이 표면 자체(`rayPos = viewPos`)라, 얕은 각도에서 첫 스텝만에 자기 자신을 재감지 | `viewPos + viewNormal * surfaceBias`로 시작점을 노멀 방향으로 오프셋 + 첫 스텝 히트 판정 유예 |
| **히트 디버그 시각화(빨강=Hit/파랑=Miss) 결과 바닥 전체가 얇은 대각선 띠 하나만 빨갛게 나옴** | 고정된 좁은 Thickness가 원근 나눗셈으로 벌어지는 깊이 오차 폭을 못 따라감 | Thickness를 `abs(rayPos.z) * thicknessScale`로 레이 진행 거리에 비례하도록 동적화 |
| **반사된 색이 프로브가 캡처된 지점 기준의 오래된 이미지처럼 보임** | SSR이 실패해 Fallback으로 빠진 `sceneColor`에 이미 (부정확한) 정적 IBL 스펙큘러가 섞여 있었음 | SSR 히트율 자체를 개선(위 두 항목)해 Fallback 발동 빈도를 최소화 — Fallback을 없애는 대신 "거의 발동하지 않는 안전망"으로 축소 |
| **SSR 컷오프(roughness > 0.35) 근처 픽셀에서 반사가 부자연스럽게 뚝 끊김** | Deferred Lighting Pass의 정적 스펙큘러 감쇠 구간과 SSR의 컷오프 값이 서로 다르게 하드코딩됨 | 두 셰이더가 참조하는 컷오프 상수를 공유 헤더 하나로 통일 |


</div>