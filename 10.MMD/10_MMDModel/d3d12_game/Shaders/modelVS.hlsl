#include "model.hlsli"

// オブジェクト定数
cbuffer ObjectParam : register(b0) {
  float4x4 World;
}

// オブジェクト定数
cbuffer SceneParam : register(b1) {
  float4x4 View;
  float4x4 Proj;
  float4x4 ViewProj;
}

VSOutput main(VSInput vIn) {
  VSOutput vOut = (VSOutput)0;

  // 頂点をローカル座標からワールド座標に変換
  float4 posW = mul(float4(vIn.pos, 1.0f), World);
  vOut.pos = mul(posW, ViewProj);

  // 法線をローカルからワールドに変換
  vOut.normal = mul(vIn.normal, (float3x3)World);

  vOut.uv = vIn.uv; 
  return vOut;
}
