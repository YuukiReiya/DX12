#include "Application.hpp"
#include "Device.hpp"
#include "Utility.hpp"

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

  // アプリごとの初期化
}

void Application::Terminate() { device_->WaitForGPU(); }

void Application::Run() {
  timer_.Tick([&]() { Update(timer_); });
  Render();
}

void Application::Update(const DX::StepTimer& timer) {}

void Application::Render() {
  // ここで描画処理をする
  device_->PrepareRendering();
  ClearRenderTarget();
  // 描画処理ここｋら

  // 描画処理ここまで
  device_->Present();
}

void Application::ClearRenderTarget() {
  // レンダーターゲット(このフレームで描画に使用するバッファ)に描画してある
  // 内容をクリア(塗りつぶすイメージ)します

  auto commandList = device_->graphicsCommandList();

  // フレームごとにバックバッファが切り替るので、このフレームでのレンダーターゲットをもらう
  auto rtv = device_->currentRenderTargetView();

  // コマンドリストにこのフレームでのレンダーターゲットを設定する
  commandList->OMSetRenderTargets(
      1,  // 設定するレンダーターゲットの数。とりあえず1枚
      &rtv,      // レンダーターゲットへのハンドル
      FALSE,     // とりあえずFALSE
      nullptr);  // とりあえずnull

  // 指定したレンダーターゲットを、特定の色で塗りつぶし
  commandList->ClearRenderTargetView(
      rtv,                //塗りつぶしたいレンダーターゲット
      Colors::CadetBlue,  // 塗る色。Colorsやfloat[4]で指定できるよ
      0,                  // 0でOK
      nullptr);           // nullでOK

  // これはレンダーターゲットのクリアとは直接関係はないのだけど
  // 記述しておくのにはちょうどいいのでここに入れます。
  auto viewport = device_->screenViewport();
  commandList->RSSetViewports(1, &viewport);

  auto scissorRect = device_->scissorRect();
  commandList->RSSetScissorRects(1, &scissorRect);
}
}  // namespace dxapp
