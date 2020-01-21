﻿#pragma once

#include "DescriptorAllocator.hpp"

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
   * @brief CommandQueueを返す
   */
  ID3D12CommandQueue* commandQueue() const { return commandQueue_.Get(); };

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
  ID3D12Device* device() const { return device_.Get(); };

  /*!
   * @brief ID3D12GraphicsCommandListを返す
   */
  ID3D12GraphicsCommandList* graphicsCommandList() const { return graphicsCommandList_.Get(); };


  /*!
   * @brief コマンドアロケータオを帰すを返す
   */
  ID3D12CommandAllocator* currentGraphicsCommandAllocator() const { return commandAllocators_[backBufferIndex_].Get(); }

  /*!
   * @brief カレントレンダーターゲットを返す
   */
  ID3D12Resource* currentRenderTarget() const { return renderTargets_[backBufferIndex_].Get(); }

  /*!
   * @brief カレントレンダーターゲットへのビューを返す
   */
  CD3DX12_CPU_DESCRIPTOR_HANDLE currentRenderTargetView() const;

  /*!
   * @brief デバイスが持ってるデプスバッファを返す
   */
  ID3D12Resource* depthStencil() const { return  depthStencil_.Get(); };

  /*!
   * @brief デバイスが持ってるデプスバッファのビューを返す
   */
  CD3DX12_CPU_DESCRIPTOR_HANDLE depthStencilView() const;


  /*!
   * @brief デプスステンシルのクリアバリューを返す
   */
  D3D12_CLEAR_VALUE depthStencilClearValue() const { return dsClearValue_; }

  /*!
   * @brief レンダーターゲットのビューポートを返す
   */
  D3D12_VIEWPORT screenViewport() const { return screenViewport_; };

  /*!
   * @brief レンダーターゲットのシザーを返す
   */
  D3D12_RECT scissorRect() const { return scissorRect_; };

  /*!
   * @brief 初期化処理
   * @pram[in] hWnd  ウィンドウハンドル
   * @pram[in] width クライアントの幅
   * @pram[in] height クライアントの高さ
   * @pram[in] descNums SRV/CBV・SAMPLER・RTV・DSVのデスクリプタハンドル数
   */
  void Initialize(HWND hWnd, std::uint32_t width, std::uint32_t height, std::array<uint32_t, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> descNums);

  /*!
   * @brief キューのコマンドをGPUに実行させてスワップチェインを切り替える
   */
  void Present();

  /*!
   * @brief 今回フレームに実行するコマンドリストをキューに積む
   */
  void PushCommandList(ID3D12CommandList* commandList);

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
   * @brief 一時的に使用するグラフィックコマンドリストを生成して返す
   */
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>
  CreateOnetimeGraphicsCommandList();

  /*!
   * @brief SRV/CBV/UAVデスクリプタヒープを返す
   */
  DescriptorAllocator* viewDescriptorHeap() const { return viewDescriptorHeap_.get(); }


  /*!
   * @brief SAMPLERデスクリプタヒープを返す
   */
  DescriptorAllocator* samplerDescriptorHeap() const { return samplerDescriptorHeap_.get(); }


  /*!
   * @brief RTVデスクリプタヒープを返す
   */
  DescriptorAllocator* rtvDescriptorHeap() const { return rtvDescriptorHeap_.get(); }


  /*!
   * @brief DSVデスクリプタヒープを返す
   */
  DescriptorAllocator* dsvDescriptorHeap() const { return dsvDescriptorHeap_.get(); }

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

  //!  カレントフレームで実行するコマンドリストを格納しておく
  std::vector<ID3D12CommandList*> commandLists_{};

  //! 外部でコマンドリストが必要になったときに使用する
  ComPtr<ID3D12CommandAllocator> subCommandAllcator_{};

  //---------------------------------------------------------------
  // レンダーターゲットのクリアに使う
  //---------------------------------------------------------------
  //! レンダーターゲットのビューポート(サイズや位置など)
  D3D12_VIEWPORT screenViewport_{};
  //! 描画範囲(のようなもの)
  D3D12_RECT scissorRect_{};

  //---------------------------------------------------------------
  // デプスバッファ
  // カメラから見たピクセルのZ値を記録して、ピクセルの奥行き表現に使う
  // デバイスと紐づかせるべきではないかもしれないが、
  // デフォルトのデプスバッファとしてここで作成しておく
  //---------------------------------------------------------------
  //! デプスバッファのフォーマット
  //! 1ピクセル当たり32bitの精度をつかう
  DXGI_FORMAT depthBufferFormat_ = DXGI_FORMAT_D24_UNORM_S8_UINT;

  //! デプスバッファ本体
  //! イメージ的にはシーンを白黒で描画するかんじ
  //! 黒いほど手前。白いほど奥。
  Microsoft::WRL::ComPtr<ID3D12Resource> depthStencil_{};

  //!  デプスバッファのクリアバリュー
  D3D12_CLEAR_VALUE dsClearValue_{};

  // SRV/CBVは同じヒープ
  std::unique_ptr<DescriptorAllocator> viewDescriptorHeap_{ nullptr };
  // サンプラ用
  std::unique_ptr<DescriptorAllocator> samplerDescriptorHeap_{ nullptr };
  // RTVヒープ
  std::unique_ptr<DescriptorAllocator> rtvDescriptorHeap_{ nullptr };
  // DSVヒープ
  std::unique_ptr<DescriptorAllocator> dsvDescriptorHeap_{ nullptr };

  std::array<DescriptorHandle, MaxBackBufferSize_> defaultRtvHandle_{};
  DescriptorHandle defaultDsvHandle_{};
};
}  // namespace dxapp