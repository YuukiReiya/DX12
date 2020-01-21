#pragma once
// DirectXMathはDirectX向けにつくられた高速な数学関数と
// 専用のデータ型が定義してある便利な奴
#include <DirectXMath.h>

namespace dxapp {
/*!
 * @brief 頂点データ：座標と頂点カラーを持つ
 */
struct VertexPositionColor {
  DirectX::XMFLOAT3 position;  //! 3D空間での座標
  DirectX::XMFLOAT4 color;     //! 頂点単位の色
};

// D3D12_INPUT_ELEMENT_DESCは頂点データのレイアウト(メンバの意味とサイズなど)を定義
// このレイアウトと頂点シェーダの入力が合っていないと正しく描画できない
// D3D12_INPUT_ELEMENT_DESCの宣言
//   SemanticName; - セマンティック名(使用用途)
//   SemanticIndex; - 同じセマンティックがあるとき使う
//   Format; - 使っているデータフォーマット
//   InputSlot; - 通常0
//   AlignedByteOffset; - 構造体の先頭からのバイトオフセット
//   InputSlotClass; - 通常はD3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA
//   InstanceDataStepRate; - 通常0

/*!
 * @brief VertexPositionColorのレイアウト情報
 */
static constexpr D3D12_INPUT_ELEMENT_DESC VertexPositionColorElement[]{
    {"SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
     D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
     0},
    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
     D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
     0},
};

// static constexprだとヘッダでも定義ができて便利(最近のC++でないとつかえないよ)

#pragma region add_ch04
// #pragma region リジョージョン名
// この間のコードがコードブロックになり、折り畳みができるようになる。べんり
// #pragma endregion

// 座標・色・法線(ノーマル)・UV(テクスチャ座標)
struct VertexPositionColorNormalTexture {
  DirectX::XMFLOAT3 position;
  DirectX::XMFLOAT4 color;
  DirectX::XMFLOAT3 normal;
  DirectX::XMFLOAT2 uv;
};

// 座標・色・UV(テクスチャ座標)のインプットレイアウト
static constexpr D3D12_INPUT_ELEMENT_DESC
    VertexPositionColorNormalTexturElement[]{
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
         D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
         D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
         D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
         D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
#pragma endregion add_ch04

}  // namespace dxapp
