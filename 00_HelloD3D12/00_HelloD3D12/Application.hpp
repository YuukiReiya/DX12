#pragma once
//�f�o�C�X�̑O���錾
namespace DX12 {
	class Device;
}
//�A�v���P�[�V�����N���X�̐錾
namespace dxapp
{
	class Application
	{
	public:
		Application();
		virtual ~Application();
		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

		/*
			@brief	������
		*/
		void Setup(HWND hWnd, std::uint32_t width, std::uint32_t height);
		
		/*
			@brief	���C������
		*/
		void Execute();

		/*
			@brief	�j������
		*/
		void Teardown();
	private:
		/*
			@brief	�X�V
		*/
		void Update();

		/*
			@brief	�`��
		*/
		void Render();

		/*
			@brief	RTV�̃N���A
		*/
		void ClearRTV();

		/*
			@var		m_hWnd
			@brief	�n���h��
		*/
		HWND m_hWnd{ nullptr };

		/*
			@var		m_pDevice
			@brief	�f�o�C�X
		*/
		std::unique_ptr<DX12::Device>m_pDevice;
	};
}	//namespace