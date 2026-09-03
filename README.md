## Deferred Light Rendering(PBR)

<div align="center">
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/Deferred/PBR07.png?raw=true" width="450" style="border:1px solid #ddd; border-radius:4px;" />
  <br>
  <p><strong> Deferred Lighting Rendering </strong></p>
</div>

> 기존 Forward 파이프라인은 오브젝트를 그리는 픽셀 셰이더 안에서 텍스처 샘플링과 라이팅 계산을 한 번에 수행

이 방식은 오브젝트 수, 라이트 수가 늘어날수록 픽셀당 반복 계산 비용이 커지고, 특히 오버드로우가 많은 씬에서는 이미 가려질 픽셀에 대해서도 조명 계산을 낭비하게 됨

```
Depth Pass & Hi-Z
        ↓
Shadow Pass (CSM)
        ↓
Occlusion Culling Phase 2
        ↓
G-Buffer Pass          ← 표면 정보만 기록, 조명 계산 없음
        ↓
Deferred Lighting Pass  ← 화면 전체 1회, Direct + Shadow + IBL 합산
        ↓
Screen Space Reflection ← Scene Color를 반사 소스로 재활용
```

**Rieux Renderer는 GPU-Driven 파이프라인을 유지한 채 G-Buffer 기록(Geometry Pass) 과 화면 전체를 대상으로 한 조명 계산(Lighting Pass) 을 분리하는 Deferred Shading으로 전환**

---

#### 1) **아키텍처 & 핵심 로직**

1. G-Buffer 구성 — 2개 렌더타겟 + 기존 Depth 재사용

    | Buffer | Format | 저장 데이터 |
    | :--- | :--- | :--- |
    | **GBuffer0** | `R8G8B8A8_UNORM` | Albedo.rgb, Metallic.a |
    | **GBuffer1** | `R16G16B16A16_FLOAT` | World Normal.rgb (\*0.5+0.5 인코딩), Roughness.a |
    | **Depth** | 기존 `KEY_DEPTH_RENDER_TEXTURE` | Reverse-Z 깊이 (신규 생성 없이 재사용) |

    - Depth를 새로 만들지 않고 재사용한 이유는, Two-Phase Occlusion Culling 구조상 Phase 2에서 확정된 최종 가시 인스턴스 집합이 이미 Phase 1이 채워둔 깊이 버퍼의 부분집합이기 때문
    
    - G-Buffer Pass는 DepthFunc = EQUAL, DepthWrite = 0으로 컬러만 채워, 깊이 버퍼를 다시 쓰는 비용 없이 오버드로우도 억제

2. G-Buffer Pass — 조명 계산 없이 표면 정보만 기록

    - 기존 `GPU-Driven Vertex Pulling`([`GPU_PBRModelVS.hlsl`](https://github.com/BOLTB0X/Rieux-Renderer/blob/Image-Based-Lighting/src/Graphics/HLSL/GPU_PBRModelVS.hlsl))을 그대로 재사용하고, 픽셀 셰이더만 라이팅 로직을 들어내고 MRT로 표면 데이터만 씀

3. Deferred Lighting Pass — 풀스크린 트라이앵글 1회
    
    - 버텍스/인덱스 버퍼 없이 `SV_VertexID`만으로 화면 전체를 덮는 삼각형 하나를 생성([`FullScreenVS.hlsl`](https://github.com/BOLTB0X/Rieux-Renderer/blob/Image-Based-Lighting/src/Graphics/HLSL/FullScreenVS.hlsl))

    - Depth + Inverse View/Projection으로 월드 좌표를 복원한 뒤, G-Buffer 채널을 읽어 **Direct Lighting + CSM Shadow + Diffuse/Specular IBL을 합산**

4. Environment Probe — Projection Box와 Influence Box 분리

    - **Projection Box** : Parallax-Corrected Cubemap 계산에 사용되는 가상의 박스. 프로브가 캡처된 지점을 기준으로, 반사/조도 벡터가 이 박스 표면과 만나는 지점을 재계산해 큐브맵의 원근 왜곡을 보정

    - **Influence Box** : 이 프로브가 실제로 얼마나 영향을 미칠지 결정하는 블렌드 영역. 표면의 월드 좌표가 이 박스 경계에 가까워질수록 smoothstep으로 가중치를 0까지 부드럽게 낮춤

---

#### 2) **G-Buffer Pass — MRT 기록**

<div align="center">
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/IBL/IBL04_G%EB%B2%84%ED%8D%BC.png?raw=true" width="350" style="border:1px solid #ddd; border-radius:4px;" />
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/SSR/t_%ED%94%84%EB%A1%9C%EB%B8%8C%EB%B2%94%EC%9C%84.png?raw=true" width="350" style="border:1px solid #ddd; border-radius:4px;" />
  <br>
  <p><strong> G-Buffers </strong></p>
</div>

```cpp
// GPU_GBufferPS.hlsl
struct PS_OUT
{
    float4 gbuffer0 : SV_TARGET0; // albedo.rgb, metallic
    float4 gbuffer1 : SV_TARGET1; // normalWS.rgb, roughness
}; // PS_OUT

PS_OUT main(PS_IN input)
{
    MeshInstanceData inst = g_InstanceData[g_InstanceCB.InstanceIndex];
    // ... 알베도/노멀/러프니스/메탈릭 샘플링 (기존 Forward PS와 동일 로직)

    PS_OUT output;
    output.gbuffer0 = float4(albedoColor.rgb, metallic);
    output.gbuffer1 = float4(normalWS * 0.5f + 0.5f, roughness);
    return output;
} // main
```

- [`Renderer::GBufferPass(ID3D12GraphicsCommandList* cmdList`](https://github.com/BOLTB0X/Rieux-Renderer/blob/Image-Based-Lighting/src/Core/Renderer.cpp#L990)

- [`GPU_GBufferPS.hlsl`](https://github.com/BOLTB0X/Rieux-Renderer/blob/Image-Based-Lighting/src/Graphics/HLSL/GPU_GBufferPS.hlsl)

이 패스는 순수하게 표면 정보만 기록하고, 라이팅과 관련된 어떤 텍스처(그림자맵, 환경맵 등)도 참조하지 않음

라이팅 계산 코드는 전부 `Deferred Lighting Pass` 로 이관되어, `G-Buffer PSO`는 `Forward PSO` 대비 바인딩할 리소스 수와 셰이더 분기가 크게 줄어듬

---

#### 3) **Deferred Lighting Pass — 월드 좌표 복원과 Parallax-Corrected IBL**

```cpp
// GPU_DeferredLightingPS.hlsl
float3 ReconstructWorldPos(float2 uv, float depth)
{
    float4 clipPos = float4(uv.x * 2.0f - 1.0f, (1.0f - uv.y) * 2.0f - 1.0f, depth, 1.0f);
    float4 worldPos = mul(clipPos, PROJ_INV);
    worldPos = mul(worldPos, VIEW_INV);
    return worldPos.xyz / worldPos.w;
} // ReconstructWorldPos

float3 ParallaxCorrection(float3 dir, float3 positionWS, float3 probePosition, float3 boxMin, float3 boxMax)
{
    float3 firstPlaneIntersect  = (boxMax - positionWS) / dir;
    float3 secondPlaneIntersect = (boxMin - positionWS) / dir;
    float3 furthestPlane = max(firstPlaneIntersect, secondPlaneIntersect);
    float  distance = min(min(furthestPlane.x, furthestPlane.y), furthestPlane.z);

    float3 intersectPosition = positionWS + dir * distance;
    return intersectPosition - probePosition;
} // ParallaxCorrection

float CalculateProbeWeight(float3 positionWS, float3 boxMin, float3 boxMax, float blendDistance)
{
    float3 fadeMin = smoothstep(boxMin, boxMin + blendDistance, positionWS);
    float3 fadeMax = smoothstep(boxMax, boxMax - blendDistance, positionWS);
    float3 fade = fadeMin * fadeMax;
    return fade.x * fade.y * fade.z;
} // CalculateProbeWeight

float4 main(PS_IN input) : SV_TARGET
{
    // G-Buffer + Depth Load → albedo, metallic, normalWS, roughness, positionWS 복원

    float3 directTerm = Evaluate_Direct_PBR(albedo, metallic, roughness, N, V, L,
                                             LIGHT_DIFFUSE.rgb, shadowFactor);

    float probeWeight  = CalculateProbeWeight(positionWS, boxMin, boxMax, blendDistance);
    float3 correctedN  = ParallaxCorrection(N, positionWS, probePos, boxMin, boxMax);
    float3 correctedR  = ParallaxCorrection(reflect(-V, N), positionWS, probePos, boxMin, boxMax);

    float3 diffuseIBL  = kD * albedo * g_IrradianceMap.Sample(LinearSampler, correctedN).rgb;
    float3 prefiltered = g_PrefilteredMap.SampleLevel(LinearSampler, correctedR, roughness * MAX_PREFILTER_MIP).rgb;
    float2 brdf = g_BRDFLUT.Sample(LinearSampler, float2(saturate(dot(N, V)), roughness));
    float3 specularIBL = prefiltered * (F0 * brdf.x + brdf.y);

    float3 ambientIBL = (diffuseIBL + specularIBL) * probeWeight;
    return float4(directTerm + ambientIBL, 1.0f);
} // main
```

- [GPU_DeferredLightingPS.hlsl](https://github.com/BOLTB0X/Rieux-Renderer/blob/Image-Based-Lighting/src/Graphics/HLSL/GPU_DeferredLightingPS.hlsl)

- `Influence Box` 는 "이 프로브가 이 지점에 얼마나 기여할지"를 결정하는 블렌드 반경이고,

- `Projection Box` 는 "큐브맵을 어떤 가상의 벽 안쪽에서 캡처한 것으로 취급할지"를 결정하는 보정 범위

두 값을 분리해두면, 나중에 프로브를 여러 개 배치해 그리드로 확장할 때 각 프로브의 블렌드 경계와 투영 보정 범위를 독립적으로 튜닝할 수 있음

---

#### 4) 트러블슈팅

<div align="center">


| 증상 | 원인 | 해결 |
| :--- | :--- | :--- |
| **G-Buffer 렌더타겟에 UAV 큐브맵 밉/면별 인덱스를 조회하면 INVALID_DESCRIPTOR_HANDLE 발생** | `RenderTexture::Init`에서 밉·면별 UAV/SRV 생성 블록이 `m_mipLevels > 1` 조건에 걸려 있어, 밉이 1개뿐인 큐브맵(Irradiance Map 등)은 디스크립터 자체가 생성되지 않음 | 조건에서 `m_mipLevels > 1`을 제거하고, 밉 1개일 때는 바깥 루프가 자연히 1회만 돌도록 구조를 통일 |
| **Parallax 보정 후 카메라가 이동할 때마다 반사·주변광이 미세하게 계속 흔들림** | `ParallaxCorrection`에 넘기는 `probePos`를 프로브가 실제로 캡처된 고정 위치가 아니라 `CAMERA_POSITION`(매 프레임 바뀌는 값)으로 잘못 대입 | 프로브 캡처 시점의 실제 위치를 `LightCB`에 실어 GPU로 전달하고, 셰이더는 그 값을 `probePos`로 사용하도록 수정 |
| **하늘/배경 픽셀이 검정이 아니라 렌더타겟 클리어 컬러(파란빛)로 남음** | `depth <= 0.0f`(Reverse-Z 기준 far plane)일 때 `discard`를 사용 — `discard`는 아무것도 쓰지 않고 넘어가므로 클리어 컬러가 그대로 노출됨 | 무조건 출력 컬러를 검정(`float4(0,0,0,1)`) 등으로 덮어쓰거나, `discard` 대신 깊이 테스트 합격 후 배경색을 직접 쓰도록 수정 |

</div>

--- 