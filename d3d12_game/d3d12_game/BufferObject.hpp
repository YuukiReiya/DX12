#pragma once
namespace dxapp{
	enum class BufferType
	{
		/*!
			@brief	���_�o�b�t�@
		*/
		VertexBuffer,
		/*!
			@brief	�C���f�b�N�X�o�b�t�@
		*/
		IndexBuffer,
		/*!
			@brief	�R���X�^���g�o�b�t�@
		*/
		ConstantBuffer,
		/*!
			@brief	enum�̃T�C�Y
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
			@brief	������
		*/
		bool Setup(ID3D12Device* device, const BufferType type, std::size_t size);
		/*!
			@brief	�I������
		*/
		void Teardown();

		/*!
			@brief	�}�b�v
		*/
		void* Map();
		/*!
			@brief	�A���}�b�v
		*/
		void UnMap();
		void Update(const void* data, std::size_t size, std::size_t offset = 0);
		
		/*!
			@brief	�m�ۂ����o�b�t�@�̃T�C�Y
		*/
		std::size_t GetBufferSize()const { return m_BufferSize; }
		/*!
			@brief �쐬�����o�b�t�@�̃��\�[�X
	   */
		ID3D12Resource1* GetResouce() { return m_Resource.Get(); }

	private:
		/*!
			@brief	�q�[�v�v���p�e�B
		*/
		CD3DX12_HEAP_PROPERTIES m_HeapProp;
		CD3DX12_RESOURCE_DESC m_ResourceDesc;
		std::size_t m_BufferSize;
		Microsoft::WRL::ComPtr<ID3D12Resource1>m_Resource;
	};

}//namespace dxapp