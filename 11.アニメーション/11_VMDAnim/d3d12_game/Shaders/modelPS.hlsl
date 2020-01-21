#include "model.hlsli"

// マテリアル
cbuffer MaterialParam : register(b2) {
  float4 Diffuse;
  float4 Specular;  // 今回は使わないです
  float3 Ambient;   // 今回は使わないです
  int useTexture;   // テクスチャ使用フラグ
};

//---

// テクスチャ, サンプラ
Texture2D<float4> Texture : register(t0);
SamplerState Sampler : register(s0);

//---

// ディフューズとテクスチャカラーだけのピクセルシェーダ
float4 main(VSOutput pIn) : SV_TARGET {
  float4 color = Diffuse;
  if (useTexture == 1) {
    // テクスチャ色とマテリアルカラーを混ぜる
    color *= Texture.Sample(Sampler, pIn.uv);
  } 

  return color;
}
