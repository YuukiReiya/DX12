#include "Application.hpp"

#include "Device.hpp"
#include "TextureManager.hpp"
#include "Utility.hpp"
#include "Scene.hpp"

#include "External/StepTimer.h"
#include "External/WICTextureLoader12.h"


namespace dxapp {
using namespace DirectX;

Application::Application() {}
Application::~Application() { Terminate(); }

void Application::Initialize(HWND hWnd, std::uint32_t screenWidth,
                             std::uint32_t screenHeight) {
  hWnd_ = hWnd;
  device_ = std::make_unique<dxapp::Device>();

  // 各種デスクリプタの作成数を適当に決める
  std::array<uint32_t, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> descNums{ 1000, 100, 100, 100 };
  device_->Initialize(hWnd, screenWidth, screenHeight, descNums);

   // キーボードとマウス入力監視オブジェクト
  // この二つはクラス内でシングルトン化している
  keyboard_ = std::make_unique<Keyboard>();
  mouse_ = std::make_unique<Mouse>();
  mouse_->SetWindow(hWnd);

  // imgui
  // Imgui初期化
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  // ImGuiをWindowsで動かすための初期化
  ImGui_ImplWin32_Init(hWnd);

  auto handle = device_->viewDescriptorHeap()->Allocate();
  auto cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(handle.cpuHandle, handle.index, handle.allocator->incrementSize());
  auto gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(handle.gpuHandle, handle.index, handle.allocator->incrementSize());
  // ImGuiをD3D12で動かすための初期化
  ImGui_ImplDX12_Init(
    device_->device(),
    device_->backBufferSize(),
    device_->currentRenderTarget()->GetDesc().Format,
    device_->viewDescriptorHeap()->heap(),
    cpuHandle, gpuHandle);

  // テクスチャーマネージャー生成
  Singleton<TextureManager>::Create();
  Singleton<TextureViewManager>::Create();

// ダミー用テクスチャロード
  Singleton<TextureManager>::instance().LoadWICTextureFromFile(
      device_.get(), L"Assets/uv_checker.png", "uv_checker");
  // ダミー用テクスチャSRV
  Singleton<TextureViewManager>::instance().CreateView(
      device_.get(),
      Singleton<TextureManager>::instance().texture("uv_checker").Get());

  scene_ = std::make_unique<Scene>();
  scene_->Initialize(device_.get());
}

void Application::Terminate() {
  SingletonFinalizer::Finalize();
  device_->WaitForGPU();

  ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
}

void Application::Run() {

  timer_.Tick([&]() { Update(timer_); });

  Render();
}

void Application::Update(const DX::StepTimer& timer) {
  auto deltaTime = static_cast<float>(timer.GetElapsedSeconds());

  // シーンアップデート
  scene_->Update(deltaTime);
}

void Application::Render() {
  device_->PrepareRendering();

  // シーンレンダリング
  scene_->Render(device_.get());

  device_->Present();
}

void Application::ClearRenderTarget() {
  // レンダーターゲット(このフレームで描画に使用するバッファ)に描画してある
  // 内容をクリア(塗りつぶす)します

  auto commandList = device_->graphicsCommandList();

  // フレームごとにバックバッファが切り替るので、このフレームでのレンダーターゲットをもらう
  auto rtv = device_->currentRenderTargetView();

  // 指定したレンダーターゲットを、特定の色で塗りつぶし
  commandList->ClearRenderTargetView(
      rtv,                //塗りつぶしたいレンダーターゲット
      Colors::CadetBlue,  // 塗る色。Colorsやfloat[4]で指定できるよ
      0,                  // 0でOK
      nullptr);           // nullでOK

  // デプスバッファクリア
  auto dsv = device_->depthStencilView();
  commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0,
                                     nullptr);

  // コマンドリストにこのフレームでのレンダーターゲットを設定する
  commandList->OMSetRenderTargets(
      1,  // 設定するレンダーターゲットの数。とりあえず1枚
      &rtv,   // レンダーターゲットへのハンドル
      FALSE,  // とりあえずFALSE
      &dsv);  // デプスステンシルビュー

  // これはレンダーターゲットのクリアとは直接関係はないのだけど
  // 記述しておくのにはちょうどいいのでここに入れます。
  auto viewport = device_->screenViewport();
  commandList->RSSetViewports(1, &viewport);

  auto scissorRect = device_->scissorRect();
  commandList->RSSetScissorRects(1, &scissorRect);
}
}  // namespace dxapp
