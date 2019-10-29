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
