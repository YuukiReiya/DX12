#include "pch.h"
#include "Device.hpp"

DX12::Device::Device() {}

DX12::Device::~Device()
{
	// GPUの処理が終わってるいるのを確認してから終わるようにする
	WaitForGPU();
#if  _DEBUG||DEBUG
	{
		//D3Dの生きているオブジェクトを確認できる
		ComPtr<ID3D12DebugDevice>debugInterface;
		if (SUCCEEDED(m_pDevice->QueryInterface(IID_PPV_ARGS(&debugInterface)))) { debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL); }
	}
#endif
}

void DX12::Device::Setup(HWND hWnd, std::uint32_t width, std::uint32_t height)
{
#pragma region デバイス
	CreateDevice();
#pragma endregion

	// 描画命令を生成・積み込みに使うオブジェクトを作ります
	// ここで作る3種類のオブジェクトを組み合わせることで描画ができるようになります

#pragma region コマンドキュー
	{
		// D3D12_COMMAND_LIST_TYPE_DIRECTは描画命令で使えるやつ
		D3D12_COMMAND_QUEUE_DESC desc{};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//詰めるリストの種類
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		// D3D12_COMMAND_LIST_TYPE_DIRECTのキューには
		// LIST_TYPE_DIRECTとLIST_TYPE_BUNDLEのコマンドを積むことができる。
		// 今回はLIST_TYPE_DIRECTだけやる。BUNDLEについては今後に期待。
		auto hr = m_pDevice->CreateCommandQueue(
			&desc,
			IID_PPV_ARGS(m_pCommandQueue.ReleaseAndGetAddressOf()));
		if (FAILED(hr)) { throw std::runtime_error("Device::CreateCommandQueue Failed."); }
		m_pCommandQueue->SetName(L"Device::ID3D12CommandQueue");
		// コマンドリストを作るにはコマンドリストの種類に応じたメモリアロケータがいる
		// コマンド生成時にアロケータがメモリを割り当てていくよ
	}
#pragma endregion
#pragma region コマンドアロケーター
	{
		for (auto& alloc : m_pCommandAllocators)
		{
			auto hr = m_pDevice->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,// 生成するコマンドリストの種類を指定
				IID_PPV_ARGS(alloc.ReleaseAndGetAddressOf()));
			if (FAILED(hr)) { throw std::runtime_error("Device::CreateCommandAllocator Failed."); }
			alloc->SetName(L"Device::ID3D12CommandAllocator");
		}
	}
#pragma endregion
#pragma region コマンドリスト
	{
		auto hr = m_pDevice->CreateCommandList(
			0,//基本は"0"
			D3D12_COMMAND_LIST_TYPE_DIRECT,//通所の描画には"DIRECT"を使う
			m_pCommandAllocators[0].Get(),//メモリを確保するアロケーター
			nullptr,//ヌルぽでOK
			IID_PPV_ARGS(m_pGraphicsCommandList.ReleaseAndGetAddressOf())
		);
		if (FAILED(hr)) { throw std::runtime_error("Device::CreateCommandList Failed."); }
		m_pGraphicsCommandList->SetName(L"Device::CreateCommandList");
		m_pGraphicsCommandList->Close();
	}
#pragma endregion
#pragma region フェンス
	CreateFence();
#pragma endregion
#pragma region スワップチェイン
	CreateSwapChain(hWnd, width, height, m_dfBackBufferFormat);
#pragma endregion
#pragma region RTV
	m_ScreenViewport.Width = static_cast<FLOAT>(width);
	m_ScreenViewport.Height = static_cast<FLOAT>(height);
	m_ScreenViewport.TopLeftX = 0.0f;
	m_ScreenViewport.TopLeftY = 0.0f;
	m_ScreenViewport.MinDepth = D3D12_MIN_DEPTH;
	m_ScreenViewport.MaxDepth = D3D12_MAX_DEPTH;

	m_ScissorRect.left = 0;
	m_ScissorRect.top = 0;
	m_ScissorRect.right = width;
	m_ScissorRect.bottom = height;
#pragma endregion
}

void DX12::Device::Present()
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_RenderTargets[m_iBackBufferIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
	);
	// コマンドリストはCloseしておかないと実行できませんよ
	m_pGraphicsCommandList->Close();

	// ここでやっとGPUに描画処理をさせる
	// ExecuteはID3D12CommandListじゃないとダメなのでキャストしてるだけ
	auto cl = reinterpret_cast<ID3D12CommandList* const*>(
		m_pGraphicsCommandList.GetAddressOf());

	// キューに積まれたコマンドを実行する
	m_pCommandQueue->ExecuteCommandLists(
		1,// キューには複数のリストが詰めるので実行するリスト数を指定
		cl
	);

	// スワップチェインの内容を切り替える
	auto hr = m_pSwapChain->Present(
		1,// バックバッファを切り替えにVSyncを何度待つか。1で1回まつ
		0
	);

	// しかしExecuteCommandLists / PresentもGPUに「働け！」と指示しているだけ。
	// つまり非同期実行なので、描画がおわるのをまって次回のフレームに進む必要がある
	WaitForRenderingCompletion();
}

void DX12::Device::PrepareRendering()
{
	// アロケータとコマンドリストをリセットして前の内容を忘れるよ
	m_pCommandAllocators[m_iBackBufferIndex]->Reset();
	m_pGraphicsCommandList->Reset(m_pCommandAllocators[m_iBackBufferIndex].Get(), nullptr);

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_RenderTargets[m_iBackBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	// リソースバリアを設定
	m_pGraphicsCommandList->ResourceBarrier(1, &barrier);
}

void DX12::Device::WaitForRenderingCompletion()
{
	const auto currentValue = m_FenceValues[m_iBackBufferIndex];
	m_pCommandQueue->Signal(m_pFence.Get(), currentValue);

	// バックバッファが切り替わっているので新しいインデックスをもらう
	m_iBackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// 処理速度によってはここに到達した時点で描画が終わりフェンス値が更新されている可能性もある
	// その状態でWaitForSingleObjectExをすると無限に待つことになるぞ。
	// そうならないようにGetCompletedValueでフェンスの現在値を確認する。
	if (m_pFence->GetCompletedValue() < m_FenceValues[m_iBackBufferIndex])
	{
		// まだ描画が終わっていないので待つ必要がある
		m_pFence->SetEventOnCompletion(m_FenceValues[m_iBackBufferIndex], m_FenceEvent.Get());
		WaitForSingleObjectEx(m_FenceEvent.Get(), INFINITE, false);
	}
	// 次のフレームのためにフェンス値更新
	m_FenceValues[m_iBackBufferIndex] = currentValue + 1;
}

void DX12::Device::WaitForGPU()
{
	if (m_pCommandQueue && m_pFence && m_FenceEvent.IsValid())
	{
		// 現在のフェンス値をローカルにコピー
		// ローカルにコピーしたほうが速度面で有利とのうわさ(検証してない)
		auto currentValue = m_FenceValues[m_iBackBufferIndex];

		// キューに積んだコマンドの実行完了を待ちます

		// CommandQueue::Signal(フェンス, 更新された時の値)をすると
		// キューの実行が終わったときにフェンスの中身が第2引数の値に更新される
		if (SUCCEEDED(m_pCommandQueue->Signal(m_pFence.Get(), currentValue))) {
			// SetEventOnCompletionはfence_が持っている値が、currentValueに
			// 更新されたときにイベントが飛んでくるように設定する
			if (SUCCEEDED(m_pFence->SetEventOnCompletion(currentValue, m_FenceEvent.Get())))
			{
				// ここで待ちます
				// 無限ループみたいなやり方でも待てるけど、CPUを無駄に使うのでお勧めしない
				WaitForSingleObjectEx(m_pFence.Get(), INFINITE, FALSE);

				// 次回の処理のためにフェンス値を増やしておく
				m_FenceValues[m_iBackBufferIndex]++;
			}
		}
	}
}

void DX12::Device::CreateDevice()
{
	HRESULT hr = S_FALSE;
#if _DEBUG
	{
		ComPtr<ID3D12Debug>debug;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) { debug->EnableDebugLayer(); }
		else { throw std::runtime_error("Device::D3D12GetDebugInterface Failed."); }
		//機能設定
		ComPtr<IDXGIInfoQueue>DXGIInfoQueue;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(DXGIInfoQueue.GetAddressOf()))))
		{
			m_DXGIFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
			DXGIInfoQueue->SetBreakOnSeverity(
				DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			DXGIInfoQueue->SetBreakOnSeverity(
				DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		}
	}
#endif // _DEBUG

#pragma region ファクトリ
	hr = CreateDXGIFactory2(m_DXGIFactoryFlags, IID_PPV_ARGS(m_pDXGIFactory.ReleaseAndGetAddressOf()));
	if (FAILED(hr)) { throw std::runtime_error("Device::CreateDXGIFactory2 Failed."); }
#pragma endregion

#pragma region アダプタ
	// アダプタ(Windowsから見えるGPUっぽいやつ)作成
	ComPtr<IDXGIAdapter1>adapter{ nullptr };
	for (UINT i = 0;; ++i)
	{
		// 使えるアダプタを順番調べる
		if (DXGI_ERROR_NOT_FOUND ==
			m_pDXGIFactory->EnumAdapters1(i, adapter.ReleaseAndGetAddressOf())) {
			// DXGI_ERROR_NOT_FOUNDはアダプタがない状態
			break;
		}

		// アダプタの情報を取得してゲームで使えるかかチェック
		DXGI_ADAPTER_DESC1 desc{};
		hr = adapter->GetDesc1(&desc);
		if (FAILED(hr)) { throw std::runtime_error("Device::IDXGIAdapter1::GetDesc1 Failed."); }

		// ソフトウェアアダプタ(CPU処理)は遅すぎるので使わない
		if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) { continue; }

		// アダプタがD3D12を使えるかチェック
		// ためしにアダプタからデバイスを作ってみる
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0,
			_uuidof(ID3D12Device), nullptr))) {
			// D3D12が使えるアダプタがあったのでループ抜ける
			break;
		}
	}
#if _DEBUG
	if (!adapter) {
		if (UseWrapAdapter && FAILED(m_pDXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf())))) { throw std::runtime_error("WARP adapter not found."); }
	}
#endif
	if (!adapter) { throw std::runtime_error("D3D12 device not found."); }
#pragma endregion

	//アダプタからデバイスの作成
	hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_pDevice.ReleaseAndGetAddressOf()));
	if (FAILED(hr)) { throw std::runtime_error("Device::CreateDevice Failed."); }
	m_pDevice->SetName(L"Device::ID3D12Device");
}

void DX12::Device::CreateFence()
{
	//フェンスオブジェクトを作る
	auto hr = m_pDevice->CreateFence(
		m_FenceValues[m_iBackBufferIndex],//初期値
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(m_pFence.ReleaseAndGetAddressOf())
	);
	if (FAILED(hr)) { throw std::runtime_error("Device::CreateFence Failed."); }
	m_pFence->SetName(L"Device::ID3D12Fence");
	//次回のフェンス値を設定
	m_FenceValues[m_iBackBufferIndex]++;
	// GPUの処理が終わるの待機するために使います
	m_FenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
	if (!m_FenceEvent.IsValid()) { throw std::runtime_error("Device::CreateEventEx Failed."); }
}

void DX12::Device::CreateSwapChain(HWND hWnd, std::uint32_t width, std::uint32_t height, DXGI_FORMAT format)
{
	WaitForGPU();

#pragma region RTVとフェンスの初期化
	for (std::uint32_t i = 0; i < c_MaxBackBufferSize; ++i)
	{
		m_RenderTargets[i].Reset();
		m_FenceValues[i] = m_FenceValues[m_iBackBufferIndex];
	}
#pragma endregion

#pragma region スワップチェインの定義
	DXGI_SWAP_CHAIN_DESC1 desc{};
	desc.BufferCount = m_iBackBufferSize;// バッファの数
	desc.Width = width;
	desc.Height = height;
	desc.Format = format;// バックバッファのフォーマット
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;//	レンダーターゲットの場合はこれ
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
#pragma endregion

#pragma region スワップチェイン作成
	ComPtr<IDXGISwapChain1>sc;
	auto hr = m_pDXGIFactory->CreateSwapChainForHwnd(
		m_pCommandQueue.Get(), hWnd, &desc, nullptr, nullptr, sc.GetAddressOf()
	);
	if (FAILED(hr)) { throw std::runtime_error("CreateSwapChainForHwnd Failed."); }
#pragma endregion
	//As関数で引数の型(スワップチェイン)のインターフェースを取得
	hr = sc.As(&m_pSwapChain);
	if (FAILED(hr)) { throw std::runtime_error("Obtaining IDXGISwapChain 4 Interface Failed."); }
#pragma region RT
	// レンダーターゲット(RT)を作ります
	{
		// RTのデスクリプターを作成
		// VRAMにあるリソースはただのメモリの塊。
		// そこでデスクリプターを使ってデータの種類やメモリ載せておく。
		// GPUはデスクリプター使ってリソースの見方を知る。
		// で！デスクリプターヒープはそのデスクリプターを記録しておくメモリを確保する
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//リソースの種類
		desc.NumDescriptors = m_iBackBufferSize;//RTの数だけメモリ確保
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//RTの場合はこれ

		auto hr = m_pDevice->CreateDescriptorHeap(
			&desc, IID_PPV_ARGS(m_pRTVDescriptorHeap.ReleaseAndGetAddressOf())
		);
		if (FAILED(hr)) { throw std::runtime_error("CreateDescriptorHeap Failed."); }
		// メモリのサイズを取得
		// メモリにあるデスクリプターにアクセスするときに使います
		m_RTVDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	for (std::uint32_t i = 0; i < m_iBackBufferSize; ++i)
	{
		// スワップチェインからバックバッファを取得してくる
		auto hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_RenderTargets[i].GetAddressOf()));
		if (FAILED(hr)) { throw std::runtime_error("IDGXSwapChain::GetBuffer Failed"); }
		m_RenderTargets[i]->SetName(L"Device::RenderTarget");
		// レンダーターゲットビューのデスクリプターを作る
		// この内容がデスクリプターヒープで用意したメモリに書き込まれる
		D3D12_RENDER_TARGET_VIEW_DESC desc{};
		desc.Format = m_dfBackBufferFormat;//RTのフォーマット(バックバッファと同一)
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;//2次元テクスチャ

		// 先に作ったRTのデスクリプタヒープから書き込むアドレスを求める
		CD3DX12_CPU_DESCRIPTOR_HANDLE RTVDescriptor(
			m_pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), i, m_RTVDescriptorSize);
		//	これでGPUがRTにアクセスできるようになるよ
		m_pDevice->CreateRenderTargetView(m_RenderTargets[i].Get(), &desc, RTVDescriptor);
		// スワップチェインから使っているバックバッファのインデクスをもらっておく
		m_iBackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	}
#pragma endregion
}
