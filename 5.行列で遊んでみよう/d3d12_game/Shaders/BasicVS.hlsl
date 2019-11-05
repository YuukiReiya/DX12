#include "ShaderCommon.hlsli"

// シェーダが外部からもらうデータを「リソース」といいます
// 下記の4種類がある
//  register: b - コンスタント(定数)バッファービュー(CBV)
//  register: t - シェーダーリソースビュー(SRV)、テクスチャだね
//  register: s - サンプラー、テクスチャの参照方法
//  register: u - 順序指定されていないアクセス ビュー(UAV)。とりあえず忘れてよい

// プログラム側から渡されるリソースと、シェーダーのリソースは一致している必要がある

// cbuffer 名前 : register(bレジスタ番号) {
//  値1
//  値2
//  ...
// } で定義できる。

// register(bレジスタ番号)は、リソースが格納されている場所を指す
// またリソースは変数ではなく、プログラムからもらうもの。
// データの並び順はシェーダとプログラム側で合わせる必要がある。
// 頂点シェーダの入力と同じ仕組みだね
cbuffer ObjectPram : register(b0) {  // 定数バッファのb0に格納される
  float4x4 world;                    // オブジェクトのワールド行列
  float4x4 worldViewProj;  // オブジェクトの射影変換済み行列
}

VSOutputPCNT main(VSInputPCNT vIn) {
  VSOutputPCNT vOut = (VSOutputPCNT)0;

  // mulは掛け算、float4つとfloat4x4行列もいい感じに掛け算してくれます
  // 頂点とworldViewProjを掛け算することで1頂点ごとに座標変換
  vOut.pos = mul(vIn.pos, worldViewProj);

  vOut.color = vIn.color;
  vOut.normal = vIn.normal;
  vOut.uv = vIn.uv;

  return vOut;
}
