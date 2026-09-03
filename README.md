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
  <td><img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/GPU_Driven/t_%EB%B0%94%EC%9A%B4%EB%94%A9%EB%B0%95%EC%8A%A4.png?raw=true" width="320"></td>
  <td><img src= "https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/Two_Phase_Occlusion_Culling/Two_Phase_Occlusion_Culling01.png?raw=true" width="320"></td>
  <td><img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/Two_Phase_Occlusion_Culling/Two_Phase_Occlusion_Culling02_masterScene04.png?raw=true" width="320"></td>

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
