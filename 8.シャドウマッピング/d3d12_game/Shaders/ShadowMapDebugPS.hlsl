#include "ShaderCommon.hlsli"

// テクスチャ
Texture2D<float4> ShadowMap : register(t0);
// サンプラ
sampler Sampler : register(s0);

float4 main(VSOutputPT pIn) : SV_Target {
  return float4(ShadowMap.Sample(Sampler, pIn.uv).rrr, 1.0f);
}
