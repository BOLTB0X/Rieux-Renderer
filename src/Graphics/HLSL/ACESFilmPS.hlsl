// ACESFilmPS.hlsl
Texture2D HDRTexture : register(t0);
SamplerState Sampler : register(s0);

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// ACES Film Tone Mapping Curve
float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
} // ACESFilm

float4 main(VS_OUT input) : SV_Target
{
    float3 hdrColor = HDRTexture.Sample(Sampler, input.uv).rgb;
    float3 ldrColor = ACESFilm(hdrColor);

    ldrColor = pow(ldrColor, 1.0f / 2.2f);

    return float4(ldrColor, 1.0f);
} // main