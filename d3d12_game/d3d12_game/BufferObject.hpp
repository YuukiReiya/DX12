#pragma once
namespace dxapp{
	enum class BufferType
	{
		/*!
			@brief	頂点バッファ
		*/
		VertexBuffer,
		/*!
			@brief	インデックスバッファ
		*/
		IndexBuffer,
		/*!
			@brief	コンスタントバッファ
		*/
		ConstantBuffer,
		/*!
			@brief	enumのサイズ
		*/
		Max
	};

	class BufferObject
	{
	public:
		BufferObject(const BufferObject&) = delete;
		BufferObject& operator=(const BufferObject&) = delete;

		BufferObject() = default;

		/*!
			@brief	初期化
		*/
		bool Setup(ID3D12Device* device, const BufferType type, std::size_t size);
		/*!
			@brief	終了処理
		*/
		void Teardown();

		/*!
			@brief	マップ
		*/
		void* Map();
		/*!
			@brief	アンマップ
		*/
		void UnMap();
		void Update(const void* data, std::size_t size, std::size_t offset = 0);
		
		/*!
			@brief	確保したバッファのサイズ
		*/
		std::size_t GetBufferSize()const { return m_BufferSize; }
		/*!
			@brief 作成したバッファのリソース
	   */
		ID3D12Resource1* GetResouce() { return m_Resource.Get(); }

	private:
		/*!
			@brief	ヒーププロパティ
		*/
		CD3DX12_HEAP_PROPERTIES m_HeapProp;
		CD3DX12_RESOURCE_DESC m_ResourceDesc;
		std::size_t m_BufferSize;
		Microsoft::WRL::ComPtr<ID3D12Resource1>m_Resource;
	};

}//namespace dxapp