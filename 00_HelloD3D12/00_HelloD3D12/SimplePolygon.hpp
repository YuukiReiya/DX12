#pragma once
#include "Vertex.hpp"

namespace dxapp
{
	class SimplePolygon
	{
	public:
#pragma region インライン
		SimplePolygon() = default;
		~SimplePolygon() = default;
#pragma endregion

		void Setup(ID3D12Device* device);
		void Teardown();

		/*!
				@brief	描画コマンドの構築
		*/
		void MakeCommandList(ID3D12GraphicsCommandList* commandList);
	private:
		void MakeTriangle();
		void CreateShader(const std::wstring& fileName, const std::wstring& profile, Microsoft::WRL::ComPtr<ID3DBlob>& blob);
		/*!
			@brief	頂点/インデックスバッファを作成し、VRAMにコピー
		*/
		void CreateBufferObject();
		/*!
		* @brief 頂点バッファ、インデックスバッファの生成関数.
		* @detail 頂点もインデックスも同じコードでID3DBlobを作るので省力化
		*/
		void CreateCommittedResource(std::size_t bufferSize, Microsoft::WRL::ComPtr<ID3D12Resource1>& resource);
		/*!
			@brief	マッピング(頂点/インデックスバッファにデータをコピーする)
		*/
		void Mapping(Microsoft::WRL::ComPtr<ID3D12Resource1>& resource, const void* src, std::size_t bufferSize);
		/*!
			@brief	ルートシグネチャの構築
		*/
		void CreateRootSignature();
		/*!
			@brief	パイプラインステートオブジェクト(PSO)の構築
		*/
		void CreatePipelineState();

		ID3D12Device* m_pDevice{};
		/*!
			@brief	頂点配列
		*/
		std::vector<VertexPositionColor>m_vPolygonVertices{};
		/*!
			@brief	インデックス配列
		*/
		std::vector<std::uint32_t>m_vPolygonIndices{};
#pragma region バッファ
		Microsoft::WRL::ComPtr<ID3D12Resource1>m_pVertexBuffer{};//頂点
		Microsoft::WRL::ComPtr<ID3D12Resource1>m_pIndexBuffer{};//インデックス
#pragma endregion

		 // 上で作ったバッファーデータは生のメモリの塊でGPUには意味が伝わらない
		 // 次の変数にVRAMにあるメモリの読み方を教えて描画の時に使う

		/*!
			@detail		"V"ertex"B"uffer"V"iew
		*/
		D3D12_VERTEX_BUFFER_VIEW m_vbView{};

		/*!
			@detail		"I"ndex"B"uffer"V"iew
		*/
		D3D12_INDEX_BUFFER_VIEW m_ibView{};
#pragma region シェーダーオブジェクト
		Microsoft::WRL::ComPtr<ID3DBlob>m_pVertexShader{};//頂点
		Microsoft::WRL::ComPtr<ID3DBlob>m_pPixelShader{};//ピクセル	
#pragma endregion
		/*!
			@brief	シェーダーとリソースの対応をさせるオブジェクト
		*/
		Microsoft::WRL::ComPtr<ID3D12RootSignature>m_pRootSignature{};
		/*!
			@brief	パイプライン
		*/
		Microsoft::WRL::ComPtr<ID3D12PipelineState>m_pPipeLine{};
	};
}//namespace
