#pragma once
namespace dxapp {

/*!
 * @brief デバイス、デバイス周りのオブジェクトを押し込めたクラス
 */
class Device {
 public:
  /*!
   * @brief コンストラクタ
   */
  Device();

  /*!
   * @brief デストラクタ
   */
  ~Device();

  // コピー・代入禁止
  Device(const Device&) = delete;
  Device& operator=(const Device&) = delete;

  /*!
   * @brief ID3D12GraphicsCommandListを返す
   */
  ID3D12CommandQueue* commandQueue() const;

  /*!
   * @brief srvDescriptorSizeを返す
   * @note SRV/CBV/UAVのデスクリプターは同じサイズ
   */
  UINT srvDescriptorSize() const { return srvDescriptorSize_; }

  /*!
   * @brief cbvDescriptorSizeを返す
   * @note SRV/CBV/UAVのデスクリプターは同じサイズ
   */
  UINT cbvDescriptorSize() const { return srvDescriptorSize_; }

  /*!
   * @brief uavDescriptorSizeを返す
   * @note SRV/CBV/UAVのデスクリプターは同じサイズ
   */
  UINT uavDescriptorSize() const { return srvDescriptorSize_; }

  /*!
   * @brief samplerDesctiptorSizeを返す
   */
  UINT samplerDesctiptorSize() const { return samplerDescriptorSize_; }

  /*!
   * @brief 現在のバックバッファインデックスを返す
   */
  std::uint32_t backBufferIndex() const { return backBufferIndex_; }

  /*!
   * @brief バックバッファの数を返す
   */
  std::uint32_t backBufferSize() const { return backBufferSize_; }

  /*!
   * @brief D3D12デバイスを返す
   */
  ID3D12Device* device() const;

  /*!
   * @brief ID3D12GraphicsCommandListを返す
   */
  ID3D12GraphicsCommandList* graphicsCommandList() const;

  /*!
   * @brief カレントレンダーターゲットへのビューを返す
   */
  CD3DX12_CPU_DESCRIPTOR_HANDLE currentRenderTargetView() const;

  /*!
   * @brief レンダーターゲットのビューポートを返す
   */
  D3D12_VIEWPORT screenViewport() const;

  /*!
   * @brief レンダーターゲットのシザーを返す
   */
  D3D12_RECT scissorRect() const;

  /*!
   * @brief 初期化処理
   * @pram[in] hWnd  ウィンドウハンドル
   * @pram[in] width クライアントの幅
   * @pram[in] height クライアントの高さ
   */
  void Initialize(HWND hWnd, std::uint32_t width, std::uint32_t height);

  /*!
   * @brief キューのコマンドをGPUに実行させてスワップチェインを切り替える
   */
  void Present();

  /*!
   * @brief コマンドリストをリセットする
   */
  void ResetCommandList();

  /*!
   * @brief 次回の描画処理ための準備をする
   */
  void PrepareRendering();

  /*!
   * @brief 描画処理が終わるのを待つ
   */
  void WaitForRenderingCompletion();

  /*!
   * @brief GPUの処理待ちをする
   */
  void WaitForGPU();

  /*!
   * @brief グラフィックコマンドリストを生成して返す
   */
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>
  CreateNewGraphicsCommandList();

 private:
  /*!
   * @brief D3D12デバイスの作成
   */
  void CreateDevice();

  /*!
   * @brief フェンスオブジェクト作成
   */
  void CreateFence();

  /*!
   * @brief スワップチェインの作成
   * @pram[in] hWnd ウィンドウハンドル
   * @pram[in] width レンダーターゲットの幅
   * @pram[in] height レンダーターゲットの高さ
   * @pram[in] format レンダーターゲットのテクスチャフォーマット
   */
  void CreateSwapChain(HWND hWnd, std::uint32_t width, std::uint32_t height,
                       DXGI_FORMAT format);
  //---------------------------------------------------------------
  // 定数や設定系
  //---------------------------------------------------------------
  // ComPtr長いのでエイリアス
  template <typename T>
  using ComPtr = Microsoft::WRL::ComPtr<T>;

  //! デバッグビルド時にWARPアダプタを使う
  static constexpr bool UseWarpAdapter_{true};
  //! バックバッファの最大値
  static constexpr std::uint32_t MaxBackBufferSize_{3};

  //---------------------------------------------------------------
  // 大事なD3D12デバイス
  //---------------------------------------------------------------
  //! D3Dの処理を司るデバイスオブジェクト
  ComPtr<ID3D12Device> device_{nullptr};

  //---------------------------------------------------------------
  // ファクトリー関連
  //---------------------------------------------------------------
  //! ファクトリオブジェクト
  ComPtr<IDXGIFactory4> dxgiFactory_{nullptr};  // デバイスを作るために必要
  //! ファクトリ作成のフラグ
  std::uint32_t dxgiFactoryFlags_{0};

  //---------------------------------------------------------------
  // スワップチェイン・バックバッファ・レンダーターゲット関連
  //---------------------------------------------------------------
  //! スワップチェインオブジェクト
  ComPtr<IDXGISwapChain4> swapChain_{nullptr};  // 画面への出力を処理してくれる

  //! 現在使っているバックバッファのインデックス
  std::uint32_t backBufferIndex_{0};

  //! 作成するバックバッファの数
  std::uint32_t backBufferSize_{2};

  //! レンダーターゲット(バックバッファ)テクスチャを保持する配列
  std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, MaxBackBufferSize_>
      renderTargets_{};

  //! バックバッファテクスチャのフォーマット。このサンプルでは初期設定で固定してます
  DXGI_FORMAT backbufferFormat_ =
      DXGI_FORMAT_R8G8B8A8_UNORM;  // 1ピクセルをRGBA各8bit使う

  //---------------------------------------------------------------
  // フェンス関連。CPU(ゲーム処理)とGPU(描画)の同期をとるためオブジェクト
  //---------------------------------------------------------------
  //! GPU処理が終わっているかを確認するために必要なフェンスオブジェクト
  ComPtr<ID3D12Fence> fence_{nullptr};
  //! フェンスに書き込む値、バックバッファと同じぶんを用意しておく
  std::array<UINT64, MaxBackBufferSize_> fenceValues_{};

  //! CPU、GPUの同期処理を楽にとるために使います
  Microsoft::WRL::Wrappers::Event fenceEvent_{};

  //---------------------------------------------------------------
  // コマンド関連
  //---------------------------------------------------------------
  //! GPUへの命令(コマンド)を蓄えるリスト
  //! いくつか種類があるが、ID3D12GraphicsCommandListは描画用
  ComPtr<ID3D12GraphicsCommandList> graphicsCommandList_{nullptr};

  //! コマンドを発行するためのアロケータ（メモリを確保してくれるひと）
  std::array<ComPtr<ID3D12CommandAllocator>, MaxBackBufferSize_>
      commandAllocators_{};

  //! コマンドリストはID3D12CommandQueueに積んでGPUに送るよ
  ComPtr<ID3D12CommandQueue> commandQueue_{nullptr};

  //! 外部でコマンドリストが必要になったときに使用する
  ComPtr<ID3D12CommandAllocator> subCommandAllcator_{};

  //---------------------------------------------------------------
  // レンダーターゲットのデスクリプタ
  // rtvは"R"ender "T"arget "View"の略。
  //---------------------------------------------------------------
  //! レンダーターゲットの設定(Descriptor)をVRAMに保存するのに使います
  ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_{nullptr};
  //! レンダーターゲットのデスクリプタオブジェクトのサイズ
  UINT rtvDescriptorSize_{0};

  //---------------------------------------------------------------
  // レンダーターゲットのクリアに使う
  //---------------------------------------------------------------
  //! レンダーターゲットのビューポート(サイズや位置など)
  D3D12_VIEWPORT screenViewport_{};
  //! 描画範囲(のようなもの)
  D3D12_RECT scissorRect_{};

  //---------------------------------------------------------------
  // シェーダーリソースの作成に必要
  //---------------------------------------------------------------
  //! SRV(主にテクスチャ）のデスクリプタサイズ
  UINT srvDescriptorSize_{};

  //! ダイナミックサンプラ(テクスチャの参照法)のデスクリプタサイズ
  UINT samplerDescriptorSize_{};
};
}  // namespace dxapp
