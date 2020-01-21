#include "model.hlsli"

// オブジェクト定数
cbuffer ObjectParam : register(b0) {
  float4x4 World;
  float4x4 Bone[256];
  //---
}

// オブジェクト定数
cbuffer SceneParam : register(b1) {
  float4x4 View;
  float4x4 Proj;
  float4x4 ViewProj;
}

//---

// スキニング
VSOutput Skinning(VSInput vIn) {
  VSOutput vOut = (VSOutput)0;
  float4 pos = 0;
  float3 nrm = 0;

  uint boneID[2] = (uint[2])vIn.boneID;
  float weight[2] = (float[2])vIn.boneWeights;

  // pmdは頂点あたり2つのボーンの影響をうける
  for (int i = 0; i < 2; i++) {
    // 参照する骨の行列をもらって
    float4x4 m = Bone[boneID[i]];

    // 頂点をボーンの行列で変換それにウェイトをかける
    // ボーン1とボーン2での結果を加算してブレンディンする
    pos += mul(float4(vIn.pos, 1.0), m) * weight[i];
    nrm += mul(vIn.normal, (float3x3)m) * weight[i];
  }
  vOut.pos = pos;
  vOut.normal = normalize(nrm);

  return vOut;
}

//---

VSOutput main(VSInput vIn) {
  VSOutput vOut = Skinning(vIn);

  vOut.pos = mul(mul(vOut.pos, World), ViewProj);
  vOut.normal = mul(vOut.normal, (float3x3)World);
  vOut.uv = vIn.uv;

  return vOut;
}
