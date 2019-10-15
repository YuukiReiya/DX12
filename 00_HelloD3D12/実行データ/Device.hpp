#pragma once
namespace DX12{
	class Device
	{
	public:
		Device();
		~Device();
		// コピー・代入禁止
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
			@brief	初期化
		*/
		void Setup(HWND hWnd, std::uint32_t width, std::uint32_t height);

		/*
			@brief	画面更新
		*/
		void Present();

		/*
			@brief	次回描画の準備
		*/
		void PrepareRendering();

		/*
			@brief	描画完了まで待つ
		*/
		void WaitForRenderingCompletion();

		/*
			@brief	GPUの処理待ちする
		*/
		void WaitForGPU();
	private:
		void CreateDevice();
		void CreateFence();
		void CreateSwapChain(HWND hWnd, std::uint32_t width, std::uint32_t height, DXGI_FORMAT format);

		//	エイリアステンプレート
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;//※ダックタイピング

		//デバッグビルド時にWARPアダプタを使う
		static constexpr bool UseWrapAdapter{ true };

		//	バックバッファの最大値
		static constexpr std::uint32_t c_MaxBackBufferSize{ 3 };

		ComPtr<ID3D12Device>m_pDevice{ nullptr };
		ComPtr<IDXGIFactory4>m_pDXGIFactory{ nullptr };//デバイス作成用のオブジェクト
		std::uint32_t m_DXGIFactoryFlags{ 0 };//ファクトリの作成フラグ
		ComPtr<IDXGISwapChain4>m_pSwapChain{ nullptr };
		std::uint32_t m_iBackBufferIndex{ 0 };//現在のバックバッファのインデックス
		std::uint32_t m_iBackBufferSize{ 2 };//作成するバックバッファの数
		std::array<ComPtr<ID3D12Resource>, c_MaxBackBufferSize>m_RenderTargets{};//レンダーターゲットテクスチャの保持配列
		DXGI_FORMAT m_dfBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;//初期設定

#pragma region フェンス関連
		//MEMO:フェンスとは…
		//CPU(ゲーム処理)とGPU(描画)の同期をとるためオブジェクト

		/*
			@var		m_pFence
			@brief	GPU処理が終わっているかを確認するために必要なフェンスオブジェクト
		*/
		ComPtr<ID3D12Fence>m_pFence{ nullptr };
		/*
			@var		m_FenceValues
			@brief	フェンスに書き込む値、バックバッファと同じぶんを用意しておく
		*/
		std::array<UINT64, c_MaxBackBufferSize>m_FenceValues{};

		Microsoft::WRL::Wrappers::Event m_FenceEvent{};
#pragma endregion

#pragma region コマンド関連
		/*
			@var		m_pGraphicsCommandList
			@brief	GPUへの命令(コマンド)を蓄えるリスト
						いくつか種類があるが、ID3D12GraphicsCommandListは描画用
		*/
		ComPtr<ID3D12GraphicsCommandList>m_pGraphicsCommandList{ nullptr };

		/*
			@var		m_pCommandAllocators
			@brief	コマンドを発行するためのアロケータ（メモリを確保してくれるひと）
		*/
		std::array<ComPtr<ID3D12CommandAllocator>, c_MaxBackBufferSize>m_pCommandAllocators{};

		/*
			@var		m_pCommandQueue
			@brief	コマンドリストはID3D12CommandQueueに積んでGPUに送るよ
		*/
		ComPtr<ID3D12CommandQueue>m_pCommandQueue{ nullptr };
#pragma endregion

#pragma region RTV
		//レンダーターゲットのデスクリプタ

		/*
			@var		m_pRTVDescriptorHeap
			@brief	レンダーターゲットの設定(Descriptor)をVRAMに保存するのに使います
		*/
		ComPtr<ID3D12DescriptorHeap>m_pRTVDescriptorHeap{ nullptr };

		/*
			@var		m_RTVDescriptorSize
			@brief	レンダーターゲットのデスクリプタオブジェクトのサイズ
		*/
		UINT m_RTVDescriptorSize{ 0 };

		D3D12_VIEWPORT m_ScreenViewport{};
		D3D12_RECT m_ScissorRect{};
#pragma endregion

	};
}//namespace