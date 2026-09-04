# DX12 - Rieux Renderer

<div align="center">
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/SSR/03.png?raw=true" width="500" style="border:1px solid #ddd; border-radius:4px;" />
  <br>
  <p><strong>✊😐 DirectX 12 GPU Driven Rendering For Sponza </strong></p>
</div>

<table>
  <tr>
  <td><img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/Deferred/PBR02.png?raw=true" width="320"></td>
  <td><img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/Deferred/PBR03.png?raw=true" width="320"></td>
  <td><img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/Deferred/PBR05.png?raw=true" width="320"></td>
  </tr>
  <tr>
  <td><img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/RieuxRenderer01.png?raw=true" width="320"></td>
  <td><img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/Deferred/PBR06.png?raw=true" width="320"></td>
  <td><img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/Deferred/PBR01.png?raw=true" width="320"></td>
  </tr>
  <tr>
  <td><img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/GPU_Driven/03_GPU_%EC%9D%B8%EC%8A%A4%ED%84%B4%EC%8B%B101.png?raw=true" width="320"></td>
  <td><img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/Model/04Load%EB%94%94%ED%8F%B4%ED%8A%B8_%EC%99%80%EC%9D%B4%EC%96%B4%ED%94%84%EB%A0%88%EC%9E%84.png?raw=true" width="320"></td>
  <td><img src= "https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/GPU_Driven/06_Occlusion%20Culling03.png?raw=true" width="320"></td>
  </tr>
  <tr>
  <td><img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/GPU_Driven/t_%EB%B0%94%EC%9A%B4%EB%94%A9%EB%B0%95%EC%8A%A4.png?raw=true" width="320"></td>
  <td><img src= "https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/Two_Phase_Occlusion_Culling/Two_Phase_Occlusion_Culling01.png?raw=true" width="320"></td>
  <td><img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/Two_Phase_Occlusion_Culling/Two_Phase_Occlusion_Culling02_masterScene04.png?raw=true" width="320"></td>
  </tr>

</table>

---

## Development Environment

- **IDE** : Visual Studio Community 2022
- **Lang/Graphics API** : C++ 17/ HLSL 6.0 / DirectX 12
- **Library :**  `DirectXTK`, `DirectXTex`**,** `spdlog`, `Assimp`**,** `ImGui`
- **Build** : `CMake 3.21`
- **Package Manager** : `vcpkg`
- **CPU/GPU:** AMD Ryzen 5 3500u Vega Mobile GFX / AMD Radeon Vega 8
- **OS:**: Windows 11
- **Test Tool**: PIX

---

## **Features**

- **Assimp** Loader
- **GPU Driven Rendering**
    - *Vertex Fetch*
    - *Bindless Texture*
    - *ExecuteIndirect*
- **Cascaded Shadow Map**
- **Deferred Rendering Pipeline**
    - *IBL*
    - *FBR Shading*
- **Two-Phase Occlusion Culling**
    - *Frustum*
    - *Hierarchical Z-Buffer*
- **SSR**
- **ACESFilm Tone Mapping**

---

## Branches

> 💡**각 Branch README 에서 상세 설명을 확인하실 수 있습니다.**

| Rendering Feature | Key Optimizations & Logic | Branch Link |
| :--- | :--- | :--- |
| **Vertex Pulling, Bindless & Instanced Sponza Loading** | GPU-Driven 파이프라인의 기반. Descriptor 병목을 해소하고 Vertex Pulling을 도입해 CPU 개입 없이 대규모 인스턴스 렌더링 환경 구축 | [Bindless-Texture](https://github.com/BOLTB0X/Rieux-Renderer/tree/Bindless-Texture) |
| **PSO/RootSignature 빌더 및 전역 인덱스 관리** | Builder 패턴을 활용한 파이프라인 상태 객체(PSO) 및 서명 최적화 설계. 전역 인덱스 관리를 통한 리소스 바인딩 효율화 | [GPU-Driven](https://github.com/BOLTB0X/Rieux-Renderer/tree/GPU-Driven) |
| **GPU Frustum Culling** | 프러스텀 교차 판정을 Compute Shader로 오프로딩(Offloading)하여 CPU 병목 제거 및 `ExecuteIndirect`를 위한 드로우 콜 인자 직접 구성 | [GPU-Frustum](https://github.com/BOLTB0X/Rieux-Renderer/tree/GPU-Frustum)  |
| **Two Phase Occlusion Culling** | 이전 프레임(Phase 1)과 현재 프레임(Phase 2)의 Hi-Z 버퍼를 교차 검증하여 가시성 정확도를 높이고 픽셀 오버드로우(Overdraw) 극적 감소| [TwoPhase-OcclusionCullings](https://github.com/BOLTB0X/Rieux-Renderer/tree/TwoPhase-OcclusionCullings)|
| **IBL** | 주변 환경 광원을 PBR 셰이딩에 적용하기 위한 Irradiance, Prefilter, BRDF LUT 사전 연산(Pre-computation) 및 적용 | [Image-Based-Lighting](https://github.com/BOLTB0X/Rieux-Renderer/tree/Image-Based-Lighting) |
| **Deferred Rendering(PBR)** |G-Buffer를 활용해 기하학적 복잡도와 라이팅 연산을 분리하고, IBL을 통합하여 다중 광원 환경에서의 렌더링 성능 확보 | [Deferred-Light](https://github.com/BOLTB0X/Rieux-Renderer/tree/Deferred-Light) |
| **SSR** |G-Buffer의 Depth/Normal을 활용한 화면 공간 Raymarching을 수행하여 동적이고 사실적인 반사 효과 구현| [SSR](https://github.com/BOLTB0X/Rieux-Renderer/tree/SSR)|
| **Cascaded Shadow Maps** |  시야 거리에 따라 Frustum을 분할(Cascade)하고 해상도를 차등 할당하여 넒은 씬에서도 메모리 효율적인 그림자 품질 유지 | [CSM](https://github.com/BOLTB0X/Rieux-Renderer/tree/CSM)|

---

## Ref

- [모델 출처 - Crytek Sponza](https://github.com/jimmiebergmann/Sponza)

- [Learn Microsoft - Direct3D 12 Graphics](https://learn.microsoft.com/en-us/windows/win32/api/_direct3d12/)

- [유니티 공식문서 - OcclusionCulling](https://docs.unity3d.com/kr/530/Manual/OcclusionCulling.html)

- [언리얼 공식문서 - 비저빌리티 및 오클루전 컬링](https://dev.epicgames.com/documentation/unreal-engine/visibility-and-occlusion-culling-in-unreal-engine?lang=ko)

- [Developer Nvidia - Chapter.29 Efficient Occlusion Culling](https://developer.nvidia.com/gpugems/gpugems/part-v-performance-and-practicalities/chapter-29-efficient-occlusion-culling)

- [Medium(@mil_kru)- Two-Pass Occlusion Culling](https://medium.com/@mil_kru/two-pass-occlusion-culling-4100edcad501)

- [Medium(@Lucmomber) - Two-Pass Hierarchical Z-Buffer Occlusion Culling](https://medium.com/@Lucmomber/two-pass-hierarchical-z-buffer-occlusion-culling-93171c5a9808)

- [Interplay of Light - Experiments in GPU-based occlusion culling](https://interplayoflight.wordpress.com/2017/11/15/experiments-in-gpu-based-occlusion-culling/)

- [Real Shading in Unreal Engine 4 Presentations Notes](https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf)

- [Learn OpenGL - HDR](https://learnopengl.com/Advanced-Lighting/HDR)

- [Learn OpenGL - Deferred Shading](https://learnopengl.com/Advanced-Lighting/Deferred-Shading)

- [Learn OpenGL - Lighting](https://learnopengl.com/PBR/Lighting)

- [Learn OpenGL - Diffuse irradiance](https://learnopengl.com/PBR/IBL/Diffuse-irradiance)

- [Learn OpenGL - Specular IBL](https://learnopengl.com/PBR/IBL/Specular-IBL)
