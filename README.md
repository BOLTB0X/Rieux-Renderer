## Vertex Pulling, Bindless & Instanced Sponza Loading

<div align="center">
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/GPU_Driven/03_GPU_%EC%9D%B8%EC%8A%A4%ED%84%B4%EC%8B%B101.png?raw=true" width="450" style="border:1px solid #ddd; border-radius:4px;" />
  <br>
  <p><strong> 인스턴싱 </strong></p>
</div>


이 프로젝트는 버텍스 버퍼를 IA에 바인딩하지 않고, **버텍스 셰이더가 `SV_VertexID`만으로 비바운드(Unbounded) SRV 배열에서 직접 정점을 가져오는 Vertex Pulling** 과, 

텍스처/ 버퍼를 인덱스 하나로 참조하는 **Bindless Descriptor** 구조를 기반으로 씬을 로드

---

#### 1) **아키텍처 & 핵심 로직**

1. **Instance Data Buffer 구축**

   - `Sponza::BuildInstanceDataBuffer`에서 그리드 배치(`gridSize`)만큼 서브메시를 순회하며, 각 인스턴스의 `MeshInstanceData`(월드 행렬, `vertexBufferIndex`, AABB, 알베도/노말/알파 등 머티리얼의 Bindless 인덱스)를 구조화 버퍼로 업로드

   - 메시 이름에 `vase`/`leaf` 등이 포함되면 `isVase` 플래그를 세워 이후 컬링·렌더 단계에서 별도 그룹으로 분리 처리

2. **Indirect Command Buffer 구축**

   - `Sponza::BuildIndirectBuffers`에서 `IndexBufferView` + `RootConstant(InstanceIndex)` + `DrawIndexed` 3개 인자로 구성된 `CommandSignature`를 정의

   - 전체 인스턴스에 대한 Master Indirect 커맨드를 만들고, `isVase` 여부에 따라 Main/Vase 두 그룹으로 나눠 이후 Frustum/Occlusion Culling의 입력으로 사용

3. **Vertex Shader에서의 Pulling**

   - `GPU_PBRModelVS.hlsl`: `SV_VertexID`로 인스턴스 데이터(`g_InstanceCB.InstanceIndex`)를 조회하고, 해당 인스턴스의 `vertexBufferIndex`로 비바운드 `StructuredBuffer<PBRVertex> g_VertexBuffers[]`에서 정점을 직접 fetch

---

#### 2) **인스턴스 데이터 및 Bindless 인덱스 채우기**

```cpp
[Init 단계]
    Sponza::BuildInstanceDataBuffer(device, gridSize, spacing, isColumn)
        └─ for z, x in gridSize × gridSize:
              for mesh in m_meshes:
                  MeshInstanceData data
                      ├─ worldMatrix = transpose(grid offset × 회전 × 부모 Transform)
                      ├─ vertexBufferIndex = mesh->GetVertexBufferSRVIndex()   // Vertex Pulling용
                      ├─ aabbMin / aabbMax
                      ├─ albedoIndex / normalIndex / alphaIndex / metallicIndex / roughnessIndex
                      │     = material.*->GetSRVIndex()                        // Bindless 텍스처 인덱스
                      └─ isVase = (meshName에 "vase"/"leaf"/특정 머티리얼 포함 시 1.0f)
              instanceDataArray에 누적
        └─ Upload Heap에 업로드 후 StructuredBuffer SRV 생성 → m_instanceDataDescriptorIndex 발급
```

```cpp
[Init 단계]
    Sponza::BuildIndirectBuffers(device, rootSignature, gridSize)
        ├─ D3D12_INDIRECT_ARGUMENT_DESC[3] = { IndexBufferView, RootConstant(InstanceIndex), DrawIndexed }
        ├─ CreateCommandSignature(csDesc, rootSignature) → m_commandSignature
        └─ for 인스턴스 순회:
              isVase 여부에 따라 mainCmds / vaseCmds 벡터에 IndirectCommand 분류 저장
```

- [`Sponza.h`](https://github.com/BOLTB0X/Rieux-Renderer/blob/ExecuteIndirect/src/Graphics/World/Sponza.h) / [`.cpp`](https://github.com/BOLTB0X/Rieux-Renderer/blob/ExecuteIndirect/src/Graphics/World/Sponza.cpp)

---

#### 3) **매 드로우 파이프라인: Vertex Pulling 셰이더**

```cpp
// GPU_PBRModelVS.hlsl
StructuredBuffer<PBRVertex>        g_VertexBuffers[] : register(t0, space2);  // Bindless 정점 배열
StructuredBuffer<MeshInstanceData> g_InstanceData    : register(t1, space0);
ConstantBuffer<InstanceCB>         g_InstanceCB      : register(b2);          // Indirect RootConstant로 전달된 InstanceIndex

VS_OUT main(uint vertexID : SV_VertexID)
{
    MeshInstanceData inst = g_InstanceData[g_InstanceCB.InstanceIndex];
    PBRVertex input = g_VertexBuffers[inst.vertexBufferIndex][vertexID];  // 버텍스 직접 Fetch

    float4 worldPos = mul(float4(input.position, 1.0f), inst.worldMatrix);
    // ...
}
```

- [`GPU_PBRModelVS.hlsl`](https://github.com/BOLTB0X/Rieux-Renderer/blob/GPU-Driven/src/Graphics/HLSL/GPU_PBRModelVS.hlsl)

- [`GPU_SponzaPS.hlsl`](https://github.com/BOLTB0X/Rieux-Renderer/blob/TwoPhase-OcclusionCulling/src/Graphics/HLSL/GPU_SponzaPS.hlsl)


`ExecuteIndirect`가 매 드로우마다 `RootConstant`로 넘기는 `InstanceIndex` 하나만으로 정점·머티리얼·트랜스폼을 전부 조회할 수 있어,

인스턴스 수가 늘어나도 CPU 측 바인딩 명령이 증가 X

전통적인 방식이라면 오브젝트 종류만큼 필요했을 IA 상태 전환이 여기서는 아예 발생 X

---

<br/>