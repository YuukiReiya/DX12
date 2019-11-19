#include "ShaderCommon.hlsli"

// 今回は定数バッファを沢山使うよ

// オブジェクト単位の定数バッファ
cbuffer ObjectParam : register(b0) {
  float4x4 World;     // ワールド行列
  float4x4 TexTrans;  // UVのトランスフォーム用
};

// シーン単位の定数バッファ
cbuffer SceneParam : register(b1) {
  float4x4 View;      // ビュー行列
  float4x4 Proj;      // 射影行列
  float4x4 ViewProj;  // ビュー射影行列
  float3 EyePos;      // 視点
  float padding[1];   // メモリアライメント用
  float4
      AmbientLight;  // 環境光（光があたらない場所でも最低限、このライト分は明るくなる）
  Light Lights[NUM_DIR_LIGHT];  // ライト（3つ分）
};

// マテリアル(メッシュの素材設定)毎の定数バッファ
cbuffer MaterialParam : register(b2) {
  float4x4 MatTrans;     // UVのトランスフォーム用
  float4 DiffuseAlbedo;  // 素材の色と思ってよい
  float3 Fresnel;        // 反射光の色
  float Roughness;       // 表面の粗さ（反射の具合が変わる）

  int useTexture;  // テクスチャ使用フラグ（intなので注意）
};

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
