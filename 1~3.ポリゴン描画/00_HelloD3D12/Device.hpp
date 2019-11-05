#pragma once
namespace DX12{
	class Device
	{
	public:
		Device();
		~Device();
		// �R�s�[�E����֎~
		Device(const Device&) = delete;
		Device& operator=(const Device&) = delete;
		ID3D12Device* GetDevice()const { return m_pDevice.Get(); };
		ID3D12GraphicsCommandList* GetGraphicsCommandList()const { return m_pGraphicsCommandList.Get(); };
		CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV()const {
			return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_iBackBufferIndex, m_RTVDescriptorSize);
		};
		D3D12_VIEWPORT GetScreeViewport()const { return m_ScreenViewport; };
		D3D12_RECT GetScissorRect()const { return m_ScissorRect; };

		/*
			@brief	������
		*/
		void Setup(HWND hWnd, std::uint32_t width, std::uint32_t height);

		/*
			@brief	��ʍX�V
		*/
		void Present();

		/*
			@brief	����`��̏���
		*/
		void PrepareRendering();

		/*
			@brief	�`�抮���܂ő҂�
		*/
		void WaitForRenderingCompletion();

		/*
			@brief	GPU�̏����҂�����
		*/
		void WaitForGPU();
	private:
		void CreateDevice();
		void CreateFence();
		void CreateSwapChain(HWND hWnd, std::uint32_t width, std::uint32_t height, DXGI_FORMAT format);

		//	�G�C���A�X�e���v���[�g
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;//���_�b�N�^�C�s���O

		//�f�o�b�O�r���h����WARP�A�_�v�^���g��
		static constexpr bool UseWrapAdapter{ true };

		//	�o�b�N�o�b�t�@�̍ő�l
		static constexpr std::uint32_t c_MaxBackBufferSize{ 3 };

		ComPtr<ID3D12Device>m_pDevice{ nullptr };
		ComPtr<IDXGIFactory4>m_pDXGIFactory{ nullptr };//�f�o�C�X�쐬�p�̃I�u�W�F�N�g
		std::uint32_t m_DXGIFactoryFlags{ 0 };//�t�@�N�g���̍쐬�t���O
		ComPtr<IDXGISwapChain4>m_pSwapChain{ nullptr };
		std::uint32_t m_iBackBufferIndex{ 0 };//���݂̃o�b�N�o�b�t�@�̃C���f�b�N�X
		std::uint32_t m_iBackBufferSize{ 2 };//�쐬����o�b�N�o�b�t�@�̐�
		std::array<ComPtr<ID3D12Resource>, c_MaxBackBufferSize>m_RenderTargets{};//�����_�[�^�[�Q�b�g�e�N�X�`���̕ێ��z��
		DXGI_FORMAT m_dfBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;//�����ݒ�

#pragma region �t�F���X�֘A
		//MEMO:�t�F���X�Ƃ́c
		//CPU(�Q�[������)��GPU(�`��)�̓������Ƃ邽�߃I�u�W�F�N�g

		/*
			@var		m_pFence
			@brief	GPU�������I����Ă��邩���m�F���邽�߂ɕK�v�ȃt�F���X�I�u�W�F�N�g
		*/
		ComPtr<ID3D12Fence>m_pFence{ nullptr };
		/*
			@var		m_FenceValues
			@brief	�t�F���X�ɏ������ޒl�A�o�b�N�o�b�t�@�Ɠ����Ԃ��p�ӂ��Ă���
		*/
		std::array<UINT64, c_MaxBackBufferSize>m_FenceValues{};

		Microsoft::WRL::Wrappers::Event m_FenceEvent{};
#pragma endregion

#pragma region �R�}���h�֘A
		/*
			@var		m_pGraphicsCommandList
			@brief	GPU�ւ̖���(�R�}���h)��~���郊�X�g
						��������ނ����邪�AID3D12GraphicsCommandList�͕`��p
		*/
		ComPtr<ID3D12GraphicsCommandList>m_pGraphicsCommandList{ nullptr };

		/*
			@var		m_pCommandAllocators
			@brief	�R�}���h�𔭍s���邽�߂̃A���P�[�^�i���������m�ۂ��Ă����ЂƁj
		*/
		std::array<ComPtr<ID3D12CommandAllocator>, c_MaxBackBufferSize>m_pCommandAllocators{};

		/*
			@var		m_pCommandQueue
			@brief	�R�}���h���X�g��ID3D12CommandQueue�ɐς��GPU�ɑ����
		*/
		ComPtr<ID3D12CommandQueue>m_pCommandQueue{ nullptr };
#pragma endregion

#pragma region RTV
		//�����_�[�^�[�Q�b�g�̃f�X�N���v�^

		/*
			@var		m_pRTVDescriptorHeap
			@brief	�����_�[�^�[�Q�b�g�̐ݒ�(Descriptor)��VRAM�ɕۑ�����̂Ɏg���܂�
		*/
		ComPtr<ID3D12DescriptorHeap>m_pRTVDescriptorHeap{ nullptr };

		/*
			@var		m_RTVDescriptorSize
			@brief	�����_�[�^�[�Q�b�g�̃f�X�N���v�^�I�u�W�F�N�g�̃T�C�Y
		*/
		UINT m_RTVDescriptorSize{ 0 };

		D3D12_VIEWPORT m_ScreenViewport{};
		D3D12_RECT m_ScissorRect{};
#pragma endregion

	};
}//namespace