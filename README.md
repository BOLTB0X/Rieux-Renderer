# Two Phase Occlusion Culling

<div align="center">
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/Two_Phase_Occlusion_Culling/Two_Phase_Occlusion_Culling02_masterScene01.gif?raw=true" width="250" style="border:1px solid #ddd; border-radius:4px;" />
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/GPU_Driven/06_Occlusion%20Culling03.png?raw=true" width="250" style="border:1px solid #ddd; border-radius:4px;" />
  <br>
  <p><strong> Occlusion Culling </strong></p>
</div>

```
Phase 1 (이전 프레임 Hi-Z로 선별) → Depth Pass & Hi-Z Rebuild (Phase 1 생존자만으로) → Phase 2 (갱신된 Hi-Z로 Phase 1 탈락자 재검사) → Final Draw
```

이전 프레임의 Hi-Z만으로 가시성을 판정하면,

**이번 프레임에 새로 드러난 오브젝트**(카메라가 회전하거나 앞을 가리던 물체가 사라진 경우)는 실제로는 보여야 하는데도 지난 프레임 기준 정보 때문에 잘못 컬링(False Negative)될 수 있음

Two Phase Occlusion Culling은 **한 프레임 안에서 Hi-Z를 두 번 활용** 해 이 문제를 해결하는 GPU-Driven 렌더링의 핵심 기법

---

#### 1) **아키텍처 & 핵심 로직**

```
Master Indirect Commands
        ↓
Frustum Culling
        ↓
Frustum Visible Commands
        ↓
Occlusion Phase 1
        ├── Visible → Final Commands
        └── Occluded → Phase 2 Candidate
        ↓
Depth Pass
        ↓
Current Frame Hi-Z
        ↓
Occlusion Phase 2
        ├── Visible → Final Commands
        └── Occluded → Drop
```

1. **Phase 1 — 이전 프레임 Hi-Z 기반 1차 선별**

   - Frustum Culling을 통과한 오브젝트(`FrustumMainCommands`/`FrustumVaseCommands`)를 입력으로, 각 인스턴스의 AABB를 현재 ViewProj로 투영(`Get_ProjectedAABB`)

   - N-1 프레임에 만들어둔 Hi-Z 텍스처를 샘플링해 오클루전 여부 판정(`Is_Occluded`) → 통과 시 `Final` 버퍼에 즉시 기록, 탈락 시 `Culled` 버퍼에 별도 보관 (바로 버리지 않고 Phase 2 재검사 대상으로 남김)

2. **Depth Pass & Hi-Z Rebuild**

   - Phase 1을 통과한 `Final` 버퍼로 Depth-Only 렌더링을 수행하고, 그 결과로 **이번 프레임 기준 Hi-Z**를 새로 빌드

   - 이 시점의 Hi-Z는 "이번 프레임에 실제로 보이는 것으로 확정된 오브젝트"만 반영하므로, 이전 프레임 대비 최신 정보를 담고 있음

3. **Phase 2 — 갱신된 Hi-Z로 탈락자 재검사**

   - Phase 1에서 `Culled` 버퍼로 넘어간 오브젝트만 대상으로, 방금 만든 **현재 프레임 Hi-Z**로 재검사

   - 이번에 통과하면 `Final` 버퍼에 Append(같은 버퍼에 이어서 기록되어 최종 드로우 시 Phase 1/2 생존자가 하나로 합쳐짐), 재차 탈락하면 그대로 Drop (더 이상의 재검사 없음)

---

#### 2) **Main / Vase 그룹 분리 및 버퍼 초기화**

```cpp
[Init 단계]
    OcclusionCuller::Init(InitParams)
        ├─ Main 그룹 자원 생성 (BuildGroupResources)
        │     └─ FinalCommands(UAV) + FinalCounter(UAV) — Indirect Argument 상태로 초기화
        ├─ Vase 그룹 자원 생성 (BuildGroupResources)
        │     └─ FinalCommands(UAV) + FinalCounter(UAV)
        ├─ Phase1 Culled 버퍼 4종 생성 (Main/Vase × Commands/Counter)
        │     └─ NON_PIXEL_SHADER_RESOURCE 상태로 초기화 (Phase 2에서 SRV로 재사용)
        ├─ Phase1 Culled 버퍼에 대한 SRV 4종 별도 생성 (Phase 2 입력용)
        └─ Readback 버퍼(Main/Vase) 및 더미 UAV 생성 (Phase 2에서 미사용 슬롯 채움용)
```

- [`OcclusionCuller.h`](https://github.com/BOLTB0X/Rieux-Renderer/blob/GPU-Driven/src/Graphics/Techniques/OcclusionCuller.h) / [`.cpp`](https://github.com/BOLTB0X/Rieux-Renderer/blob/GPU-Driven/src/Graphics/Techniques/OcclusionCuller.cpp)

- 인스턴스를 **Main / Vase** 두 그룹으로 나눠 각각 독립된 `Command/Counter` 버퍼를 운용합니다. 오브젝트 종류별로 버퍼 용량과 생존 카운트를 분리 추적할 수 있어, 특정 그룹의 오버플로우가 다른 그룹에 영향을 주지 않도록 설계

---

#### 3) **매 프레임 Phase 1 → Depth/Hi-Z → Phase 2 디스패치 파이프라인**

```cpp
[매 프레임 렌더 파이프라인]
Renderer::PopulateCommandList()
  ├─ 1. FrustumPass()               // Frustum 통과 커맨드를 Main/Vase 버퍼에 기록
  │
  ├─ 2. OcclusionPhase1Pass()
  │    └─ OcclusionCuller::DispatchPhase1()
  │         ├─ Final/Culled 카운터 버퍼 전부 0으로 리셋 (CopyBufferRegion)
  │         ├─ 전 자원 UAV 상태로 전환 후 Compute 파이프라인 바인딩
  │         │    ├─ b1 PhaseInfo = {0, hasPreviousHiz}  // Phase 1 명시
  │         │    ├─ t0: 이전 프레임 Hi-Z
  │         │    ├─ t1~t4: Frustum 통과 결과 (입력)
  │         │    └─ u0~u3: Final(생존), u4~u7: Culled(1차 탈락)
  │         ├─ Dispatch((total + 63) / 64, 1, 1)
  │         └─ Final 버퍼는 Indirect Argument로, Culled 버퍼는 SRV(NON_PIXEL_SHADER_RESOURCE)로 전환
  │
  ├─ 3. DepthPass()                 // Phase 1 Final 버퍼로 Depth 렌더링 → Hi-Z 재생성
  │
  ├─ 4. ShadowPass()
  │
  └─ 5. OcclusionPhase2Pass()
       └─ OcclusionCuller::DispatchPhase2()
            ├─ Final 버퍼를 다시 UAV로 전환 (Append 대상)
            ├─ b1 PhaseInfo = {1, 1}   // Phase 2 명시
            ├─ t0: 이번 프레임(방금 만든) Hi-Z
            ├─ t1~t4: Phase 1의 Culled 버퍼를 SRV로 입력
            ├─ u4~u7: 더미 UAV 바인딩 (Phase 2는 재탈락자를 기록하지 않으므로)
            ├─ Dispatch((total + 63) / 64, 1, 1)
            └─ Final 버퍼 전부 Indirect Argument로 전환 → 최종 Draw에 바로 사용
```

- [`Renderer.cpp`](https://github.com/BOLTB0X/Rieux-Renderer/blob/TwoPhase-OcclusionCulling/src/Core/Renderer.cpp)

---

#### 4) **Hi-Z 밉맵 체인 빌드 (Depth Pass 직후)**

```
Depth Texture
        ↓
Hi-Z Mip 0으로 복사
        ↓
Mip 0 → Mip 1 Downsample
        ↓
Mip 1 → Mip 2 Downsample
        ↓
...
        ↓
최종 Hi-Z Texture
```

Depth Pass가 끝나면 원본 뎁스 버퍼를 Hi-Z 텍스처의 `Mip 0` 으로 복사한 뒤, Compute Shader로 한 단계씩 다운샘플링하며 전체 밉 체인을 완성

```cpp
[HierarchicalZBuffer::Build]
    1. 원본 DepthTexture → HiZ Mip 0 로 CopyTextureRegion
    2. Mip 0는 SRV(NON_PIXEL_SHADER_RESOURCE), Mip 1~N-1은 UAV 상태로 일괄 전환
    3. for i in [1, mipLevels):
         inputWidth/Height  = max(1, width/height >> (i-1))
         outputWidth/Height = max(1, width/height >> i)
         ├─ HZBConstants(inputWidth, inputHeight) 상수 버퍼 세팅
         ├─ SRV: Mip(i-1) 바인딩, UAV: Mip(i) 바인딩
         ├─ Dispatch((outputWidth+7)/8, (outputHeight+7)/8, 1)
         └─ 다음 밉을 위해 방금 쓴 Mip(i)을 UAV → SRV로 전환
    4. 마지막 밉까지 SRV 상태로 정리
```

```cpp
// HierarchicalZCS.hlsl
// 2x2 텍셀을 로드해 Reverse-Z 기준 가장 먼 깊이(=min)를 다음 밉에 기록
float minDepth = min(min(d0, d1), min(d2, d3));

// 홀수 해상도일 때 마지막 열/행이 2x2 샘플링에서 누락되지 않도록 별도 보정
if ((InputResolution.x % 2 != 0) && (...)) { minDepth = min(minDepth, ...); }

g_OutputDepth[DTid.xy] = minDepth;
```

<div align="center">
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/GPU_Driven/t_HiZ-lv1.png?raw=true" width="220" style="border:1px solid #ddd; border-radius:4px;" />
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/GPU_Driven/t_HiZ-lv2.png?raw=true" width="220" style="border:1px solid #ddd; border-radius:4px;" />
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/GPU_Driven/t_HiZ-lv3.png?raw=true" width="220" style="border:1px solid #ddd; border-radius:4px;" />
  <br>
  <p><strong> HiZ Level 1 ~ 3 </strong></p>
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/GPU_Driven/t_HiZ-lv4.png?raw=true" width="220" style="border:1px solid #ddd; border-radius:4px;" />
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/GPU_Driven/t_HiZ-lv5.png?raw=true" width="220" style="border:1px solid #ddd; border-radius:4px;" />
  <br>
  <p><strong> HiZ Level 4 ~ 5 </strong></p>
</div>

- [`HierarchicalZBuffer.h`](https://github.com/BOLTB0X/Rieux-Renderer/blob/GPU-Driven/src/Graphics/Techniques/HierarchicalZBuffer.h) / [`.cpp`](https://github.com/BOLTB0X/Rieux-Renderer/blob/GPU-Driven/src/Graphics/Techniques/HierarchicalZBuffer.cpp)

- [`HierarchicalZCS.hlsl`](https://github.com/BOLTB0X/Rieux-Renderer/blob/GPU-Driven/src/Graphics/HLSL/HierarchicalZCS.hlsl)

Reverse-Z를 사용하므로 일반적인 Max가 아닌 **Min 연산**으로 다음 밉의 깊이를 결정합니다. 각 밉은 **"해당 영역에서 `가장 먼(=가장 작은 값)` 깊이"** 를 담게 되어, 

이후 Occlusion Culling에서 오브젝트의 AABB가 가릴 수 있는 최댓값 후보를 보수적으로 잡을 수 있게 함

홀수 해상도 처리를 별도 분기로 두어, 2의 거듭제곱이 아닌 화면 해상도에서도 마지막 열/행이 다운샘플링에서 누락되지 않도록 방어

---

#### 5) **Compute Shader 내부: AABB 투영과 Hi-Z 오클루전 판정**

```cpp
// Culling.hlsli - Is_Occluded
float2 sizeInPixels = (projAABB.uvMax - projAABB.uvMin) * screenSize;
float objectMipLevel = ceil(log2(max(max(sizeInPixels.x, sizeInPixels.y), 1.0f)));
float mipLevel = min(objectMipLevel, maxMipLevel);

// AABB의 화면 투영 영역 4개 모서리를 해당 Mip에서 샘플링
float maxOccluderDepth = min(min(d0, d1), min(d2, d3));

// 오브젝트의 가장 가까운 깊이가 가림막보다 더 뒤에 있으면 안 보임
return (projAABB.closestZ < maxOccluderDepth);
```

- **Conservative Mip 선택**: AABB가 화면에서 차지하는 픽셀 크기에 비례해 Hi-Z 밉레벨을 선택(`ceil(log2(...))`)

   - 오브젝트보다 한 단계 넉넉한 밉을 골라, 여러 텍셀을 한 번에 대표하는 값으로 과소평가된 오클루전(실제로 보이는데 가려졌다고 오판)을 방지

- **4-Corner Min Depth**: AABB 투영 영역의 네 모서리 깊이 중 **최솟값**(카메라에 가장 가까운 값)을 가림막 깊이로 채택해,

   - 오브젝트가 조금이라도 드러나 있다면 보이는 것으로 판정하는 보수적 전략을 썼습니다.

```cpp
// OcclusionCullingCS.hlsl - Phase 분기
if (isVisible)
{
    // Phase 1이든 Phase 2든 통과하면 무조건 Final에 Append
    // ...
}
else if (g_PhaseCB.isPhase2 == 0)
{
    // Phase 1에서 탈락했을 때만 Culled 버퍼에 보관 (Phase 2 재검사 대상)
    // Phase 2에서도 탈락하면 이 분기를 타지 않으므로 그대로 Drop
    // ...
}
```

<div align="center">
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/Two_Phase_Occlusion_Culling/Two_Phase_Occlusion_Culling02_masterScene06.png?raw=true" width="250" style="border:1px solid #ddd; border-radius:4px;" />
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/Two_Phase_Occlusion_Culling/Two_Phase_Occlusion_Culling01.png?raw=true" width="250" style="border:1px solid #ddd; border-radius:4px;" />
  <br>
  <p><strong> 메인/씬 카메라 구분 | AABB 박스 초록(보임), 노랑(백컬링/논컬링), 빨강(오클루전 컬링) </strong></p>
</div>


- 셰이더 하나(`OcclusionCullingCS.hlsl`)를 Phase 1/2 공용으로 재사용하고, `PhaseInfo` 상수 버퍼의 `isPhase2` 플래그로 입력 소스(Frustum 결과 vs Culled 결과)와 출력 대상(Final+Culled vs Final만)만 다르게 바인딩하는 방식으로 셰이더 중복 없이 두 단계를 구현

- [`OcclusionCuller.h`](https://github.com/BOLTB0X/Rieux-Renderer/blob/TwoPhase-OcclusionCulling/src/Graphics/Techniques/OcclusionCuller.h) / [`.cpp`](https://github.com/BOLTB0X/Rieux-Renderer/blob/TwoPhase-OcclusionCulling/src/Graphics/Techniques/OcclusionCuller.cpp)

- [`OcclusionCullingCS.hlsl`](https://github.com/BOLTB0X/Rieux-Renderer/blob/TwoPhase-OcclusionCulling/src/Graphics/HLSL/OcclusionCullingCS.hlsl) 

---
