## GPU Driven - PSO/RootSignature 빌더 및 전역 인덱스 관리

Bindless·GPU-Driven 파이프라인은 파이프라인 종류(Frustum Culling, Occlusion Culling, Hi-Z, Sponza 렌더링 등)마다 서로 다른 루트 시그니처 레이아웃과 PSO가 필요

매번 `D3D12_ROOT_PARAMETER` 배열을 손으로 채우고 인덱스를 매직 넘버로 하드코딩하면 레이아웃이 하나만 바뀌어도 여러 파일의 숫자를 전부 맞춰줘야 하는 문제가 생김

이를 막기 위해 **태그(문자열) 기반 Root Signature 빌더**로 파라미터를 등록하고, **PSOManager가 루트 시그니처·PSO·셰이더 blob을 문자열 키로 중앙 관리**하며, 실제 드로우 코드는 인덱스 대신 `RendererState`의 정적 필드(`RendererState::CullingFrustumPlanesIndex` 등)를 참조하는 구조를 채택

---

#### 1) **아키텍처 & 핵심 로직**

1. **D3D12RootSignature::Builder — 태그 기반 파라미터 등록**

   - `AddCBV`/`AddConstants`/`AddSRV`/`AddSRVTable`/`AddUAVTable`/`AddStaticSampler` 등 체이닝 가능한 빌더 메서드로 루트 파라미터를 하나씩 추가

   - 각 호출마다 `m_paramIndexByTag[tag] = 현재 루트 파라미터 개수`로 태그와 인덱스를 매핑해두고, `Init()` 시점에 이 맵을 `D3D12RootSignature::m_paramIndexMap`으로 그대로 넘김

2. **PSOManager — 문자열 키로 루트 시그니처/PSO/셰이더 중앙 관리**

   - `BuildDefaultSponza`, `BuildGPUDriven`, `BuildFrustumCullingCompute`, `BuildOcclusionCullingCompute`, `BuildHierarchicalZ`, `BuildSolidCullBack/CullNone`, `BuildWireframeCullBack/CullNone`, `BuildDebugAABB` 등 파이프라인 종류별 `Build*` 메서드가 각각 `CreateRootSignature`로 필요한 태그만 등록하고 PSO를 컴파일

   - 완성된 결과는 `m_rootSignatureMap`/`m_psoMap`/`m_shaderBlobs`에 문자열 키로 저장되어, 이후 `GetPSO(name)`/`GetD3D12RootSignature(name)`/`GetID3D12RootSignature(name)`로 어디서든 조회

3. **RendererState — 매직 넘버 대신 정적 인덱스로 노출**

   - 루트 파라미터 태그명과 동일한 이름의 정적 필드(`CullingFrustumPlanesIndex`, `BindlessTexIndex`, `InstanceDataIndex` 등)를 `RendererState`에 선언

   - 초기화 시점에 `PSOManager::GetD3D12RootSignature(name)->GetParamIndex(tag)`로 실제 인덱스를 조회해 이 정적 필드에 한 번만 캐싱하고, 이후 모든 `SetGraphicsRootDescriptorTable`/`SetComputeRootDescriptorTable` 호출은 이 필드를 참조

---

#### 2) **Root Signature Builder: 태그로 파라미터 위치 추상화**

```cpp
// D3D12RootSignature::Builder::AddCBV
D3D12RootSignature::Builder& D3D12RootSignature::Builder::AddCBV(const std::string& tag,
    UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility) {
    m_paramIndexByTag[tag] = static_cast<UINT>(m_rootParameters.size());  // 태그 -> 인덱스 매핑 선기록

    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.ShaderVisibility = visibility;

    m_rootParameters.push_back(param);
    return *this;  // 체이닝
} // AddCBV
```

```cpp
// D3D12RootSignature::Init
bool D3D12RootSignature::Init(const InitParams& params) {
    // ... D3D12SerializeRootSignature / CreateRootSignature ...

    m_paramIndexMap = b.m_paramIndexByTag;  // Builder가 쌓아온 태그 맵을 그대로 승계
    return true;
} // Init

UINT D3D12RootSignature::GetParamIndex(const std::string& tag) const {
    auto it = m_paramIndexMap.find(tag);
    if (it == m_paramIndexMap.end()) {
        DebugHelper::DebugPrint("루트 파라미터 태그를 찾을 수 없음: " + tag);
        return UINT_MAX;
    }
    return it->second;
} // GetParamIndex
```

- [`D3D12RootSignature.h`](https://github.com/BOLTB0X/Rieux-Renderer/blob/GPU-Driven/src/Core/D3D12/D3D12RootSignature.h) / [`.cpp`](https://github.com/BOLTB0X/Rieux-Renderer/blob/GPU-Driven/src/Core/D3D12/D3D12RootSignature.cpp)

`D3D12_ROOT_PARAMETER` 배열에서 파라미터의 물리적 위치(몇 번째 인덱스인지)는 루트 시그니처를 어떻게 구성했느냐에 따라 계속 바뀔 수 있음

이 위치를 코드 곳곳에 숫자로 박아두는 대신, `Add*` 호출 시점에 함께 넘긴 태그 문자열로 위치를 되찾아오도록 해서 **루트 시그니처 레이아웃이 바뀌어도 호출부 코드는 그대로 유지**

---

#### 3) **`PSOManager`: 파이프라인별 Build 메서드와 조회 인터페이스**

```cpp
[PSOManager 책임 분리]
PSOManager
 ├─ CreateRootSignature(name, builderConfigLambda)
 │     └─ Builder를 구성하는 람다를 받아 D3D12RootSignature::Init 호출 후 m_rootSignatureMap[name]에 저장
 │
 ├─ [파이프라인별 Build 메서드]
 │     ├─ BuildDefaultSponza()              // 기본 PBR 렌더링 루트 시그니처/PSO
 │     ├─ BuildGPUDriven()                  // Vertex Pulling + Bindless 루트 시그니처/PSO
 │     ├─ BuildFrustumCullingCompute()      // Frustum Culling Compute 전용
 │     ├─ BuildOcclusionCullingCompute()    // Two Phase Occlusion Culling Compute 전용
 │     ├─ BuildHierarchicalZ()              // Hi-Z 다운샘플 Compute 전용
 │     ├─ BuildDepthRecord() / BuildShadowRecord()
 │     ├─ BuildSolidCullBack() / BuildSolidCullNone()     // Main/Vase 그룹용 PSO 쌍
 │     ├─ BuildWireframeCullBack() / BuildWireframeCullNone()
 │     └─ BuildDebugAABB() / BuildDebugLine() / BuildDebugTransReverseZ()
 │
 └─ [조회 인터페이스]
       ├─ GetPSO(name) -> D3D12PipelineState*
       ├─ GetD3D12RootSignature(name) -> D3D12RootSignature*   // GetParamIndex(tag) 호출용
       └─ GetID3D12RootSignature(name) -> ID3D12RootSignature*  // SetGraphicsRootSignature 바인딩용
```

- [`PSOManager.h`](https://github.com/BOLTB0X/Rieux-Renderer/blob/GPU-Driven/src/Core/Managers/PSOManager.h) / [`.cpp`](https://github.com/BOLTB0X/Rieux-Renderer/blob/GPU-Driven/src/Core/Managers/PSOManager.cpp)

`Sponza`가 Main 그룹은 `psoSolidCull`, Vase 그룹은 `psoSolidNoCull`로 렌더링(3.2 참고)하는 것처럼, 컬링 여부만 다르고 나머지 상태는 동일한 PSO 쌍이 여러 곳에서 필요

`BuildSolidCullBack`/`BuildSolidCullNone`처럼 Build 메서드 단위로 이 쌍을 명확히 나눠, 각 파이프라인이 어떤 PSO 조합을 쓰는지 이름만으로 추적할 수 있게 했음

---

#### 4) **DescriptorHeapAllocator: 프리 리스트 기반 인덱스 할당**

```cpp
UINT DescriptorHeapAllocator::Allocate() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_freeList.empty()) {          // 반납된 인덱스가 있으면 우선 재사용
        UINT index = m_freeList.back();
        m_freeList.pop_back();
        return index;
    }

    if (m_nextFreeIndex >= m_capacity) {
        DebugHelper::DebugPrint("DescriptorHeapAllocator 용량 초과");
        return UINT_MAX;
    }

    return m_nextFreeIndex++;           // 없으면 새 인덱스 발급
} // Allocate

void DescriptorHeapAllocator::Free(UINT index) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_freeList.push_back(index);        // 즉시 재사용 목록에 반환
} // Free
```

- [`DescriptorHeapAllocator.h`](https://github.com/BOLTB0X/Rieux-Renderer/blob/GPU-Driven/src/Graphics/Tool/DescriptorHeapAllocator.h) / [`.cpp`](https://github.com/BOLTB0X/Rieux-Renderer/blob/GPU-Driven/src/Graphics/Tool/DescriptorHeapAllocator.cpp)

Sponza의 `m_instanceDataDescriptorIndex`, 각 텍스처의 `GetSRVIndex()`, Occlusion Culling의 `m_mainVisibleDescIndex` 등 3.1~3.3에서 사용된 모든 Bindless 인덱스가 이 Allocator를 거쳐 발급

`std::mutex`로 보호되어 있어 향후 멀티스레드 리소스 로딩으로 확장해도 인덱스 충돌 없이 동시 할당이 가능

---

#### 5) **RendererState: 태그 → 인덱스를 정적 필드로 캐싱**

```cpp
class RendererState {
public:
    static UINT FrameCBIndex;
    static UINT LightCBIndex;
    static UINT InstanceDataIndex;
    static UINT BindlessTexIndex;
    static UINT BindlessBufIndex;
    static UINT CullingFrustumPlanesIndex;
    static UINT CullingMasterInstanceIndex;
    static UINT CullingVisibleMainCommandsIndex;
    static UINT CullingVisibleVaseCommandsIndex;
    // ...
}; // RendererState
```

```cpp
// 이후 FrustumCuller::Dispatch(), Sponza::SubmitIndirect() 등 모든 드로우 코드는
cmdList->SetComputeRootDescriptorTable(RendererState::CullingFrustumPlanesIndex, ...) // 형태로 사용
```

---

<br/>
