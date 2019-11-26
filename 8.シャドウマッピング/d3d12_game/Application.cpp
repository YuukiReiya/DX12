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
  device_->Initialize(hWnd, screenWidth, screenHeight);

  // テクスチャーマネージャー生成
  Singleton<TextureManager>::Create();

#pragma region add_1105
  // キーボードとマウス入力監視オブジェクト
  // この二つはクラス内でシングルトン化している
  keyboard_ = std::make_unique<Keyboard>();
  mouse_ = std::make_unique<Mouse>();
  mouse_->SetWindow(hWnd);

  scene_ = std::make_unique<Scene>();
  scene_->Initialize(device_.get());
#pragma endregion
}

void Application::Terminate() {
  SingletonFinalizer::Finalize();
  device_->WaitForGPU();
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
  ClearRenderTarget();

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
