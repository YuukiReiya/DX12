// マテリアル(メッシュの素材設定)毎の定数バッファ
cbuffer MaterialParam : register(b2) {
  float4x4 MatTrans;     // UVのトランスフォーム用
  float4 DiffuseAlbedo;  // 素材の色と思ってよい
  float3 Fresnel;        // 反射光の色
  float Roughness;       // 表面の粗さ（反射の具合が変わる）
  int useTexture;  // テクスチャ使用フラグ（intなので注意）
};
