// High Level Shader Language = HLSL
// 構文自体はC言語がベース。構造体や関数も作れます。
// データ型や関数は専用のものになったりします

// hlsliはhlsl [i]nclude の略らしいよ
// シェーダーで使用頻度の高い構造体や関数をおいておくと便利

// 変数名の後ろにある「: SV_POSITION | COLOR」はセマンティクスといいます
// 変数の役割を指示する役割がある。
// シェーダの外やり取りする変数には指定する必要があります。
// とりあえず今は「へー」と思っておいてよい

//-------------------------------------------------------------------
// 頂点シェーダへの入力データ型
//-------------------------------------------------------------------
// float型の座標と色が入る構造体
struct VSInput {
  float4 pos : SV_POSITION;  // float4はfloatが4つ(xyzw)入るデータ型
  float4 color : COLOR;
};

// 頂点シェーダの入力
struct VSInputPCNT {
  float3 pos : POSITION;
  float4 color : COLOR;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD;
};

//-------------------------------------------------------------------
// 頂点シェーダからの出力データ型
//-------------------------------------------------------------------
struct VSOutputPT {
  float4 pos : SV_POSITION;  // 射影変換済み頂点
  float2 uv : TEXCOORD;      // UV
};

struct VSOutputPCNT {
  float4 pos : SV_POSITION;  // 射影変換済み頂点
  float4 color : COLOR;      // 頂点カラー
  float3 normal : NORMAL;    // ワールド変換した法線
  float2 uv : TEXCOORD;      // UV
};

struct VSOutputLitTex {
  float4 posVPW : SV_POSITION;  // 最終的な頂点座標
  float3 posW : POSITION;       // ワールド変換だけした頂点
  float3 normal : NORMAL;       // ワールド変換した法線
  float2 uv : TEXCOORD;         // UV
};
