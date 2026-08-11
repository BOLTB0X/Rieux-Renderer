// VertexBufferOnlyDebugPS.hlsl
struct PS_IN
{
    float4 positionCS : SV_POSITION;
    float3 positionWS : TEXCOORD0;
    float3 normalWS : TEXCOORD1;
    float3 tangentWS : TEXCOORD2;
    float3 binormalWS : TEXCOORD3;
    float2 texcoord : TEXCOORD4;
    nointerpolation uint albedoIndex : TEXCOORD5;
    nointerpolation uint normalIndex : TEXCOORD6;
    nointerpolation uint alphaIndex : TEXCOORD7;
}; // PS_IN

float4 main(PS_IN input) : SV_TARGET
{
    return float4(0.0f, 1.0f, 0.0f, 1.0f);
}