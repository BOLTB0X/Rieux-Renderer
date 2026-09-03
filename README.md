## Image Based Lighting

<div align="center">
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/IBL/IBL05_Irradiance%20Map0.png?raw=true" width="220" style="border:1px solid #ddd; border-radius:4px;" />
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/IBL/IBL03BRDF.png?raw=true" width="220" style="border:1px solid #ddd; border-radius:4px;" />
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/IBL/IBL02_Filter_Face0-Filter00.png?raw=true" width="220" style="border:1px solid #ddd; border-radius:4px;" />
  <br>
  <p><strong> Diffuse irradiance | IBL03BRDF |  Specular IBL  </strong></p>
</div>

> 단일 색상의 Ambient Light나 단순한 환경 맵핑만으로는 PBR(Physically Based Rendering) 환경에서 금속과 비금속 표면의 사실적인 반사를 표현할 수 없음 

IBL은 씬의 전방위 환경 정보를 담은 큐브맵(Cubemap)을 광원으로 취급하여, 모든 방향에서 들어오는 **빛의 난반사(Diffuse)와 정반사(Specular)를 물리 기반 수학 모델에 따라 계산.**

연산량이 매우 방대하기 때문에 Epic Games의 **Split-Sum Approximation(분리 합산 근사)** 방식을 도입하여 실시간 렌더링에 적합하도록 파이프라인을 구축

```text
Scene Capture (Environment Map)
        ↓
[Pre-computation / Dirty Update]
 ├─ BRDF Integration Map ──→ 2D LUT (초기 1회 생성)
 ├─ Diffuse Irradiance ────→ 반구(Hemisphere) 영역의 빛을 미리 적분 (Cubemap)
 └─ Specular Prefiltered ──→ 표면 거칠기(Roughness)에 따른 반사광을 Mipmap에 분리 저장 (Cubemap)
        ↓
Lighting Pass (Deferred / Forward)
        └─ 3개의 맵을 샘플링하여 최종 주변광(Ambient) 도출
```

`Environment Probe` 캡처와 IBL 연산에 `Dirty Caching` 구조를 적용하여, 매 프레임 발생하는 무거운 적분 연산을 차단하고 GPU 프레임 타임을 최적화

---

#### 1) **아키텍처 & 핵심 로직**

<div align="center">
  <img src="https://github.com/BOLTB0X/DirectX-Playground/blob/main/DemoGIF/DX12-Renderer/SSR/t_%ED%94%84%EB%A1%9C%EB%B8%8C%EB%B2%94%EC%9C%84.png?raw=true" width="250" style="border:1px solid #ddd; border-radius:4px;" />
  <br/>
  <p><strong> 환경 맵 범위 </strong></p>
</div>

- **IBL 텍스처 버퍼 구조**

    | Buffer | Format | Type | 목적 및 특징 |
    | :--- | :--- | :--- | :--- |
    | **BRDF LUT** | `R16G16_FLOAT` | Texture2D | (N·V)와 Roughness를 기반으로 Fresnel(F)과 기하 감쇠(G)를 사전 계산한 2D 룩업 테이블 |
    | **Irradiance Map** | `R16G16B16A16_FLOAT` | TextureCube | 저해상도(예: 32x32). Cosine-weighted 반구 샘플링을 통해 Diffuse IBL 연산에 사용 |
    | **Prefiltered Map** | `R16G16B16A16_FLOAT` | TextureCube | 원본 환경 맵 해상도. Mipmap 레벨이 올라갈수록 Roughness가 높은 표면의 흐릿한 반사를 저장 |

- **Dirty Caching** 

    - 동적 환경 맵 갱신 시, 6개의 큐브맵 면과 모든 밉맵 레벨을 매 프레임 다시 계산하면 막대한 GPU 오버헤드(수십 ms)가 발생
    
    - 카메라가 프로브의 일정 반경을 벗어나거나 씬의 주요 광원이 변경될 때만 IsDirty 플래그를 활성화하여 제한적으로 IBL 텍스처를 재구성(Update)

- **Compute Shader 기반 생성**

    - 텍스처의 밉(Mip)과 슬라이스(Face) 단위 접근이 용이한 Compute Shader(`UAV`)를 사용하여 `Irradiance` 및 `Prefilter` 필터링 패스를 고속으로 병렬 처리.

---

#### 2) **BRDF Integration & Prefiltering (Compute Shader)**

```cpp
// GPU_BRDFIntegrationCS.hlsl
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float2 uv = float2(DTid.xy) / float2(g_Resolution.xy);
    float NdotV = max(uv.x, 0.001f);
    float roughness = uv.y;

    float3 V = float3(sqrt(1.0f - NdotV * NdotV), 0.0f, NdotV);
    float A = 0.0f;
    float B = 0.0f; 

    // Hammersley 시퀀스와 Importance Sampling을 이용한 적분 근사
    for (uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H  = ImportanceSampleGGX(Xi, N, roughness);
        float3 L  = normalize(2.0f * dot(V, H) * H - V);
        
        // ... (G_Smith 연산 및 가중치 합산 생략)
    }

    g_BRDF_UAV[DTid.xy] = float2(A / float(SAMPLE_COUNT), B / float(SAMPLE_COUNT));
}
```

**Roughness에 따라 선명한 거울 반사부터 희미하게 흩어지는 무광 금속의 질감까지, 추가적인 광원 비용 없이 텍스처 샘플링(Mip 레벨 지정)과 수식만으로 물리적으로 올바른 PBR 환경광을 완성**

---