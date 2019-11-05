#include "Application.hpp"
#include "Device.hpp"
#include "Utility.hpp"
#include "BasicShader.hpp"
#include "BufferObject.hpp"
#include "GeometoryMesh.hpp"
#include "TextureManager.hpp"

#include "External/StepTimer.h"
#include "External/WICTextureLoader12.h"

namespace dxapp {
using namespace DirectX;
using namespace API;

Application::Application() {}
Application::~Application() { Terminate(); }

void Application::Initialize(HWND hWnd, std::uint32_t screenWidth,
                             std::uint32_t screenHeight) {
  hWnd_ = hWnd;
  device_ = std::make_unique<dxapp::Device>();
  device_->Initialize(hWnd, screenWidth, screenHeight);

  // アプリごとの初期化
  auto dev = device_->device();

  // シェーダ生成
  m_BasicShader = std::make_unique<BasicShader>();
  m_BasicShader->Setup(dev);

  // テクスチャーマネージャー生成
  Singleton<TextureManager>::Create();

  // テクスチャロード
  // シングルトン化されているので instanceメソッドでどこからでも
  // インスタンスが取得できる

  // Assets/cat.pngはすでにプロジェクトに追加してある
  Singleton<TextureManager>::instance().LoadWICTextureFromFile(
	  device_.get(), L"Assets/cat.png", "cat");

  //-----------------------------------------------------------------
  // キューブのシェーダーリソースを作ります.作る順番に意味はないです
  //-----------------------------------------------------------------
  // まずはサンプラーを作ります
  // サンプラはほかのシェーダにも使いまわせるので独立したヒープ
  {
	  // サンプラー用のデスクリプタヒープを作る
	  D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
		  D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,  //ヒープタイプはサンプラ
		  1,                                   // 作るのは1個
		  D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,  // シェーダから見える設定
		  0 };                                         // 0でOK
	  dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_SamplerHeap));

	  // サンプラーの設定を作る
	  // テクスチャ補間の設定やテクスチャアドレッシング設定
	  D3D12_SAMPLER_DESC samplerDesc{};
	  // テクスチャフィルタ
	  // テクスチャのスケール時に線形補間。処理も軽いがそれほど奇麗ではない
	  samplerDesc.Filter =
		  D3D12_ENCODE_BASIC_FILTER(D3D12_FILTER_TYPE_LINEAR,  // 縮小時
			  D3D12_FILTER_TYPE_LINEAR,  // 拡大時
			  D3D12_FILTER_TYPE_LINEAR,  // mipmap
			  D3D12_FILTER_REDUCTION_TYPE_STANDARD);
	  // テクスチャアドレッシング
	  // UV座標は0～1.0の範囲で表現するが、uv座標がその範囲を超えた場合の処理
	  //   0   1
	  //  0 +---+
	  //    |   | これが通常この範囲の外はどうする？
	  //  1 +---+
	  //
	  //  D3D12_TEXTURE_ADDRESS_MODE_WRAPだとテクスチャがタイル状に
	  //  敷き詰められているかのように処理してくれるようになる。
	  //   -2  -1   0　 1　  2  u方向
	  // -1 +---+---+---+---+
	  //    |   |   |   |   |
	  //  0 +---+---+---+---+ こんな感じになる
	  //    |   |   |   |   |
	  //  1 +---+---+---+---+
	  //  v方向
	  samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	  samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	  samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	  // こっから下は割愛
	  samplerDesc.MaxLOD = FLT_MAX;
	  samplerDesc.MinLOD = -FLT_MAX;
	  samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	  // サンプラー用ディスクリプタヒープからメモリアドレスもらう
	  auto handleSampler = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		  m_SamplerHeap->GetCPUDescriptorHandleForHeapStart(),  // ヒープ先頭
		  0,  // 0番目をもらう(最初のサンプラー)
		  device_->samplerDesctiptorSize());  // サンプラのデスクリプタサイズ

	  // サンプラー生成
	  dev->CreateSampler(&samplerDesc, handleSampler);
  }

  // 次にCBV/SRVのデスクリプタヒープを作るよ
  {
	  // シェーダの中で定義した定数バッファとSRCで2個だけど、
	  // 定数バッファはダブルバッファ化したいので2個分つくる
	  // なので SRVの数 + (定数バッファの数 * backBufferSize) * オブジェクト数分
	  // 作成するのが最大効率得られそうな気がする
	  D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
		  D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		  1 + (1 * device_->backBufferSize()),
		  D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
	  // GPUのハンドルは後からでも取れるのでいまは生成してあればよい
	  dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cbvSrvHeap));
  }

  // CBVを作るよ
  {
	  // キューブ(オブジェクト単位での)の定数バッファ
	  // バッファとビューをバックバッファ分確保
	  auto bufferCount = device_->backBufferSize();
	  m_CubeObj.cbuffer.resize(bufferCount);

	  auto cbSize = sizeof(BasicShaderCB);
	  for (UINT i = 0; i < bufferCount; i++) {
		  // オブジェクトごとの定数バッファ作成
		  auto& cbuffer = m_CubeObj.cbuffer[i];
		  cbuffer = std::make_unique<BufferObject>();
		  // バッファオブジェクト初期化
		  cbuffer->Setup(dev, BufferType::ConstantBuffer, cbSize);

		  // 定数バッファビューをつくります
		  D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc{};
		  // GPUアドレス
		  cbDesc.BufferLocation = cbuffer->GetResouce()->GetGPUVirtualAddress();
		  cbDesc.SizeInBytes = static_cast<UINT>(cbuffer->GetBufferSize());

		  // ヒープの位置を取得
		  CD3DX12_CPU_DESCRIPTOR_HANDLE handleCBV(
			  m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart(),  // アドレス先頭
			  static_cast<UINT>(BasicShaderResourceIndex::Constant) +
			  i,  // CBVは0/1番
			  device_->cbvDescriptorSize());
		  // バッファビューを作る
		  dev->CreateConstantBufferView(&cbDesc, handleCBV);
	  }
  }

  // テクスチャからシェーダーリソースビュー(SRV)の準備.
  {
	  auto handleSRV = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		  m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart(),  // ヒープ先頭から
		  static_cast<UINT>(
			  BasicShaderResourceIndex::Srv),  // 2番目のデスクリプタ仕様
		  device_->srvDescriptorSize());       // SRVデスクリプタサイズ

	  // 猫ちゃんテクスチャを持ってくる
	  auto tex = Singleton<TextureManager>::instance().texture("cat");

	  // SRV設定
	  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	  srvDesc.Texture2D.MipLevels =
		  tex->GetDesc().MipLevels;            // テクスチャと合わせる
	  srvDesc.Format = tex->GetDesc().Format;  // テクスチャと合わせる
	  // この下はとりあえずこれでOK
	  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	  // SRV生成
	  // GPUのハンドルは後からでも取れるのでいまは生成してあればよい
	  dev->CreateShaderResourceView(tex.Get(), &srvDesc, handleSRV);
  }

  // キューブ用GeometoryMeshの生成
  m_CubeObj.mesh = GeometoryMesh::CreateCube(device_->device());
  // キューブのトランスフォームを初期化
  m_CubeObj.trasnform.Reset();

  // カメラ設定
  camera_.Reset();
  camera_.fov = static_cast<float>(screenWidth) / screenHeight;
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

void Application::Update(const DX::StepTimer& timer) 
{
	// カメラ更新
	{
		camera_.CalculateViewMatrix();
		camera_.CalculateProjMatrix();
		camera_.viewProj = camera_.view * camera_.proj;
	}
	UpdateCube();
}

void Application::Render() {
  // ここで描画処理をする
  device_->PrepareRendering();
  ClearRenderTarget();
  // 描画処理ここｋら
  m_BasicShader->Begin(device_->graphicsCommandList());
  DrawCube();
  m_BasicShader->End();
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
void Application::UpdateCube()
{
  // 移動・回転・拡縮がベクトルになっていると人間としては扱いやすい
  // しかしベクトルのままだとレンダリング時に扱いずらい.。
  // 平行移動(Translate),回転(Rotation),スケール(Scale)をばらばらに送ると
  // シェーダ内で全頂点ごとにそれぞれを合成する計算をすることになる。
  // そこでオブジェクト単位の情報はアプリケーションで計算してあげる。

  // Transformのベクトルデータを行列に変換

  // 不思議なことにSRTの3ベクトル成分を合成して、4x4行列に変換することができる
  // あとはシェーダで行列と頂点座標を合成すると、カメラから見た頂点の位置に変換できる
  // 不思議だね、すごいね

  // 動作確認でとりあえずY回転させておくよ
	m_CubeObj.trasnform.rot.y +=
		static_cast<float>(DirectX::XM_PIDIV2 * timer_.GetElapsedSeconds());

	// まずはベクトル毎に行列に変換
	auto& trans = m_CubeObj.trasnform;
	XMMATRIX s = XMMatrixScaling(trans.sca.x, trans.sca.y, trans.sca.z);
	auto r = XMMatrixRotationY(trans.rot.y);
	auto t = XMMatrixTranslation(trans.pos.x, trans.pos.y, trans.pos.z);

	// 移動(Translate)/回転(Rotation)/スケール(Scale)でまとめてSRTとか言ったりする
	// SRTを合成するにはそれぞれを行列化して、すべてを掛け算をすればよい
	// ただし行列は掛け算の順序により、計算結果が変わってしますので注意
	// 正しい計算方法はないので、今回は一番わかりやすい結果になる計算とする

	//計算のイメージとしては左から右に変換されていく
	// 1．s; [原点(0,0,0)でスケーリング]
	// 2. s * r; [スケールした状態で原点で回転する]
	// 3. s* r * t; [回転した状態で平行移動(オブジェクトのXYZも回転している)]
	//  これで衛星が公転するような移動を表現できる
	trans.world = s * r * t;
	// ちなみにオブジェクトのシーン内での行列をワールド行列と呼ぶ
}
void Application::DrawCube()
{
	auto buckBufferIndex = device_->backBufferIndex();

	// シェーダにキューブの定数バッファを送りつけます
	auto& cbuffer = m_CubeObj.cbuffer[buckBufferIndex];
	{
		BasicShaderCB param{};
		// 行列の合成
		auto w = m_CubeObj.trasnform.world;
		// ワールド・ビュー・射影変換行列
		auto wvp = w * camera_.viewProj;

		// XMMatrixTransposeは行列の並びを変える関数
		// 1行目を1列目に...4行目を4列目にする
		// DirectXMathの行列とシェーダーの行列の並びがちがうため
		// XMStoreFloat4x4はXMMATRIXのデータをXMFLOAT4x4にロード
		w = XMMatrixTranspose(w);
		XMStoreFloat4x4(&param.world, w);

		wvp = XMMatrixTranspose(wvp);
		XMStoreFloat4x4(&param.wvp, wvp);

		// バッファに書き込み
		cbuffer->Update(&param, sizeof(BasicShaderCB));
	}
	// シェーダーリソースを渡して
	m_BasicShader->SetCBufferDescriptorHeap(m_cbvSrvHeap.Get(), buckBufferIndex);
	m_BasicShader->SetSrvDescriptorHeap(m_cbvSrvHeap.Get(), 2);
	m_BasicShader->SetSamplerDescriptorHeap(m_SamplerHeap.Get(), 0);

	// シェーダー側のコマンド発行
	m_BasicShader->Apply();

	// メッシュ自体のコマンド
	m_CubeObj.mesh->Draw(device_->graphicsCommandList());
}
}  // namespace dxapp
