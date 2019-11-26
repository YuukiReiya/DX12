
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
  float3 EyePos;      // カメラの位置
  float pad;          // メモリの並びを整える用
  float4
      AmbientLight;  // 環境光（光があたらない場所でも最低限、このライト分は明るくなる）
  Light Lights[NUM_DIR_LIGHT];  // ライト（3つ分）
};
