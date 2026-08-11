// DepthPS.hlsl
Texture2D    g_Textures[] : register(t0, space1);
SamplerState g_Sampler : register(s0);

struct PS_IN
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    uint   alphaIdx : BLENDINDICES0;
}; // PS_IN

void main(PS_IN input)
{
    if (input.alphaIdx == 0xFFFFFFFF)
        return;

    float alpha = g_Textures[input.alphaIdx].Sample(g_Sampler, input.uv).a;

    clip(alpha - 0.1f);
} // main