#pragma once
// 今回はとりあえず1シェーダに対応する専用クラスとしてつくります。

namespace dxapp{

	/*!
		@brief	ベーシックシェーダーで使用する定数バッファ

		MEMO:	本来なら規定構造体
	*/
	struct BasicShaderCB 
	{
		// XMFLOAT4X4とシェーダのfloat4x4は同じ形になる
		// Transformデータを行列(マトリクス)に変換して格納
		DirectX::XMFLOAT4X4 world{};
		DirectX::XMFLOAT4X4 wvp{};
	};

	/*!
		@brief BasicShaderでのリソース、デスクリプタの開始インデクス
	*/
	enum class BasicShaderResourceIndex {
		Constant = 0,
		Srv = 2
	};

	class BasicShader
	{
	public:
		BasicShader();
		~BasicShader();

		void Setup(ID3D12Device* device);
		void Teardown();

		/*!
			 @brief フレームでシェーダを使用するときに最初に呼び出す関数
			 @details Endを呼ぶまでは引数のコマンドリストを使う
			 @param[in,ont] commandList コマンドを積むためのコマンドリスト
		*/
		void Begin(ID3D12GraphicsCommandList* commandList);
		/*!
			 @brief フレームでシェーダを使い追わったら必ず呼ぶ関数
		 */

		void End();
		/*!
			@brief シェーダが持っているパラメータでコマンドを発行
		*/
		void Apply();

		/*!
			@brief 参照する定数バッファのデスクリプタ
			@param[in] heaps デスクリプタヒープ
			@param[in] offset ヒープ内でのオフセット位置
		*/
		void SetCBufferDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset);
		/*!
			@brief 参照するSRVのデスクリプタ
			@param[in] heaps デスクリプタヒープ
			@param[in] offset ヒープ内でのオフセット位置
		*/
		void SetSrvDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset);
		/*!
			 @brief 参照するサンプラのデスクリプタ
		  	 @param[in] heaps デスクリプタヒープ
		   	 @param[in] offset ヒープ内でのオフセット位置
		*/
		void SetSamplerDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset);
	private:
		class Impl;
		std::unique_ptr<Impl> impl_;
	};
}//namespace dxapp