#include "Device.hpp"

namespace dxapp {

Device::Device() {}

Device::~Device() {
  // GPUの処理が終わってるいるのを確認してから終わるようにする
  WaitForGPU();

#if _DEBUG
  {
    // デバッグビルドのときだけD3Dのデバッグ機能を使てみるよ
    // D3Dの生きてるオブジェクトを確認できる
    ComPtr<ID3D12DebugDevice> debugInterface;
    if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&debugInterface)))) {
      debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL |
                                              D3D12_RLDO_IGNORE_INTERNAL);
    }
  }
#endif
}

void Device::Initialize(HWND hWnd, std::uint32_t width, std::uint32_t height) {
  CreateDevice();

  // 描画命令を生成・積み込みに使うオブジェクトを作ります
  // ここで作る3種類のオブジェクトを組み合わせることで描画ができるようになります

  // キューの作成
  {
    // コマンドリストを積むキューの設定
    // D3D12_COMMAND_LIST_TYPE_DIRECTは描画命令で使えるやつ
    D3D12_COMMAND_QUEUE_DESC desc{};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;  // 詰めるリストの種類
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    // キューの作成
    // D3D12_COMMAND_LIST_TYPE_DIRECTのキューには
    // LIST_TYPE_DIRECTとLIST_TYPE_BUNDLEのコマンドを積むことができる。
    // 今回はLIST_TYPE_DIRECTだけやるよ。BUNDLEについては今後に期待。
    auto hr = device_->CreateCommandQueue(
        &desc,
        IID_PPV_ARGS(commandQueue_.ReleaseAndGetAddressOf()));  // 受け取る変数
    if (FAILED(hr)) {
      throw std::runtime_error("Device::CreateCommandQueue Failed");
    }
    commandQueue_->SetName(L"Device::ID3D12CommandQueue");
  }

  // コマンドリストを作るにはコマンドリストの種類に応じたメモリアロケータがいる
  // コマンド生成時にアロケータがメモリを割り当てていくよ
  {
    for (auto& alloc : commandAllocators_) {
      auto hr = device_->CreateCommandAllocator(
          D3D12_COMMAND_LIST_TYPE_DIRECT,  // 生成するコマンドリストの種類を指定
          IID_PPV_ARGS(alloc.ReleaseAndGetAddressOf()));  // 受け取る変数
      if (FAILED(hr)) {
        throw std::runtime_error("Device::CreateCommandAllocator Failed");
      }
      alloc->SetName(L"Device::ID3D12CommandAllocator");
    }

    auto hr = device_->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(subCommandAllcator_.ReleaseAndGetAddressOf()));
    if (FAILED(hr)) {
      throw std::runtime_error("Device::CreateCommandAllocator Failed");
    }
    subCommandAllcator_->SetName(L"Device::subCommandAllcator_");
  }

  {
    // コマンドリスト作る
    auto hr = device_->CreateCommandList(
        0,                               // 基本は0
        D3D12_COMMAND_LIST_TYPE_DIRECT,  // 通常の描画にはDIRECTをつかう
        commandAllocators_[0].Get(),  // メモリを確保してくれるアロケータ
        nullptr,                      // nullでOK
        IID_PPV_ARGS(
            graphicsCommandList_.ReleaseAndGetAddressOf()));  // 受け取る変数

    if (FAILED(hr)) {
      throw std::runtime_error("Device::CreateCommandList Failed");
    }
    graphicsCommandList_->SetName(L"Device::CreateCommandList");

    // コマンドリストは使う前に必ずCloseする必要がある！忘れないでね
    graphicsCommandList_->Close();
  }

  // フェンスの作成
  CreateFence();

  // スワップチェイン作成
  CreateSwapChain(hWnd, width, height, backbufferFormat_);

  // SRVとサンプラのデスクリプタサイズを設定
  srvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  samplerDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

  // レンダーターゲットの設定に使うビューポートとシザーの設定
  screenViewport_.Width = static_cast<FLOAT>(width);
  screenViewport_.Height = static_cast<FLOAT>(height);
  screenViewport_.TopLeftX = 0.0f;
  screenViewport_.TopLeftY = 0.0f;
  screenViewport_.MinDepth = D3D12_MIN_DEPTH;
  screenViewport_.MaxDepth = D3D12_MAX_DEPTH;

  scissorRect_.left = 0;
  scissorRect_.top = 0;
  scissorRect_.right = width;
  scissorRect_.bottom = height;

  // デプスバッファ
  {
    // デスクリプタサイズ
    dsvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // ヒープのデスクリプタ
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.NumDescriptors = 1;                     // 1でよい
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;  // タイプはこれ

    // ヒープ生成
    device_->CreateDescriptorHeap(
        &dsvHeapDesc,
        IID_PPV_ARGS(dsvDescriptorHeap_.ReleaseAndGetAddressOf()));

    // デプスバッファになるテクスチャのデスクリプタ
    D3D12_RESOURCE_DESC desc{CD3DX12_RESOURCE_DESC::Tex2D(
        depthBufferFormat_,  // テクスチャフォーマット
        width,  // テクスチャ幅(ここではレンダーターゲットと同じサイズ)
        height, // テクスチャ高さ(ここではレンダーターゲットと同じサイズ)
        1, 0, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)};  // テクスチャをデプスバッファに使うためのフラグ設定


    // デプスバッファをクリア（テクスチャを塗りつぶす）
    // するときの設定
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = depthBufferFormat_;  // デプスバッファフォーマット
    clearValue.DepthStencil.Depth = 1.0f;  // デプスは1.0
    clearValue.DepthStencil.Stencil = 0;   // ステンシルは0

    // ヒープの種類を設定
    CD3DX12_HEAP_PROPERTIES depthHeapProp{D3D12_HEAP_TYPE_DEFAULT};

    // デプスバッファテクスチャ生成
    device_->CreateCommittedResource(
        &depthHeapProp, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
        IID_PPV_ARGS(depthStencil_.ReleaseAndGetAddressOf()));

    // 上で作ったリソースからビューを作る
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = depthBufferFormat_;  // デプスバッファのフォーマット
    dsvDesc.ViewDimension =
        D3D12_DSV_DIMENSION_TEXTURE2D;  // ビューのデータの見方
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    // ビュー生成
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(
        dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
    device_->CreateDepthStencilView(
        depthStencil_.Get(), &dsvDesc, handle);
  }
}

void Device::CreateDevice() {
  HRESULT hr = S_FALSE;

#if _DEBUG
  {
    // デバッグビルドのときだけD3Dのデバッグ機能を有効にするよ
    ComPtr<ID3D12Debug> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
      debug->EnableDebugLayer();
    } else {
      // なんかエラーメッセージとか出したほうがいい気がするよ
    }

    // デバッグ機能の設定
    ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
    if (SUCCEEDED(DXGIGetDebugInterface1(
            0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf())))) {
      dxgiFactoryFlags_ = DXGI_CREATE_FACTORY_DEBUG;

      dxgiInfoQueue->SetBreakOnSeverity(
          DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
      dxgiInfoQueue->SetBreakOnSeverity(
          DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
    }
  }
#endif

  // ファクトリの作成
  // この後につづくアダプタやスワップチェインを作成するオブジェクトです。
  hr = CreateDXGIFactory2(dxgiFactoryFlags_,
                          IID_PPV_ARGS(dxgiFactory_.ReleaseAndGetAddressOf()));
  if (FAILED(hr)) {
    throw std::runtime_error("CreateDXGIFactory2 Failed");
  }

  // アダプタ(Windowsから見えるGPUっぽいやつ)作成
  ComPtr<IDXGIAdapter1> adapter = nullptr;
  for (UINT i = 0;; ++i) {
    // 使えるアダプタを順番調べる
    if (DXGI_ERROR_NOT_FOUND ==
        dxgiFactory_->EnumAdapters1(i, adapter.ReleaseAndGetAddressOf())) {
      // DXGI_ERROR_NOT_FOUNDはアダプタがない状態
      break;
    }

    // アダプタの情報を取得してゲームで使えるかかチェックしていくよ
    DXGI_ADAPTER_DESC1 desc = {};
    hr = adapter->GetDesc1(&desc);
    if (FAILED(hr)) {
      throw std::runtime_error("IDXGIAdapter1::GetDesc1 Failed");
    }

    // ソフトウェアアダプタ(CPU処理)は遅すぎるので使わないよ
    if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) {
      continue;
    }

    // アダプタがD3D12を使えるかチェックする
    // ためしにアダプタからデバイスを作ってみる
    if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                    _uuidof(ID3D12Device), nullptr))) {
      // D3D12が使えるアダプタがあったのでループ抜ける
      break;
    }
  }

  //#if _DEBUG  // リリースでも使う
  if (!adapter) {
    // DEBUGビルド時だけWARPアダプタでの動作を許可
    // WARPはソフトウェアアダプタよりずっと高速だがゲームを動かすのはつらい
    if (UseWarpAdapter_ && FAILED(dxgiFactory_->EnumWarpAdapter(IID_PPV_ARGS(
                               adapter.ReleaseAndGetAddressOf())))) {
      throw std::runtime_error("WARP adapter not found");
    }
  }
  //#endif

  if (!adapter) {
    // D3D12が使えるアダプターがないのであった
    throw std::runtime_error("D3D12 device not found");
  }

  // 見つけたアダプタからデバイス(このアプリケーションだけで使える仮想のGPU。的なイメージ)を作る
  hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                         IID_PPV_ARGS(device_.ReleaseAndGetAddressOf()));
  if (FAILED(hr)) {
    throw std::runtime_error("D3D12CreateDevice Failed");
  }
  device_->SetName(L"Device::ID3D12Device");
}

void Device::CreateFence() {
  // フェンスオブジェクトを作ります
  auto hr = device_->CreateFence(
      fenceValues_[backBufferIndex_],  // フェンスの初期値
      D3D12_FENCE_FLAG_NONE,  // 通常はD3D12_FENCE_FLAG_NONEでよい
      IID_PPV_ARGS(fence_.ReleaseAndGetAddressOf()));  // 受け取る変数
  if (FAILED(hr)) {
    throw std::runtime_error("Device::CreateFence Failed");
  }
  fence_->SetName(L"Device::ID3D12Fence");

  // 次回のフェンス値を設定
  fenceValues_[backBufferIndex_]++;

  // GPUの処理が終わるの待機するために使います
  fenceEvent_.Attach(
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
  if (!fenceEvent_.IsValid()) {
    throw std::runtime_error("CreateEventEx Failed");
  }
}

void Device::CreateSwapChain(HWND hWnd, std::uint32_t width,
                             std::uint32_t height, DXGI_FORMAT format) {
  WaitForGPU();

  // レンダーターゲットテクスチャとフェンス値をきれいにしておく
  for (int i = 0; i < MaxBackBufferSize_; i++) {
    renderTargets_[i].Reset();
    fenceValues_[i] = fenceValues_[backBufferIndex_];
  }

  // スワップチェインの定義を記述
  {
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.BufferCount = backBufferSize_;  // バッファの数
    desc.Width = width;                  // バックバッファの幅
    desc.Height = height;                // バックバッファの高さ
    desc.Format = format;  // バックバッファのフォーマット
    desc.BufferUsage =
        DXGI_USAGE_RENDER_TARGET_OUTPUT;  // レンダーターゲットの場合はこれ
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    desc.Flags = 0;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    // スワップチェインの作成
    ComPtr<IDXGISwapChain1> sc;
    auto hr = dxgiFactory_->CreateSwapChainForHwnd(
        commandQueue_.Get(), hWnd, &desc, nullptr, nullptr, sc.GetAddressOf());
    if (FAILED(hr)) {
      throw std::runtime_error("CreateSwapChainForHwnd Failed!");
    }

    // As関数で引数の型のインタフェースを取得(してみる)
    hr = sc.As(&swapChain_);
    if (FAILED(hr)) {
      // 失敗する場合もあるよ
      throw std::runtime_error("Obtaining IDXGISwapChain 4 Interface Failed");
    }
  }

  // レンダーターゲット(RT)を作ります
  {
    // RTのデスクリプターを作成
    // VRAMにあるリソースはただのメモリの塊。
    // そこでデスクリプターを使ってデータの種類やメモリ載せておく。
    // GPUはデスクリプター使ってリソースの見方を知る。
    // で！デスクリプターヒープはそのデスクリプターを記録しておくメモリを確保する
    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;  // リソースの種類
    desc.NumDescriptors = backBufferSize_;  // RTの数だけのメモリを確保
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;  //レンダーターゲットはこれ

    // descの内容でメモリを動的に確保してもらう
    auto hr = device_->CreateDescriptorHeap(
        &desc, IID_PPV_ARGS(rtvDescriptorHeap_.ReleaseAndGetAddressOf()));
    if (FAILED(hr)) {
      throw std::runtime_error("CreateDescriptorHeap Failed");
    }
    // メモリのサイズを取得
    // メモリにあるデスクリプターにアクセスするときに使います
    rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  }

  // バックバッファの数だけレンダーターゲットを作成
  for (std::uint32_t i = 0; i < backBufferSize_; i++) {
    // スワップチェインからバックバッファを取得してくる
    auto hr = swapChain_->GetBuffer(
        i, IID_PPV_ARGS(renderTargets_[i].GetAddressOf()));
    if (FAILED(hr)) {
      throw std::runtime_error("IDXGISwapChain::GetBuffer Failed");
    }
    renderTargets_[i]->SetName(L"Device::RenderTarget");

    // レンダーターゲットビューのデスクリプターを作る
    // この内容がデスクリプターヒープで用意したメモリに書き込まれる
    D3D12_RENDER_TARGET_VIEW_DESC desc{};
    desc.Format = backbufferFormat_;  // RTのフォーマット(バックバッファと同一)
    desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;  // 2次元のテクスチャ

    // 先に作ったRTのデスクリプタヒープから書き込むアドレスを求める
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
        rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(), i,
        rtvDescriptorSize_);

    // これでGPUがRTにアクセスできるようになるよ
    device_->CreateRenderTargetView(renderTargets_[i].Get(), &desc,
                                    rtvDescriptor);

    // スワップチェインから使っているバックバッファのインデクスをもらっておく
    backBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();
  }
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>
Device::CreateNewGraphicsCommandList() {
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cl;

  auto hr = device_->CreateCommandList(
      0, D3D12_COMMAND_LIST_TYPE_DIRECT, subCommandAllcator_.Get(), nullptr,
      IID_PPV_ARGS(cl.ReleaseAndGetAddressOf()));

  if (FAILED(hr)) {
    throw std::runtime_error("Device::CreateNewGraphicsCommandList Failed");
  }
  cl->SetName(L"Device::CreateNewGraphicsCommandList");

  return cl;
}

void Device::WaitForGPU() {
  if (commandQueue_ && fence_ && fenceEvent_.IsValid()) {
    // 現在のフェンス値をローカルにコピー
    // ローカルにコピーしたほうが速度面で有利とのうわさ(検証してない)
    auto currentValue = fenceValues_[backBufferIndex_];

    // キューに積んだコマンドの実行完了を待ちます

    // CommandQueue::Signal(フェンス, 更新された時の値)をすると
    // キューの実行が終わったときにフェンスの中身が第2引数の値に更新される
    if (SUCCEEDED(commandQueue_->Signal(fence_.Get(), currentValue))) {
      // SetEventOnCompletionはfence_が持っている値が、currentValueに
      // 更新されたときにイベントが飛んでくるように設定する
      if (SUCCEEDED(
              fence_->SetEventOnCompletion(currentValue, fenceEvent_.Get()))) {
        // ここで待ちます
        // 無限ループみたいなやり方でも待てるけど、CPUを無駄に使うのでお勧めしない
        WaitForSingleObjectEx(fenceEvent_.Get(), INFINITE, FALSE);

        // 次回の処理のためにフェンス値を増やしておく
        fenceValues_[backBufferIndex_]++;
      }
    }
  }
}

void Device::WaitForRenderingCompletion() {
  // キューにシグナルを送る
  const auto currentValue = fenceValues_[backBufferIndex_];
  commandQueue_->Signal(fence_.Get(), currentValue);

  // バックバッファが切り替わっているので新しいインデックスをもらう
  backBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();

  // 処理速度によってはここに到達した時点で描画が終わりフェンス値が更新されている可能性もある
  // その状態でWaitForSingleObjectExをすると無限に待つことになるぞ。
  // そうならないようにGetCompletedValueでフェンスの現在値を確認する。
  if (fence_->GetCompletedValue() < fenceValues_[backBufferIndex_]) {
    // まだ描画が終わっていないので待つ必要がある
    fence_->SetEventOnCompletion(fenceValues_[backBufferIndex_],
                                 fenceEvent_.Get());
    WaitForSingleObjectEx(fenceEvent_.Get(), INFINITE, FALSE);
  }
  // 次のフレームのためにフェンス値更新
  fenceValues_[backBufferIndex_] = currentValue + 1;
}

void Device::Present() {
  D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
      renderTargets_[backBufferIndex_].Get(),
      D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
  graphicsCommandList_->ResourceBarrier(1, &barrier);

  // コマンドリストはCloseしておかないと実行できませんよ
  graphicsCommandList_->Close();

  // ここでやっとGPUに描画処理をさせる
  // ExecuteはID3D12CommandListじゃないとダメなのでキャストしてるだけ
  auto cl = reinterpret_cast<ID3D12CommandList* const*>(
      graphicsCommandList_.GetAddressOf());

  // キューに積まれたコマンドを実行する
  commandQueue_->ExecuteCommandLists(
      1,  // キューには複数のリストが詰めるので実行するリスト数を指定
      cl);

  // スワップチェインの内容を切り替える
  swapChain_->Present(
      1,  // バックバッファを切り替えにVSyncを何度待つか。1で1回まつ
      0);  // とりあえず0でよい

  // しかしExecuteCommandLists / PresentもGPUに「働け！」と指示しているだけ。
  // つまり非同期実行なので、描画がおわるのをまって次回のフレームに進む必要がある
  WaitForRenderingCompletion();
}

void Device::ResetCommandList() {
  // アロケータとコマンドリストをリセットして前の内容を忘れるよ
  commandAllocators_[backBufferIndex_]->Reset();
  graphicsCommandList_->Reset(commandAllocators_[backBufferIndex_].Get(),
                              nullptr);
}

void Device::PrepareRendering() {
  ResetCommandList();
  D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
      renderTargets_[backBufferIndex_].Get(), D3D12_RESOURCE_STATE_PRESENT,
      D3D12_RESOURCE_STATE_RENDER_TARGET);
  // リソースバリアを設定
  graphicsCommandList_->ResourceBarrier(1, &barrier);
}

ID3D12Device* Device::device() const { return device_.Get(); }

ID3D12GraphicsCommandList* Device::graphicsCommandList() const {
  return graphicsCommandList_.Get();
}

ID3D12CommandQueue* Device::commandQueue() const { return commandQueue_.Get(); }

CD3DX12_CPU_DESCRIPTOR_HANDLE Device::currentRenderTargetView() const {
  return CD3DX12_CPU_DESCRIPTOR_HANDLE(
      rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
      backBufferIndex_, rtvDescriptorSize_);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Device::depthStencilView() const {
  return CD3DX12_CPU_DESCRIPTOR_HANDLE(
      dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
}

D3D12_VIEWPORT Device::screenViewport() const { return screenViewport_; }

D3D12_RECT Device::scissorRect() const { return scissorRect_; }

}  // namespace dxapp
