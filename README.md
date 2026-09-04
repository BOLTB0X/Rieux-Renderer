## Frustum Culling & Backface/No-Cull 분리

<div align="center">
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/GPU_Driven/02_GPU%ED%94%84%EB%9F%AC%EC%8A%A4%ED%85%8000.png?raw=true" width="250" style="border:1px solid #ddd; border-radius:4px;" />
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/GPU_Driven/t_%EB%B0%94%EC%9A%B4%EB%94%A9%EB%B0%95%EC%8A%A4.png?raw=true" width="250" style="border:1px solid #ddd; border-radius:4px;" />
  <br>
  <p><strong> Frustum Culling | AABB 박스 </strong></p>
</div>

GPU 상에서 인스턴스 단위로 AABB와 카메라 프러스텀의 교차를 판정해 화면 밖 오브젝트를 1차로 걸러냄

동시에 통과한 인스턴스를 **Main(불투명, 백페이스 컬링 적용)** 과 **Vase(나뭇잎·화분 등 양면 렌더링이 필요한 오브젝트, 컬링 미적용)** 두 그룹으로 분리해, 이후 렌더 단계에서 서로 다른 래스터라이저 PSO로 그림

---

#### 1) **아키텍처 & 핵심 로직**

1. **CPU 측 프러스텀 평면 갱신**

   - `FrustumCuller::Frame` → `Frustum::Frame(viewMatrix, projectionMatrix)`에서 카메라의 6개 평면(Near/Far/Left/Right/Top/Bottom)을 매 프레임 계산해 GPU 상수 버퍼로 업로드

2. **GPU AABB-Frustum 교차 판정**

   - `FrustumCullingCS.hlsl`: 전체 인스턴스에 대해 `Check_Box_Visible`로 AABB의 8개 꼭짓점과 6개 평면을 대조, 하나라도 평면 안쪽이면 통과

   - 통과한 인스턴스는 `isVase` 값에 따라 `g_VisibleMainCommands` / `g_VisibleVaseCommands` 중 하나에 `InterlockedAdd` 기반으로 Append

3. **PSO 분기 렌더링**

   - `Sponza::SubmitIndirect`: Main 그룹은 `m_psoSolidCull`(백페이스 컬링 적용)로, Vase 그룹은 `m_psoSolidNoCull`(양면 렌더링)로 각각 독립적인 `ExecuteIndirect` 호출

---

#### 2) **AABB-Frustum 교차 판정 및 그룹 분리**

```cpp
// FrustumCullingCS.hlsl
[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint meshIndex = dispatchThreadID.x;
    if (meshIndex >= g_FrustumCullingCB.totalInstances) return;

    MeshInstanceData instance = g_MasterInstanceData[meshIndex];

    bool visible = Check_Box_Visible(instance.aabbMin, instance.aabbMax,
                                      instance.worldMatrix, g_FrustumCullingCB.frustumPlanes);
    if (!visible) return;

    // Main / Vase 그룹으로 분리 Append
    if (instance.isVase > 0.5f)
    {
        uint idx;
        g_VaseCount.InterlockedAdd(0, 1, idx);
        g_VisibleVaseCommands[idx] = g_MasterCommands[meshIndex];
    }
    else
    {
        uint idx;
        g_MainCount.InterlockedAdd(0, 1, idx);
        g_VisibleMainCommands[idx] = g_MasterCommands[meshIndex];
    }
} // main
```

```cpp
// Culling.hlsli - Check_Box_Visible
// AABB의 8개 꼭짓점을 월드 공간으로 변환한 뒤
// 6개 평면 각각에 대해 "8개 꼭짓점 중 하나라도 평면 안쪽(dot(normal, corner) + d >= 0)"이면 통과
// 한 평면이라도 8개 전부 바깥이면 완전히 절두체 밖으로 판단해 컬링
```

- [`FrustumCullingCS.hlsl`](https://github.com/BOLTB0X/Rieux-Renderer/blob/GPU-Driven/src/Graphics/HLSL/FrustumCullingCS.hlsl)

- [`Culling.hlsli`](https://github.com/BOLTB0X/Rieux-Renderer/blob/GPU-Driven/src/Graphics/HLSL/Culling.hlsli)

---

#### 3) **Main / Vase 그룹별 PSO 바인딩**

```cpp
// Sponza::SubmitIndirect
if (params.mainVisibleCommandsBuffer && m_mainIndirectCount > 0)
{
    cmdList->SetPipelineState(m_enableWireframe ? m_psoWireCull : m_psoSolidCull);   // 백페이스 컬링 ON
    cmdList->ExecuteIndirect(m_commandSignature.Get(), m_mainIndirectCount,
                              params.mainVisibleCommandsBuffer, 0, params.mainCounterBuffer, 0);
}

if (params.vaseVisibleCommandsBuffer && m_vaseIndirectCount > 0)
{
    cmdList->SetPipelineState(m_enableWireframe ? m_psoWireNoCull : m_psoSolidNoCull); // 컬링 OFF
    cmdList->ExecuteIndirect(m_commandSignature.Get(), m_vaseIndirectCount,
                              params.vaseVisibleCommandsBuffer, 0, params.vaseCounterBuffer, 0);
}
```

<div align="center">
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/Model/03Load%EB%94%94%ED%8F%B4%ED%8A%B8_%EB%B6%80%EB%B6%84%EC%BB%AC%EB%A7%81.png?raw=true" width="280" style="border:1px solid #ddd; border-radius:4px;" />
  <br>
  <p><strong> 부분 컬링 </strong></p>
</div>

- [`FrustumCuller.h`](https://github.com/BOLTB0X/Rieux-Renderer/blob/GPU-Driven/src/Graphics/Techniques/FrustumCuller.h) / [`.cpp`](https://github.com/BOLTB0X/Rieux-Renderer/blob/GPU-Driven/src/Graphics/Techniques/FrustumCuller.cpp)

- [`Sponza.cpp`](https://github.com/BOLTB0X/Rieux-Renderer/blob/GPU-Driven/src/World/Sponza.cpp)


컬링 셰이더 단계에서 이미 Main/Vase로 그룹이 나뉘어 있기 때문에,

렌더 단계는 PSO만 바꿔가며 `ExecuteIndirect`를 두 번 호출하는 것으로 끝

오브젝트 단위로 매번 컬링 모드를 스위칭하는 대신, 그룹 단위 배치 처리로 파이프라인 상태 전환 횟수를 최소화
