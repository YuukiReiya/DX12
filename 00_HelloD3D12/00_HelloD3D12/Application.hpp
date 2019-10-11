#pragma once
//デバイスの前方宣言
namespace DX12 {
	class Device;
}
//アプリケーションクラスの宣言
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
			@brief	初期化
		*/
		void Setup(HWND hWnd, std::uint32_t width, std::uint32_t height);
		
		/*
			@brief	メイン処理
		*/
		void Execute();

		/*
			@brief	破棄処理
		*/
		void Teardown();
	private:
		/*
			@brief	更新
		*/
		void Update();

		/*
			@brief	描画
		*/
		void Render();

		/*
			@brief	RTVのクリア
		*/
		void ClearRTV();

		/*
			@var		m_hWnd
			@brief	ハンドラ
		*/
		HWND m_hWnd{ nullptr };

		/*
			@var		m_pDevice
			@brief	デバイス
		*/
		std::unique_ptr<DX12::Device>m_pDevice;
	};
}	//namespace