#include "pch.h"
#include "Application.hpp"
#include "Device.hpp"
#include "SimplePolygon.hpp"

dxapp::Application::Application() {}

dxapp::Application::~Application()
{
	Teardown();
}

void dxapp::Application::Setup(HWND hWnd, std::uint32_t width, std::uint32_t height)
{
	m_hWnd = hWnd;
	m_pDevice = std::make_unique<DX12::Device>();
	m_pDevice->Setup(hWnd, width, height);
	m_pSimplePolygon = std::make_unique<SimplePolygon>();
	m_pSimplePolygon->Setup(m_pDevice->GetDevice());
}

void dxapp::Application::Execute()
{
	Update();
	Render();
}

void dxapp::Application::Teardown()
{
	m_pDevice->WaitForGPU();
}

void dxapp::Application::Update()
{
}

void dxapp::Application::Render()
{
	//描画処理
	m_pDevice->PrepareRendering();
	ClearRTV();
	m_pSimplePolygon->MakeCommandList(m_pDevice->GetGraphicsCommandList());
	//描画更新命令
	m_pDevice->Present();
}

void dxapp::Application::ClearRTV()
{
	// レンダーターゲット(このフレームで描画に使用するバッファ)に描画してある
	// 内容をクリア(塗りつぶすイメージ)します
	auto commandList = m_pDevice->GetGraphicsCommandList();

	// フレームごとにバックバッファが切り替るので、このフレームでのレンダーターゲットをもらう
	auto RTV = m_pDevice->GetCurrentRTV();
	// コマンドリストにこのフレームでのレンダーターゲットを設定する
	commandList->OMSetRenderTargets(
		1,			//設定するRTの数。とりま1枚
		&RTV,	//RTハンドル
		FALSE,	//とりまFALSE
		nullptr	//とりまヌルぽでOK
	);
	// 指定したレンダーターゲットを、特定の色で塗りつぶし
	commandList->ClearRenderTargetView(
		RTV,										//塗りつぶすRTV
		DirectX::Colors::CadetBlue,		//塗りつぶし色
		0,											//０でおk
		nullptr									//ヌルぽでおk
	);
	// これはレンダーターゲットのクリアとは直接関係はないのだけど
	// 記述しておくのにはちょうどいいのでここに入れます。
	auto vp = m_pDevice->GetScreeViewport();
	commandList->RSSetViewports(1, &vp);

	auto scissorRect = m_pDevice->GetScissorRect();
	commandList->RSSetScissorRects(1, &scissorRect);
}
