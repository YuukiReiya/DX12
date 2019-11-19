// High Level Shader Language = HLSL
// 構文自体はC言語がベース。構造体や関数も作れます。
// データ型や関数は専用のものになったりします

// hlsliはhlsl [i]nclude の略らしいよ
// シェーダーで使用頻度の高い構造体や関数をおいておくと便利

// float型の座標と色が入る構造体
struct VSInput {
  float4 pos : SV_POSITION;  // float4はfloatが4つ(xyzw)入るデータ型
  float4 color : COLOR;
};

// 変数名の後ろにある「: SV_POSITION | COLOR」はセマンティクスといいます
// 変数の役割を指示する役割がある。
// シェーダの外やり取りする変数には指定する必要があります。
// とりあえず今は「へー」と思っておいてよい

// 頂点シェーダの入力
struct VSInputPCNT {
  float3 pos : POSITION;
  float4 color : COLOR;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD;
};

// 頂点シェーダから出力
struct VSOutputPCNT {
  float4 pos : SV_POSITION;
  float4 color : COLOR;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD;
};

//-------------------------------------------------------------------
// 11/12 追加ここから
//-------------------------------------------------------------------
// ライティング頂点シェーダの出力
struct VSOutputLitTex {
  float4 posVPW : SV_POSITION;  // 最終的な頂点座標
  float3 posW : POSITION;       // ワールド変換だけした頂点
  float3 normal : NORMAL;       // ワールド変換した法線
  float2 uv : TEXCOORD;         // テクスチャ座標
};

// ディレクショナルライト3個まで対応
#define NUM_DIR_LIGHT 3

// ライト構造体（ディレクショナルだけ対応）
// 定数バッファは16byte単位でメモリを扱う。
// floatは4byte = float3なら12byteになる。
// そのためfloat3のあとにfloat3を並べると、16byteをこえメモリの境界を
// またいでしまうので、そのようなときは詰め物(padding)で並びを整えておく
struct Light {
  float3 strength;  // ライト色・明るさ
  float pad1;
  float3 direction;  // ディレクショナルライトの向き
  float pad2;
};

//-------------------------------------------------------------------
// 11/12 追加ここまで
//-------------------------------------------------------------------
