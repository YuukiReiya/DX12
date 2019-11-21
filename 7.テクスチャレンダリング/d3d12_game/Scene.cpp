#include "Scene.hpp"

#include "BufferObject.hpp"
#include "Camera.hpp"
#include "Device.hpp"
#include "GeometoryMesh.hpp"
#include "LightingShader.hpp"
#include "Material.hpp"
#include "TextureManager.hpp"

namespace {

/*!
 * @brief ワールド行列を作成するためのデータ
 */
struct Transform {
  DirectX::XMFLOAT3 pos{};                               //!< 座標
  DirectX::XMFLOAT3 rot{};                               //!< 回転
  DirectX::XMFLOAT3 sca{1, 1, 1};                        //!< スケール
  DirectX::XMMATRIX world{DirectX::XMMatrixIdentity()};  //!< ワールド行列
  DirectX::XMMATRIX texTrans{
      DirectX::XMMatrixIdentity()};  //!< テクスチャトランスフォーム
};

/*!
 * @brief 描画オブジェクト
 */
struct RenderObject {
  Transform transform;  //! オブジェクトのトランスフォーム
  std::vector<std::unique_ptr<dxapp::BufferObject>>
      transCb;  //! transformの定数バッファへのポインタ

  // 下のデータはほかのオブジェクトと共有できる情報なのでポインタでもらっておく
  dxapp::GeometoryMesh* mesh;  //! メッシュ
  dxapp::Material* material;   //! マテリアル
};
}  // namespace

namespace dxapp {
using namespace DirectX;
using namespace Microsoft::WRL;

/*!
 * @brief Sceneクラスの内部実装
 */
class Scene::Impl {
 public:
  Impl();
  ~Impl() = default;

  /*!
   * @brief 初期化
   */
  void Initialize(Device* device);

  /*!
   * @brief 描画
   */
  void Render(Device* device);

  /*!
   * @brief 更新
   */
  void Update(float deltaTime);

 private:
  /*!
   * @brief CBV/SRVデスクリプタヒープ生成
   */
  void CreateCbvSrvHeap(Device* device);

  /*!
   * @brief サンプラーデスクリプタヒープ生成
   */
  void CreateSamplerHeap(Device* device);

  /*!
   * @brief SRV生成
   */
  void CreateSrv(Device* device, ID3D12Resource* tex,
                 D3D12_CPU_DESCRIPTOR_HANDLE handle, int offset);

  /*!
   * @brief BufferObject生成
   */
  void CreateBufferObject(std::unique_ptr<BufferObject>& buffer,
                          ID3D12Device* device, std::size_t bufferSize);

  /*!
   * @brief BufferObjectからViewを生成
   */
  void CreateBufferView(std::unique_ptr<BufferObject>& buffer, Device* device,
                        D3D12_CPU_DESCRIPTOR_HANDLE heapStart, int offset);

  /*!
   * @brief テクスチャありのRenderObject生成
   */
  void CreateRenderObj(Device* device);

  /*!
   * @brief テクスチャありのRenderObject生成
   */
  void CreateMaterial(Device* device);

  // サンプラ デスクリプタヒープ
  ComPtr<ID3D12DescriptorHeap> samplerHeap_{};
  // CBV/SRV デスクリプタヒープ
  ComPtr<ID3D12DescriptorHeap> cbvSrvHeap_{};

  // カメラ
  FpsCamera camera_;

  // シェーダー
  std::unique_ptr<LightingShader> lightingShader_;

  // メッシュデータ
  std::unique_ptr<GeometoryMesh> teapotMesh_;

  // 描画オブジェクト
  std::vector<std::unique_ptr<RenderObject>> renderObjs_;

  // シーンパラメータ
  // このサンプルでは1つあればOK
  LightingShader::SceneParam sceneParam_;
  std::vector<std::unique_ptr<BufferObject>> sceneParamCb_;

  // マテリアルは使いまわせるので連想配列に入れて管理
  std::unordered_map<std::string, std::unique_ptr<Material>> materials_;

  /*!
   * @brief レンダーテクスチャに使うデスクリプタヒープの生成
   */
  void CreateRenderTextureHeap(Device* device);

  /*!
   * @brief レンダーテクスチャに使うデスクリプタヒープの生成
   */
  void CreateRenderTextureObject(Device* device);

  // レンダーテクスチャのサイズ
  // このサイズである必要はない
  // 画面全体にエフェクトかけるときはスクリーンサイズで作ったりする
  const UINT RenderTextureSize_{ 512 };

  // RTV/DSV/SRVのデスクリプタヒープはDevice.cppでも作っているが
  // バックバッファ用に使う事しか考えてない。修正すると話が
  // ややこしくなるのでわかりやすさ優先で別物として作る
  ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_{ nullptr };
  ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_{ nullptr };

  // こちらもすでにcbvSrvHeap_を作っているがこちらもわかりやすさのため
  // 別で作ってみます。仕組みがわかる人はcbvSrvHeap_を使っていいよ
  ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_{ nullptr };

  //-----------------------------------------------------------------
  // レンダーテクスチャ（カラーテクスチャ）
  // レンダーテクスチャの場合は次の内容で準備する
  //    1. レンダーターゲットとして使えるテクスチャの作成
  //    2. レンダーターゲットビューにする
  //    3. レンダーテクスチャからSRVつくる
  // ということで変数はテクスチャ1個・ディスクリプタが2個必要
  //-----------------------------------------------------------------
  // レンダーテクスチャそのもの
  ComPtr<ID3D12Resource> rtTexture_;  // rtは"R"ender "T"argetの略
  // レンダーターゲットとしてのビュー
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_;  // RenderTarget "V"iewの略
  // rtTexture_のシェーダーリソースとしてのビュー
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtTextureSrv_;

  // レンダーターゲット時のクリアカラー
  // ビュー作成時とコマンドリストでのクリアカラーが一致していないとエラーが出る
  float clearColor_[4]{ 0.2f, 0.2f, 0.8f, 0.0f };

  // レンダーターゲットのテクスチャを張るキューブオブジェクト<=メッシュデータ！！
  std::unique_ptr<GeometoryMesh> cubeMesh_;
  // ティーポットとは一緒に描画できないので別の管理にする
  std::vector<std::unique_ptr<RenderObject>> rtCubes_;

  // キューブ描画に使うカメラ
  FpsCamera cubeCamera_;

  // キューブ描画のシーン定数
  LightingShader::SceneParam cubeSceneParam_;
  std::vector<std::unique_ptr<BufferObject>> cubeSceneParamCB_;
};

Scene::Impl::Impl(){};

void Scene::Impl::Initialize(Device* device) {
  CreateSamplerHeap(device);
  CreateCbvSrvHeap(device);

  camera_.LookAt({0, 5, -5}, {0, 0, 0}, {0, 1, 0});
  camera_.UpdateViewMatrix();

  Mouse::Get().SetMode(Mouse::MODE_RELATIVE);

  // テクスチャロード
  {
    // ダミー用テクスチャ
    Singleton<TextureManager>::instance().LoadWICTextureFromFile(
        device, L"Assets/uv_checker.png", "uv_checker");

    Singleton<TextureManager>::instance().LoadWICTextureFromFile(
        device, L"Assets/bricks.png", "bricks");

    Singleton<TextureManager>::instance().LoadWICTextureFromFile(
        device, L"Assets/fabric.png", "fabric");

    Singleton<TextureManager>::instance().LoadWICTextureFromFile(
        device, L"Assets/grass.png", "grass");

    Singleton<TextureManager>::instance().LoadWICTextureFromFile(
        device, L"Assets/travertine.png", "travertine");
  }

  // シェーダー作成
  lightingShader_ = std::make_unique<LightingShader>();
  lightingShader_->Initialize(device);

  // uv_checkerをダミーテクスチャを設定（uv_checkerはヒープ0にある）
  lightingShader_->SetDammySrvDescriptorHeap(cbvSrvHeap_.Get(), 0);
  // さらについでにデフォルトサンプラも入れておきますね
  lightingShader_->SetDefaultSamplerDescriptorHeap(samplerHeap_.Get(), 0);

  // メッシュ作成
  teapotMesh_ = GeometoryMesh::CreateTeapot(device->device());

  // マテリアル作成
  CreateMaterial(device);

  // 描画オブジェクト作成
  CreateRenderObj(device);

  // ライトの設定
  // ライトが3つあるのは3点照明を作りたいから
  // 光源が1つだけだど蔭が強すぎるので補助ライトを2つおいてあげる
  // 現実世界での照明方法ではあるがCGでも一般的
  // （物理ベースのシェーディングでは使わないけど）　
  {
    // 環境光（最低限これくらいは明るい）
    sceneParam_.ambientLight = {0.25f, 0.25f, 0.25f, 1.0f};

    // メインのライト（キーライトといいます）
    // 光の向き
    sceneParam_.lights[0].direction = {0.57f, -0.57f, 0.57f};
    // メインなのでライトの明るさがほかのより強い
    sceneParam_.lights[0].strength = {0.8f, 0.8f, 0.8f};

    // 補助ライト1（フィルライト）
    // キーライトの反対側において、キーライトの陰を弱くします
    sceneParam_.lights[1].direction = {-0.57f, -0.57f, 0.57f};
    // ライトはメインより暗くします
    sceneParam_.lights[1].strength = {0.4f, 0.4f, 0.4f};

    // 補助ライト2（バックライト）
    // ほんとはオブジェクトの反対側において輪郭を浮き上がらせるためのライト
    // 動かすのが面倒なのでいまは固定
    sceneParam_.lights[2].direction = {0.0f, -0.707f, -0.707f};
    // ライトは暗め
    sceneParam_.lights[2].strength = {0.2f, 0.2f, 0.2f};

    // SceneParamの定数バッファを作成
    sceneParamCb_.resize(device->backBufferSize());
    for (auto& cb : sceneParamCb_) {
      CreateBufferObject(cb, device->device(),
                         sizeof(LightingShader::SceneParam));
    }
  }

#pragma region add_1119
  CreateRenderTextureHeap(device);
  CreateRenderTextureObject(device);
#pragma endregion
}

void Scene::Impl::Update(float deltaTime) {
	auto keyState = Keyboard::Get().GetState();
	auto mouseState = Mouse::Get().GetState();

	// マウスの右ボタン押してるとカメラ更新
	if (mouseState.rightButton) {
		if (keyState.W) {
			camera_.Dolly(+3.0f * deltaTime);
		}
		if (keyState.S) {
			camera_.Dolly(-3.0f * deltaTime);
		}

		if (keyState.D) {
			camera_.Truck(+3.0f * deltaTime);
		}
		if (keyState.A) {
			camera_.Truck(-3.0f * deltaTime);
		}

		if (keyState.Q) {
			camera_.Boom(+3.0f * deltaTime);
		}
		if (keyState.E) {
			camera_.Boom(-3.0f * deltaTime);
		}

		auto x = static_cast<float>(mouseState.x);
		auto y = static_cast<float>(mouseState.y);

		// 適当な感じで値を補正してます
		x = XMConvertToRadians(0.3f * x);
		y = XMConvertToRadians(0.3f * y);
		camera_.Pan(x);
		camera_.Tilt(y);
	}

#pragma region 追記
	else
	{


		if (keyState.W)
		{

		}


	}

#pragma endregion


	camera_.UpdateViewMatrix();

	if (keyState.L && keyState.D1) {
		sceneParam_.lights[0].strength = { 0.8f, 0.8f, 0.8f };
	}

	if (keyState.L && keyState.D2) {
		sceneParam_.lights[0].strength = { 0.1f, 0.1f, 0.1f };
	}

#pragma region 課題2
	//変更箇所が複数あると大変だし、一か所にまとめて書く。本来はやらんよ。。。


	//ティーポット2つはスケールゼロで非表示にする
	{
		auto objs = { &renderObjs_[1]->transform,&renderObjs_[2]->transform, };
		for (auto it : objs)
		{
			it->world = XMMatrixScaling(0, 0, 0);
		}
	}
	//キューブの座標
	const XMFLOAT3 c_CubePos{ 0,0,5 };
	{
		auto& transform = rtCubes_[0]->transform;

		transform.sca.x = transform.sca.y = transform.sca.z = 2.0f;

		XMMATRIX s, r, t;
		s = XMMatrixScaling(transform.sca.x, transform.sca.y, transform.sca.z);
		r = XMMatrixRotationRollPitchYaw(transform.rot.x, transform.rot.y, transform.rot.z);
		t = XMMatrixTranslation(c_CubePos.x, c_CubePos.y, c_CubePos.z);
		transform.world = s * r * t;
	}
	//カメラ
	{
		auto& cam = cubeCamera_;
		auto& tmpC = camera_;
		static XMFLOAT3 pos = { 0,0,-2 };
		
		float val = 0.01f;
		if (keyState.W)
		{
			pos.z += val;
		}
		if (keyState.S)
		{
			pos.z += -val;
		}
		if (keyState.Q)
		{
			pos.y += val;
		}
		if (keyState.E)
		{
			pos.y += -val;
		}

		if (keyState.F)
		{
			auto& cc = cubeCamera_;
			auto& c = camera_;

			XMFLOAT3 cpos = {0,1.f,2};
			XMFLOAT3 lpos = { c_CubePos };
			cc.LookAt(cpos, lpos, { 0,1,0 });
			c.LookAt(cc.position(), { 0,0,0 }, { 0,1,0 });
			cc.UpdateViewMatrix();
			c.UpdateViewMatrix();
		}

		cam.LookAt(
			pos,
			{ 0,0,0 },
			{ 0,1,0 }
		);

		//cam.LookAt(
		//	camera_.position(),
		//	camera_.,
		//	{ 0,1,0 }
		//);

		cam.UpdateViewMatrix();
	}

#pragma endregion


}

void Scene::Impl::Render(Device* device) {
	auto index = device->backBufferIndex();

#pragma region add_1119
	auto commandList = device->graphicsCommandList();

	//
	// まずは前回までのティーポットをレンダーテクスチャに描画する
	//

	// レンダーターゲットテクスチャの初期設定
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvs[]{ rtv_ };
		D3D12_CPU_DESCRIPTOR_HANDLE dsv =
			device->depthStencilView();  // デフォルトのデプスステンシル

		// まずはレンダーターゲット・デプスをきれいにする
		commandList->ClearRenderTargetView(rtv_, clearColor_, 0, nullptr);
		// デプスもきれいにしておく
		commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0,
			nullptr);

		// レンダーテクスチャを描画先にする
		commandList->OMSetRenderTargets(_countof(rtvs), rtvs, FALSE, &dsv);

		// ビューポート
		auto viewport = CD3DX12_VIEWPORT(
			0.0f, 0.0f,
			static_cast<float>(
				RenderTextureSize_),  // 横 テクスチャサイズに合わせる
			static_cast<float>(
				RenderTextureSize_));  // 縦 テクスチャサイズに合わせる
		commandList->RSSetViewports(1, &viewport);

		// シザー
		auto scissorRect = CD3DX12_RECT(
			0, 0,
			static_cast<LONG>(RenderTextureSize_),  // 横 テクスチャサイズに合わせる
			static_cast<LONG>(
				RenderTextureSize_));  // 縦 テクスチャサイズに合わせる
		commandList->RSSetScissorRects(1, &scissorRect);
	}
	// この下からは前回までの内容
#pragma endregion

#pragma region 前回
	lightingShader_->Begin(device->graphicsCommandList());
	// SceneParam転送
	{
		auto v = camera_.view();
		auto p = camera_.proj();
		auto vp = v * p;
		XMStoreFloat4x4(&sceneParam_.view, XMMatrixTranspose(v));
		XMStoreFloat4x4(&sceneParam_.proj, XMMatrixTranspose(p));
		XMStoreFloat4x4(&sceneParam_.viewProj, XMMatrixTranspose(vp));
		sceneParam_.eyePos = camera_.position();

		sceneParamCb_[index]->Update(&sceneParam_,
			sizeof(LightingShader::SceneParam));

		lightingShader_->SetSceneParam(
			sceneParamCb_[index]->resource()->GetGPUVirtualAddress());
	}

	// マテリアル転送
	for (auto& mat : materials_) {
		mat.second->Update(index);
	}

	// オブジェクト描画
	for (auto& obj : renderObjs_) {
		LightingShader::ObjectParam param{};
		XMStoreFloat4x4(&param.world, XMMatrixTranspose(obj->transform.world));
		XMStoreFloat4x4(&param.texTrans,
			XMMatrixTranspose(obj->transform.texTrans));

		// バッファ転送
		auto& cbuffer = obj->transCb[index];
		cbuffer->Update(&param, sizeof(LightingShader::ObjectParam));

		// 定数バッファ・テクスチャなどの設定
		{
			lightingShader_->SetObjectParam(
				cbuffer->resource()->GetGPUVirtualAddress());

			lightingShader_->SetMaterialParam(obj->material->materialCb(index));

			// テクスチャがあれば設定
			if (obj->material->HasTexture()) {
				ID3D12DescriptorHeap* srv = nullptr;
				std::uint32_t srvOffset = 0;
				obj->material->textureDescHeap(&srv, &srvOffset);

				lightingShader_->SetSrvDescriptorHeap(srv, srvOffset);
				lightingShader_->SetSamplerDescriptorHeap(samplerHeap_.Get(), 0);
			}
		}
		// コマンドリスト発行
		lightingShader_->Apply();

		// メッシュ描画コマンド発行
		obj->mesh->Draw(device->graphicsCommandList());
	}
	lightingShader_->End();
#pragma endregion

#pragma region add_1119
	// レンダーターゲットからテクスチャとして使用可能になるまで待つためのバリア
   // レンダーテクスチャはレンダーターゲットとシェーダーリソースの二つの面を持つ

	// 描画コマンドは発行して即時完了しているわけではない。
	// レンダリングが終わってない状態で、シェーダーリソースになると描画が壊れてしまう。

	// そのため実際の描画が終わるまでリソースへバリアを設定し
	// 適切な状態になるまで次の処理を待ってもらことができる
	auto barrierToSRV = CD3DX12_RESOURCE_BARRIER::Transition(
		rtTexture_.Get(),                    // このテクスチャが
		D3D12_RESOURCE_STATE_RENDER_TARGET,  // レンダーターゲットから
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);  // SRV状態になるまで
	commandList->ResourceBarrier(                     // 待つ
		1,               // 設定するバリアの数
		&barrierToSRV);  // バリアのアドレス

	// デバイスが持っているデフォルトのレンダーターゲットを取得
	auto rtv = device->currentRenderTargetView();  // バックバッファ
	auto dsv = device->depthStencilView();         // デフォルトのDSV

	// カラーバッファ(レンダーターゲットビュー)のクリア
	commandList->ClearRenderTargetView(rtv, Colors::CadetBlue, 0, nullptr);
	// デプスバッファ(デプスステンシルビュー)のクリア
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0,
		nullptr);

	// 描画先をバックバッファのレンダーターゲットとする
	commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	// ビューポート・シザーもウィンドウに基づく値にする
	auto viewport = device->screenViewport();
	commandList->RSSetViewports(1, &viewport);
	auto scissorRect = device->scissorRect();
	commandList->RSSetScissorRects(1, &scissorRect);

	//
	// ここから描画
	//

	// シェーダーは使いまわし
	lightingShader_->Begin(device->graphicsCommandList());

	// カメラ設定が違うのでSceneParamは転送
	{
		auto v = cubeCamera_.view();
		auto p = cubeCamera_.proj();
		auto vp = v * p;
		XMStoreFloat4x4(&cubeSceneParam_.view, XMMatrixTranspose(v));
		XMStoreFloat4x4(&cubeSceneParam_.proj, XMMatrixTranspose(p));
		XMStoreFloat4x4(&cubeSceneParam_.viewProj, XMMatrixTranspose(vp));
		cubeSceneParam_.eyePos = cubeCamera_.position();

		cubeSceneParamCB_[index]->Update(&cubeSceneParam_,
			sizeof(LightingShader::SceneParam));

		lightingShader_->SetSceneParam(
			cubeSceneParamCB_[index]->resource()->GetGPUVirtualAddress());
	}

	// オブジェクト描画
	for (auto& obj : rtCubes_) {
		LightingShader::ObjectParam param{};
		XMStoreFloat4x4(&param.world, XMMatrixTranspose(obj->transform.world));
		XMStoreFloat4x4(&param.texTrans,
			XMMatrixTranspose(obj->transform.texTrans));

		// バッファ転送
		auto& cbuffer = obj->transCb[index];
		cbuffer->Update(&param, sizeof(LightingShader::ObjectParam));

		// 定数バッファ・テクスチャなどの設定
		{
			lightingShader_->SetObjectParam(
				cbuffer->resource()->GetGPUVirtualAddress());

			lightingShader_->SetMaterialParam(obj->material->materialCb(index));

			// テクスチャがあれば設定
			if (obj->material->HasTexture()) {
				ID3D12DescriptorHeap* srv = nullptr;
				std::uint32_t srvOffset = 0;
				obj->material->textureDescHeap(&srv, &srvOffset);

				lightingShader_->SetSrvDescriptorHeap(srv, srvOffset);
				lightingShader_->SetSamplerDescriptorHeap(samplerHeap_.Get(), 0);
			}
		}
		// コマンドリスト発行
		lightingShader_->Apply();

		// メッシュ描画コマンド発行
		obj->mesh->Draw(device->graphicsCommandList());
	}
	lightingShader_->End();

	// レンダーテクスチャがシェーダーリソースから
	// レンダーターゲットとして使えるのを待つ
	auto toRT = CD3DX12_RESOURCE_BARRIER::Transition(
		rtTexture_.Get(),  // レンダーテクスチャが
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,  // シェーダーリソースから
		D3D12_RESOURCE_STATE_RENDER_TARGET);  // レンダーターゲット状態になるまで

	CD3DX12_RESOURCE_BARRIER barriers[] = { toRT };
	commandList->ResourceBarrier(  // 待ってね
		_countof(barriers),  // こんな感じで配列経由で複数のバリア(今は1個だけど)
		barriers);  // を一気に設定することもできるよ
#pragma endregion
}

void Scene::Impl::CreateSamplerHeap(Device* device) {
  auto dev = device->device();

  // サンプラー用のデスクリプタヒープを作る
  D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
      D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,  //ヒープタイプはサンプラ
      1,                                   // 作るのは1個
      D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,  // シェーダから見える設定
      0};                                         // 0でOK
  dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&samplerHeap_));

  // サンプラーの設定を作る
  // テクスチャ補間の設定やテクスチャアドレッシング設定
  D3D12_SAMPLER_DESC samplerDesc{};
  // テクスチャフィルタ
  samplerDesc.Filter =
      D3D12_ENCODE_BASIC_FILTER(D3D12_FILTER_TYPE_LINEAR,  // 縮小時
                                D3D12_FILTER_TYPE_LINEAR,  // 拡大時
                                D3D12_FILTER_TYPE_LINEAR,  // mipmap
                                D3D12_FILTER_REDUCTION_TYPE_STANDARD);
  // テクスチャアドレッシング
  samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  samplerDesc.MaxLOD = FLT_MAX;
  samplerDesc.MinLOD = -FLT_MAX;
  samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

  auto handleSampler = CD3DX12_CPU_DESCRIPTOR_HANDLE(
      samplerHeap_->GetCPUDescriptorHandleForHeapStart(),
      0,  // 1個しかないので決め打ちでよい
      device->samplerDesctiptorSize());

  // サンプラー生成
  dev->CreateSampler(&samplerDesc, handleSampler);
}

void Scene::Impl::CreateCbvSrvHeap(Device* device) {
  auto dev = device->device();

  D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      2048,  // とりあえずいっぱい作っておきますね
      D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
  dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&cbvSrvHeap_));
}

void Scene::Impl::CreateSrv(Device* device, ID3D12Resource* tex,
                            D3D12_CPU_DESCRIPTOR_HANDLE handle, int offset) {
  auto handleSRV = CD3DX12_CPU_DESCRIPTOR_HANDLE(
      handle, offset,
      device->srvDescriptorSize());  // SRVデスクリプタサイズ

  // SRV設定
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Texture2D.MipLevels =
      tex->GetDesc().MipLevels;            // テクスチャと合わせる
  srvDesc.Format = tex->GetDesc().Format;  // テクスチャと合わせる
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

  // SRV生成
  device->device()->CreateShaderResourceView(tex, &srvDesc, handleSRV);
}

void Scene::Impl::CreateBufferObject(std::unique_ptr<BufferObject>& buffer,
                                     ID3D12Device* device,
                                     std::size_t bufferSize) {
  buffer = std::make_unique<BufferObject>();
  buffer->Initialize(device, BufferObjectType::ConstantBuffer, bufferSize);
}

void Scene::Impl::CreateBufferView(std::unique_ptr<BufferObject>& buffer,
                                   Device* device,
                                   D3D12_CPU_DESCRIPTOR_HANDLE heapStart,
                                   int offset) {
  D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
  desc.BufferLocation = buffer->resource()->GetGPUVirtualAddress();
  desc.SizeInBytes = static_cast<UINT>(buffer->bufferSize());

  // ヒープの位置を取得
  CD3DX12_CPU_DESCRIPTOR_HANDLE handle(heapStart, offset,
                                       device->cbvDescriptorSize());
  // バッファビューを作る
  device->device()->CreateConstantBufferView(&desc, handle);
}


void Scene::Impl::CreateMaterial(Device* device) {
  int offset = 0;  // デスクリプタヒープの位置
  {
    auto t = Singleton<TextureManager>::instance().texture("uv_checker");

    CreateSrv(device, t.Get(),
              cbvSrvHeap_->GetCPUDescriptorHandleForHeapStart(), offset);
    auto mat = std::make_unique<Material>();
    mat->Initialize(device);
    mat->SetTexture(cbvSrvHeap_.Get(), offset);
    mat->SetMatrix(DirectX::XMMatrixIdentity());
    materials_.emplace("uv_checker", std::move(mat));
    offset++;
  }

  {
    auto t = Singleton<TextureManager>::instance().texture("bricks");

    CreateSrv(device, t.Get(),
              cbvSrvHeap_->GetCPUDescriptorHandleForHeapStart(), offset);
    auto mat = std::make_unique<Material>();
    mat->Initialize(device);
    mat->SetTexture(cbvSrvHeap_.Get(), offset);
    mat->SetMatrix(DirectX::XMMatrixIdentity());

    materials_.emplace("bricks", std::move(mat));
    offset++;
  }
  {
    auto t = Singleton<TextureManager>::instance().texture("fabric");

    CreateSrv(device, t.Get(),
              cbvSrvHeap_->GetCPUDescriptorHandleForHeapStart(), offset);
    auto mat = std::make_unique<Material>();
    mat->Initialize(device);
    mat->SetTexture(cbvSrvHeap_.Get(), offset);

    mat->SetRoughness(0.9f);
    mat->SetFresnel({0.05f, 0.05f, 0.05f});
    mat->SetMatrix(DirectX::XMMatrixIdentity());

    materials_.emplace("fabric", std::move(mat));
    offset++;
  }
  {
    auto t = Singleton<TextureManager>::instance().texture("grass");

    CreateSrv(device, t.Get(),
              cbvSrvHeap_->GetCPUDescriptorHandleForHeapStart(), offset);
    auto mat = std::make_unique<Material>();
    mat->Initialize(device);
    mat->SetTexture(cbvSrvHeap_.Get(), offset);
    mat->SetMatrix(DirectX::XMMatrixIdentity());

    materials_.emplace("grass", std::move(mat));
    offset++;
  }
  {
    auto t = Singleton<TextureManager>::instance().texture("travertine");

    CreateSrv(device, t.Get(),
              cbvSrvHeap_->GetCPUDescriptorHandleForHeapStart(), offset);
    auto mat = std::make_unique<Material>();
    mat->Initialize(device);
    mat->SetTexture(cbvSrvHeap_.Get(), offset);
    mat->SetMatrix(DirectX::XMMatrixIdentity());

    materials_.emplace("travertine", std::move(mat));
    offset++;
  }
}

void Scene::Impl::CreateRenderTextureHeap(Device* device)
{
	auto dev = device->device();
	// RTV のディスクリプタヒープ。Device.cppでやっているのと同じこと
	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		10,  // なんとなく10個分用意してみる（もっとたくさんあてもよい
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
	dev->CreateDescriptorHeap(
		&rtvDesc, IID_PPV_ARGS(rtvDescriptorHeap_.ReleaseAndGetAddressOf()));

	// SRV のディスクリプタヒープ
	D3D12_DESCRIPTOR_HEAP_DESC srvDesc{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
									   10,  // とりあえず10個分
									   D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
									   0 };
	dev->CreateDescriptorHeap(
		&srvDesc, IID_PPV_ARGS(srvDescriptorHeap_.ReleaseAndGetAddressOf()));
}

void Scene::Impl::CreateRenderTextureObject(Device* device)
{
	// 今更だけど。device->device()って凄く悪い名前でしたね！
 // 後悔しているけどもう直さないぞ！
	auto dev = device->device();

	// ピクセルの色を記録するテクスチャのデスクリプタ
	// バックバッファはDXGIが用意するので不要な手順だが
	// レンダーターゲットとして使うテクスチャを用意
	{
		// まずはレンダーターゲットに使えるテクスチャを用意
		auto renderTexDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R8G8B8A8_UNORM,  // 普通にRGBA8フォーマット
			RenderTextureSize_,          // テクスチャサイズ横
			RenderTextureSize_,          // テクスチャサイズ縦
			1, 1, 1, 0,                  // ここはこれでOK
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);  // レンダーターゲットになることを許可

		D3D12_CLEAR_VALUE clearColor{};
		clearColor.Format = renderTexDesc.Format;
		// テクスチャをレンダーターゲットとしてクリアするときに
		// 色が違うと怒られてしまうので、ちゃんとクリアカラーで初期設定。
		clearColor.Color[0] = clearColor_[0];
		clearColor.Color[1] = clearColor_[1];
		clearColor.Color[2] = clearColor_[2];
		clearColor.Color[3] = clearColor_[3];

		// レンダーテクスチャ作成
		device->device()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
			&renderTexDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,  // レンダーターゲットとして作成
			&clearColor,                         // 塗りつぶしの色
			IID_PPV_ARGS(rtTexture_.GetAddressOf()));

		// レンダーターゲットビューのデスクリプタハンドルもらってくる
		// rtvDescriptorHeap_からもらうので注意。SRVとRTVは混ぜられない！
		rtv_ = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
			0,  // レンダーターゲットとして0番を使う
			dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

		// レンダーターゲットとしてビュー作成
		dev->CreateRenderTargetView(rtTexture_.Get(), nullptr, rtv_);

		// レンダーターゲットテクスチャをシェーダーリソースビューとして使えるようにする
		// srvDescriptorHeap_のデスクリプタハンドルもらってくる
		// この辺は通常のテクスチャと同じやり方
		rtTextureSrv_ = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
			0,  // SRVは0番を使う
			dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

		// レンダーターゲットテクスチャをSRVにする
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Format = rtTexture_->GetDesc().Format;  // テクスチャと合わせる
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		// レンダーテクスチャのSRV作成
		device->device()->CreateShaderResourceView(rtTexture_.Get(), &srvDesc,
			rtTextureSrv_);
	}

	// ここからレンダーテクスチャを画面に出すための設定
	auto bufferSize = device->backBufferSize();

	cubeMesh_ = GeometoryMesh::CreateCube(dev);
	{
		// 適当なマテリアを作ってレンダーテクスチャ割り当て
		auto mat = std::make_unique<Material>();
		mat->Initialize(device);
		mat->SetTexture(srvDescriptorHeap_.Get(), 0);
		mat->SetMatrix(DirectX::XMMatrixIdentity());
		materials_.emplace("renderTexture", std::move(mat));

		auto colorCube = std::make_unique<RenderObject>();
		colorCube->mesh = cubeMesh_.get();
		colorCube->material = materials_.at("renderTexture").get();
		colorCube->transCb.resize(bufferSize);
		for (auto& cb : colorCube->transCb) {
			CreateBufferObject(cb, device->device(),
				sizeof(LightingShader::ObjectParam));
		}
		// このオブジェクトは別管理
		rtCubes_.emplace_back(std::move(colorCube));
	}

	// ティーポット描画に使うカメラのアスペクトを1.0fにしないと形が歪んじゃう
	// 試しにした2行をコメントアウトして確認するとよい
	camera_.SetLens(1.0, XM_PIDIV4, 1.0f, 1000.0f);
	camera_.UpdateViewMatrix();

	// キューブを描画するカメラは固定
	cubeCamera_.LookAt({ 0.0f, 0.0f, -1.5f }, { 0.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f });
	cubeCamera_.UpdateViewMatrix();

	// とりあえずティーポットのシーンデータをコピー
	cubeSceneParam_ = sceneParam_;

	// カメラ設定が変わるのでシーンの定数バッファは別に作る
	cubeSceneParamCB_.resize(bufferSize);
	for (auto& cb : cubeSceneParamCB_) {
		CreateBufferObject(cb, device->device(),
			sizeof(LightingShader::SceneParam));
	}
}

void Scene::Impl::CreateRenderObj(Device* device) {
  auto bufferSize = device->backBufferSize();
  // とりあえずティーポットを1個だけ作るよ
  {
    auto teapot = std::make_unique<RenderObject>();
    teapot->mesh = teapotMesh_.get();
    teapot->transform.texTrans = XMMatrixIdentity();
    teapot->transCb.resize(bufferSize);
    for (auto& cb : teapot->transCb) {
      CreateBufferObject(cb, device->device(),
                         sizeof(LightingShader::ObjectParam));
    }
    teapot->material = materials_.at("travertine").get();
    renderObjs_.emplace_back(std::move(teapot));
  }

  {
    auto teapot = std::make_unique<RenderObject>();
    teapot->mesh = teapotMesh_.get();
    teapot->transform.texTrans = XMMatrixIdentity();
    teapot->transCb.resize(bufferSize);
    for (auto& cb : teapot->transCb) {
      CreateBufferObject(cb, device->device(),
                         sizeof(LightingShader::ObjectParam));
    }
    teapot->material = materials_.at("fabric").get();
    renderObjs_.emplace_back(std::move(teapot));
  }

  {
    auto teapot = std::make_unique<RenderObject>();
    teapot->mesh = teapotMesh_.get();
    teapot->transform.texTrans = XMMatrixIdentity();
    teapot->transCb.resize(bufferSize);
    for (auto& cb : teapot->transCb) {
      CreateBufferObject(cb, device->device(),
                         sizeof(LightingShader::ObjectParam));
    }
    teapot->material = materials_.at("bricks").get();
    renderObjs_.emplace_back(std::move(teapot));
  }
}

//-------------------------------------------------------------------
// Sceneの実装
//-------------------------------------------------------------------
Scene::Scene() : impl_(new Impl){};
Scene::~Scene() {}

void Scene::Initialize(Device* device) { impl_->Initialize(device); };

void Scene::Terminate(){};

void Scene::Update(float deltaTime) { impl_->Update(deltaTime); };

void Scene::Render(Device* device) { impl_->Render(device); };

}  // namespace dxapp
