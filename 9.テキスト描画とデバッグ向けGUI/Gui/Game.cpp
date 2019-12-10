//
// Game.cpp
//

#include "Game.h"
#include "pch.h"

#include "TextRenderer.h"

extern void ExitGame();

using namespace DirectX;
using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false) {
  m_deviceResources = std::make_unique<DX::DeviceResources>();
  m_deviceResources->RegisterDeviceNotify(this);
}

Game::~Game() {
#pragma region imgui
	// ImGuiの終了処理
	// この順番で呼ぶはず。一応D3D12のデバイスが削除される前に呼んでおこう
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#pragma endregion
  if (m_deviceResources) {
    m_deviceResources->WaitForGpu();
  }
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height) {
	m_deviceResources->SetWindow(window, width, height);

	m_deviceResources->CreateDeviceResources();
	CreateDeviceDependentResources();

	m_deviceResources->CreateWindowSizeDependentResources();
	CreateWindowSizeDependentResources();

	// TODO: Change the timer settings if you want something other than the
	// default variable timestep mode. e.g. for 60 FPS fixed timestep update
	// logic, call:
	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(1.0 / 60);

#pragma region TextRenderer
	textRenderer_ = std::make_unique<TextRenderer>(m_deviceResources->GetBackBufferCount());
	textRenderer_->Initialize(window, m_deviceResources.get());

	//	黄色いブラシ
	{
		textRenderer_->CreateColorBrush("Yellow", D2D1::ColorF::Yellow);
	}
	//	赤いブラシ
	{
		textRenderer_->CreateColorBrush("Red", D2D1::ColorF::Red);
	}
	//	LargeBoldFont : 大きい太文字のフォーマット
	{
		textRenderer_->CreateTextFormat("LargeBoldFont", L"メイリオ", 40.0f, DWRITE_FONT_WEIGHT_BOLD);
	}

	//計算済みレイアウトのテキストを追加。フォーマットはデフォルト
	textRenderer_->CreateTextLayoutWithDefaultFormat("SleepText", L"今日は眠いですね", 400.0f, 200.0f);

	//計算済みレイアウトのテキスト追加。太字のフォーマット
	textRenderer_->CreateTextLayout("HelloText", L"おはようございます", 100.0f, 400.0f, "LargeBoldFont");
#pragma endregion

#pragma region imgui
	auto device = m_deviceResources->GetD3DDevice();
	// ImguiにSRVのデスクリプタヒープがいるのでつくるだけ
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc{
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2048,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
		device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(srvHeap_.GetAddressOf()));
	}

	// デスクリプタヒープをもらう。
   // 今回はほかには使ってないので0番目の固定で使いますよ
	auto size = device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle{
		srvHeap_->GetCPUDescriptorHandleForHeapStart(), 0, size };
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle{
		srvHeap_->GetGPUDescriptorHandleForHeapStart(), 0, size };
	// Imgui初期化
	// この順番で呼び出そう
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	// ImGuiをWindowsで動かすための初期化
	ImGui_ImplWin32_Init(window);
	// ImGuiをD3D12で動かすための初期化
	ImGui_ImplDX12_Init(
		device,                                   // D3D12デバイス
		m_deviceResources->GetBackBufferCount(),  // バックバッファ数
		DXGI_FORMAT_B8G8R8A8_UNORM,  // バックバッファフォーマット
		srvHeap_.Get(),              // デスクリプタヒープ
		cpuHandle, gpuHandle);  // imguiのフォントが使うsrvのハンドル
#pragma endregion
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick() {
#pragma region TextRenderer
	// テキストレンダラーのバッファをきれいにする
	// 各フレームでテキストを追加する前に呼んでおこう
	textRenderer_->Clear();
#pragma endregion

	m_timer.Tick([&]() { Update(m_timer); });

#pragma region TextRender_TestCode
	// とりあえずFPSとデルタタイムでも出してみる
	auto fps = std::to_wstring(static_cast<int>(m_timer.GetFramesPerSecond()));
	auto delta = std::to_wstring(m_timer.GetElapsedSeconds());
	textRenderer_->AddDrawTextWithBrush(L"FPS: " + fps + L" / " + delta, 0, 0,
		"Yellow");

	// 太字の描画
	textRenderer_->AddDrawTextWithFormat(L"こんにちは！", 400, 200,
		"LargeBoldFont");

	// レイアウトテキスト
	// 位置と色を変えてみる。文字列自体は変えられないけど色と表示位置は変更可能
	textRenderer_->AddDrawTextLayout("SleepText", 200, 600);
	textRenderer_->AddDrawTextLayoutWithBrush("SleepText", 200, 620, "Red");
	textRenderer_->AddDrawTextLayout("SleepText", 200, 640);
	textRenderer_->AddDrawTextLayoutWithBrush("SleepText", 200, 660, "Red");
	textRenderer_->AddDrawTextLayoutWithBrush("HelloText", 200, 0, "Yellow");
#pragma endregion

#pragma region imgui
	// 次の3行はフレーム開始時ImGuiを使う前に呼ぶこと
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
#pragma region imgui_01
	{
#pragma region imgui_02
		// ImGuiで操作する変数
		// Gameクラスのメンバを作るのがめんどいのでstaticで作っちゃった
		static float s_sliderValue{};
		static float s_windowColor[4]{};
		static float s_vec[3]{};
#pragma endregion
		// ImGui::Begin / EndでImGuiのウィンドウが1つ作れる
		ImGui::Begin("First ImGui");  // 文字列がウィンドウ名
		{
#pragma region imgui_02
			// 上から順にウィンドウ内に表示される

	// ImGui::Textは引数の文字列を表示します。
	// デフォルトのフォントだと日本語が出ないので注意だよ
			ImGui::Text("DirectX12");

			// ImGuiによるFPSの表示
			ImGui::Text("Framerate(avg) %.3f ms/frame (%.1f FPS)",
				1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			// ImGui::Buttonでボタン。引数はボタン名
			ImGui::Button("Don't work button");

			// 上のボタンは何もしないボタン
			// 下のようにボタンをifの式に使うと押された時の処理ができるよ
			if (ImGui::Button("Button")) {
				// ボタンが押下されたときの処理
				s_sliderValue = 100.0f;
			}

			// SliderFloatは名前の通りfloatデータをスライダー操作できる
			// IntとかFloat3とかもある
			ImGui::SliderFloat("Slider", &s_sliderValue, 0.0f, 100.0f);
			ImGui::SliderFloat3("Vector3", s_vec, -100.0f, 100.0f);

			// カラーピッカー。float4要素を色として操作
			ImGui::ColorPicker4("Second Window Color",  // Guiのラベル文字列
				s_windowColor);         // 操作するfloat型配列
#pragma endregion
		}
		ImGui::End();

#pragma region imgui_02
		// PushStyleColorでGuiの色を操作できる
	// ImVec4はImGuiが使うデータ型 float4要素
	// PushStyleColor / PopStyleColorペアの内側にあるGuiの色がかわる
		ImGui::PushStyleColor(
			ImGuiCol_TitleBgActive,  // ウィンドウのタイトル部分の色
			ImVec4(s_windowColor[0], s_windowColor[1], s_windowColor[2], 1.0f));
		ImGui::PushStyleColor(
			ImGuiCol_TitleBg,  // ウィンドウタイトルがフォアグラウンドの時の色
			ImVec4(s_windowColor[0], s_windowColor[1], s_windowColor[2], 1.0f));
#pragma endregion

		// SetNextWindowPos
		// 次のBegin/Endで出すウィンドウの位置
		ImGui::SetNextWindowPos(ImVec2(240, 20), ImGuiCond_Once);
		// SetNextWindowSize
		// 次のBegin/Endで出すウィンドウのサイズ
		ImGui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_Once);

		ImGui::Begin("Second Window");
		{
#pragma region imgui_02
			// カラーピッカーの色相版
			ImGui::ColorEdit4("Second Window Color", s_windowColor,
				ImGuiColorEditFlags_PickerHueWheel);

			// ウィンドウ内スクロール
			ImGui::BeginChild("Scrolling");
			for (int n = 0; n < 50; n++) {
				// フォーマット文字列まで作ってくれる神か・・・
				ImGui::Text("%04d: Some text", n);
			}
			ImGui::EndChild();
#pragma endregion
		}
	ImGui::End();
#pragma region imgui_02
	// PushStyleColorしたら、同じだけPopStyleColorしようね
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
#pragma endregion
	}
#pragma endregion
#pragma endregion
	Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer) {
  PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

  float elapsedTime = float(timer.GetElapsedSeconds());

  // TODO: Add your game logic here.
  elapsedTime;
  PIXEndEvent();
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render() {
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0) {
		return;
	}

	// Prepare the command list to render a new frame.
	m_deviceResources->Prepare();
	Clear();

	// まずは普通にD3D12のレンダリングをする
	auto commandList = m_deviceResources->GetCommandList();
#pragma region TextRenderの描画で消す
#if false
	{
		// このリソースバリアはあとで消します
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_deviceResources->GetRenderTarget(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		commandList->ResourceBarrier(1, &barrier);
	}
#endif // false
#pragma endregion
#pragma region imgui
	// imguiが使うデスクリプタヒープをセット
	commandList->SetDescriptorHeaps(1, srvHeap_.GetAddressOf());

	// 描画はなんと2行でOKだ
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
#pragma endregion

	commandList->Close();

	// D3D11on12の描画をする前に、それまでのD3D12のコマンドを実行しておく
	ID3D12CommandList* cl[] = { commandList };
	m_deviceResources->GetCommandQueue()->ExecuteCommandLists(1, cl);

#pragma region TextRenderer
	textRenderer_->Render();
#pragma endregion


	// Presentの中身はテキストレンダリングに合わせて次の修正がしてある
	//  ExecuteCommandLists、つまりD3D12の描画が終わった後にD2Dの描画をする必要がある
	//  Present関数ではExecuteCommandListsまで行っていたので、そこをコメントアウトした
	m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear() {
  auto commandList = m_deviceResources->GetCommandList();
  PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

  // Clear the views.
  auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
  auto dsvDescriptor = m_deviceResources->GetDepthStencilView();

  commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
  commandList->ClearRenderTargetView(rtvDescriptor, Colors::CornflowerBlue, 0,
                                     nullptr);
  commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH,
                                     1.0f, 0, 0, nullptr);

  // Set the viewport and scissor rect.
  auto viewport = m_deviceResources->GetScreenViewport();
  auto scissorRect = m_deviceResources->GetScissorRect();
  commandList->RSSetViewports(1, &viewport);
  commandList->RSSetScissorRects(1, &scissorRect);

  PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated() {
  // TODO: Game is becoming active window.
}

void Game::OnDeactivated() {
  // TODO: Game is becoming background window.
}

void Game::OnSuspending() {
  // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming() {
  m_timer.ResetElapsedTime();

  // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowMoved() {
  auto r = m_deviceResources->GetOutputSize();
  m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height) {
  if (!m_deviceResources->WindowSizeChanged(width, height)) return;

  CreateWindowSizeDependentResources();

  // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const {
  // TODO: Change to desired default window size (note minimum size is 320x200).
  width = 1024;
  height = 768;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources() {
  auto device = m_deviceResources->GetD3DDevice();

  // TODO: Initialize device dependent objects here (independent of window
  // size).
  device;
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources() {
  // TODO: Initialize windows-size dependent objects here.
}

void Game::OnDeviceLost() {
  // TODO: Add Direct3D resource cleanup here.
}

void Game::OnDeviceRestored() {
  CreateDeviceDependentResources();

  CreateWindowSizeDependentResources();
}
#pragma endregion
