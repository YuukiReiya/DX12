#pragma once

namespace dxapp
{
	/*!
			@brief	頂点情報
	*/
	struct VertexPositionColor
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
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

	static constexpr D3D12_INPUT_ELEMENT_DESC c_VertexLayout[]
	{
		{"SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
		 D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
		 D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
}//namespace
