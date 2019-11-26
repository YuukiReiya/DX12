#include "ShaderCommon.hlsli"

#include "BaseConstant.hlsli"
#include "MaterialConstant.hlsli"

VSOutputLitTex main(VSInputPCNT vIn) {
  VSOutputLitTex vOut = (VSOutputLitTex)0;

  // 頂点をローカル（モデリング）座標からワールド座標に変換
  float4 posW = mul(float4(vIn.pos, 1.0f), World);
  vOut.posW = posW.xyz;
  // VPWも計算
  vOut.posVPW = mul(posW, ViewProj);

  // 法線をローカルからワールドに変換
  vOut.normal = mul(vIn.normal, (float3x3)World);

  // テクスチャのトランスフォーム
  // UVのスクロールやテクスチャの回転などが使えて便利
  float4 uv = mul(float4(vIn.uv, 0.0f, 1.0f), TexTrans);
  uv = mul(uv, MatTrans);
  vOut.uv = uv.xy;  // .xyでxyzwの4成分からxy成分が渡せる

  return vOut;
}
