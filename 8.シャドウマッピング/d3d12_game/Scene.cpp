#include "Scene.hpp"

#include "BufferObject.hpp"
#include "Camera.hpp"
#include "Device.hpp"
#include "GeometoryMesh.hpp"
#include "LightingShader.hpp"
#include "Material.hpp"
#include "TextureManager.hpp"

#pragma region 追加
#include <DirectXCollision.h>
#include "ShadowMapDebugShader.hpp"
#include "ShadowMapShader.hpp"
#pragma endregion

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
	 * @brief マテリアル生成
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

	// 影を落とすためにメッシュを用意
	std::unique_ptr<GeometoryMesh> floorMesh_;
	std::unique_ptr<GeometoryMesh> wallMesh_;
	std::unique_ptr<GeometoryMesh> pillarMesh_;

#pragma region 追加
	void CreateDSVHeap(Device* device);
	void CreateShadowMapSampler(Device* device);
	void CreateShadowMapObject(Device* device);

	std::unique_ptr<ShadowMapShader>shadowMapShader_;

	// シャドウマップの解像度
   // これが大きいほど影がきれいにでる。大きすぎると高負荷
   // このサンプルでは上げても大したことないので
   // 2048とかでもぜんぜんOK
	const UINT ShadowMapSize_{ 512 };

	// シャドウマップになるテクスチャ
	ComPtr<ID3D12Resource> shadowMap_;

	// DSVのデスクリプタヒープ・リソースハンドル
	ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_{ nullptr };
	// シャドウマップのDSVのハンドル
	CD3DX12_CPU_DESCRIPTOR_HANDLE shadowMapDsv_;
	// シャドウマップのSRVハンドル
	CD3DX12_CPU_DESCRIPTOR_HANDLE shadowMapSrv_;

	// デプスステンシルのクリア値
	D3D12_CLEAR_VALUE clearDepth_{};

	// シャドウマップ描画用のビューポートとシザー
	D3D12_VIEWPORT smViewport_ =
		CD3DX12_VIEWPORT{ 0.0f,
						 0.0f,
						 static_cast<float>(ShadowMapSize_),
						 static_cast<float>(ShadowMapSize_),
						 0.0f,
						 1.0f };

	D3D12_RECT smScissor_ = CD3DX12_RECT{ 0, 0, static_cast<LONG>(ShadowMapSize_),
										 static_cast<LONG>(ShadowMapSize_) };

	// バウンディングスフィア
	// 影を落とす領域を決めるための球として使う
	DirectX::BoundingSphere sceneBounds_{};

	// シャドウマップのシーンパラメータ
	// ライティングの使いまわし
	LightingShader::SceneParam shadowParam_{};

	// シャドウマップ描画用の定数バッファ
	std::vector<std::unique_ptr<BufferObject>> shadowSceneCB_;

	// シャドウマップ確認用のクアッド
	std::unique_ptr<GeometoryMesh> quadMesh_;
	std::unique_ptr<ShadowMapDebugShader> debugShader_;

#pragma endregion

};

Scene::Impl::Impl(){};

void Scene::Impl::Initialize(Device* device) {
  CreateSamplerHeap(device);
  CreateCbvSrvHeap(device);

  camera_.LookAt({0, 2, -5}, {0, 0, 0}, {0, 1, 0});
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

#pragma region 追加
	//シャドウマップのオブジェクトを追加
	CreateShadowMapObject(device);
#pragma endregion
  }
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
    camera_.UpdateViewMatrix();

  } else {
    auto& transform = renderObjs_[0]->transform;

    // このフレームでの移動行列を作る
    XMMATRIX addTransMat = XMMatrixIdentity();  // 正規化行列
    if (keyState.W) {
      addTransMat = XMMatrixTranslation(0, 0, 5.0f * deltaTime);
    }
    if (keyState.S) {
      addTransMat = XMMatrixTranslation(0, 0, -5.0f * deltaTime);
    }

    // 回転結果がtransform.posの影響を受けないので普通にtransform.rotを使う
    if (keyState.D) {
      transform.rot.y += XMConvertToRadians(90.f * deltaTime);
    }
    if (keyState.A) {
      transform.rot.y += XMConvertToRadians(-90.f * deltaTime);
    }

    // 行列を作る

    // 前フレームでの位置に動かす
    auto transMat =
        XMMatrixTranslation(transform.pos.x, transform.pos.y, transform.pos.z);

    auto rotY = XMMatrixRotationY(transform.rot.y);
    auto fixY = XMMatrixRotationY(XMConvertToRadians(180.0f));
    transform.world = addTransMat * rotY * transMat;

    // 行列から座標だけ抜き取る
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, transform.world);
    // 行列の下記の位置にxyzが入っている
    // 単純に行列から座標だけ
    transform.pos.x = m._41;
    transform.pos.y = m._42;
    transform.pos.z = m._43;
  }

  // 各種データ構築

  // 適当にライトを回す
  {
    float angle = 0.1f * deltaTime;
    XMMATRIX r = XMMatrixRotationY(angle);
    for (int i = 0; i < 3; i++) {
      XMVECTOR lightDir = XMLoadFloat3(&sceneParam_.lights[i].direction);
      lightDir = XMVector3TransformNormal(lightDir, r);
      XMStoreFloat3(&sceneParam_.lights[i].direction, lightDir);
    }
  }

  // カメラ更新
  {
    auto v = camera_.view();
    auto p = camera_.proj();
    auto vp = v * p;
    XMStoreFloat4x4(&sceneParam_.view, XMMatrixTranspose(v));
    XMStoreFloat4x4(&sceneParam_.proj, XMMatrixTranspose(p));
    XMStoreFloat4x4(&sceneParam_.viewProj, XMMatrixTranspose(vp));
    sceneParam_.eyePos = camera_.position();
  }

#pragma region 追加
  //シャドウ関連の更新
  {
	  // シャドウマップ描画用変数の設定
	  // 球の領域に収まる部分は影が落ちる
	  sceneBounds_.Center = XMFLOAT3{ 0.0f, 0.0f, 0.0f };

	  // 影を落としたい範囲
	  // 今回はfloorメッシュのサイズを包む外接円を描画範囲として使う
	  sceneBounds_.Radius = sqrtf(15.0f * 15.0f + 15.0f * 15.0f) / 2.0f;

	  // ライトのビュー行列を計算
	  XMVECTOR dir = XMLoadFloat3(&sceneParam_.lights[0].direction);
	  XMVECTOR pos = -2.0f * sceneBounds_.Radius * dir;
	  XMVECTOR target = XMLoadFloat3(&sceneBounds_.Center);
	  XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	  // 普通にLookAtで計算
	  XMMATRIX view = XMMatrixLookAtLH(pos, target, up);

	  // ビュー行列を使って影描画の領域を決める
	  XMFLOAT3 sphereCenter;
	  XMStoreFloat3(&sphereCenter, XMVector3TransformCoord(target, view));

	  // ライト空間の正射影行列つくる
	  float left = sphereCenter.x - sceneBounds_.Radius;
	  float right = sphereCenter.x + sceneBounds_.Radius;
	  float bottom = sphereCenter.y - sceneBounds_.Radius;
	  float top = sphereCenter.y + sceneBounds_.Radius;
	  float nearZ = sphereCenter.z - sceneBounds_.Radius;
	  float farZ = sphereCenter.z + sceneBounds_.Radius;
	  XMMATRIX proj =
		  XMMatrixOrthographicOffCenterLH(left, right, bottom, top, nearZ, farZ);

	  // 空間の数値は-1～1だけどUV座標は0～1なので
	  // この行列をかけるとうまいこと補正されます
	  XMMATRIX bias{ 0.5f, 0.0f,  0.0f, 0.0f,   //
					0.0f, -0.5f, 0.0f, 0.0f,   //
					0.0f, 0.0f,  1.0f, 0.0f,   //
					0.5f, 0.5f,  0.0f, 1.0f };  //

	  // 定数バッファに設定
	  XMStoreFloat4x4(&shadowParam_.view, XMMatrixTranspose(view));
	  XMStoreFloat4x4(&shadowParam_.proj, XMMatrixTranspose(proj));

	  auto vp = XMMatrixMultiply(view, proj);
	  XMStoreFloat4x4(&shadowParam_.viewProj, XMMatrixTranspose(vp));

	  // ここはsceneParam_に渡しているので注意
	  XMStoreFloat4x4(&sceneParam_.shadowTransform,
		  XMMatrixTranspose(XMMatrixMultiply(vp, bias)));
  }
#pragma endregion

}

void Scene::Impl::Render(Device* device) {
	auto index = device->backBufferIndex();
	auto commandList = device->graphicsCommandList();

	//
	// 定数のアップロードも更新でやったほうがいいよねー
	//

	// シーン定数を転送
	sceneParamCb_[index]->Update(&sceneParam_,
		sizeof(LightingShader::SceneParam));

	// マテリアルの定数転送
	for (auto& mat : materials_) {
		mat.second->Update(index);
	}

	// オブジェクトの定数
	for (auto& obj : renderObjs_) {
		LightingShader::ObjectParam param{};
		XMStoreFloat4x4(&param.world, XMMatrixTranspose(obj->transform.world));
		XMStoreFloat4x4(&param.texTrans,
			XMMatrixTranspose(obj->transform.texTrans));
		// バッファ転送
		obj->transCb[index]->Update(&param, sizeof(LightingShader::ObjectParam));
	}

#pragma region 追加
	// シャドウマップシーン定数を転送
	shadowSceneCB_[index]->Update(&shadowParam_,
		sizeof(LightingShader::SceneParam));

	commandList->RSSetViewports(1, &smViewport_);
	commandList->RSSetScissorRects(1, &smScissor_);

	auto SrvdTosv = CD3DX12_RESOURCE_BARRIER::Transition(
		shadowMap_.Get(),                        // このテクスチャが
		D3D12_RESOURCE_STATE_GENERIC_READ,       // SRV状態になるまで
		D3D12_RESOURCE_STATE_DEPTH_WRITE);       // デプスステンシルから
	commandList->ResourceBarrier(1, &SrvdTosv);  // 待つ

	// シャドウマップ描画の設定
	commandList->ClearDepthStencilView(shadowMapDsv_, D3D12_CLEAR_FLAG_DEPTH,
		1.0f, 0, 0, nullptr);
	commandList->OMSetRenderTargets(0, nullptr, false, &shadowMapDsv_);

	// オブジェクト描画
	shadowMapShader_->Begin(commandList);
	{
		shadowMapShader_->SetSceneParam(
			shadowSceneCB_[index]->resource()->GetGPUVirtualAddress());

		// 影を落とすオブジェクトを描画
		// 実際のゲームでは事前に視界外のオブジェクトは除外して
		// 描画負荷を抑えたりする
		for (auto& obj : renderObjs_) {
			shadowMapShader_->SetObjectParam(
				obj->transCb[index]->resource()->GetGPUVirtualAddress());
			shadowMapShader_->Apply();
			obj->mesh->Draw(device->graphicsCommandList());
		}
	}
	shadowMapShader_->End();

	// レンダーテクスチャと同じ要領でシャドウマップの
	// ステートが変わるのを待ちます
	auto dsvToSrv = CD3DX12_RESOURCE_BARRIER::Transition(
		shadowMap_.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_GENERIC_READ);
	commandList->ResourceBarrier(1, &dsvToSrv);
#pragma endregion

	// 描画先セット
	// デフォルトのレンダーターゲットはApplicatioクラスでクリアしている
	auto rtv = device->currentRenderTargetView();
	auto dsv = device->depthStencilView();
	commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	auto viewport = device->screenViewport();
	auto scissorRect = device->scissorRect();
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	// 通常の描画
	lightingShader_->Begin(device->graphicsCommandList());
	{
		lightingShader_->SetSceneParam(
			sceneParamCb_[index]->resource()->GetGPUVirtualAddress());

		// サンプラはずっと同じなので設定してしまう
		lightingShader_->SetSamplerDescriptorHeap(samplerHeap_.Get());
		// カラーテクスチャサンプラ
		auto sampler = CD3DX12_GPU_DESCRIPTOR_HANDLE(
			samplerHeap_->GetGPUDescriptorHandleForHeapStart(), 0,
			device->samplerDesctiptorSize());
		lightingShader_->SetTextureSampler(sampler);

#pragma region 追加
		//	シャドウ用サンプラ
		sampler = CD3DX12_GPU_DESCRIPTOR_HANDLE(
			samplerHeap_->GetGPUDescriptorHandleForHeapStart(), 1,
			device->samplerDesctiptorSize());
		lightingShader_->SetShadowMapSampler(sampler);
#pragma endregion

		lightingShader_->SetSrvDescriptorHeap(cbvSrvHeap_.Get());
		auto srvHeap = cbvSrvHeap_->GetGPUDescriptorHandleForHeapStart();
		auto srvDescSize = device->srvDescriptorSize();

#pragma region 追加
		// 影も同じ設定なのでループ外で設定
		// シャドウマップ
		lightingShader_->SetShadowMap(
			CD3DX12_GPU_DESCRIPTOR_HANDLE(srvHeap, 100, srvDescSize));
#pragma endregion

		for (auto& obj : renderObjs_) {
			// バッファ転送
			auto& cbuffer = obj->transCb[index];

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
					lightingShader_->SetTexture(
						CD3DX12_GPU_DESCRIPTOR_HANDLE(srvHeap, srvOffset, srvDescSize));
				}
			}

			// コマンドリスト発行
			lightingShader_->Apply();

			// メッシュ描画コマンド発行
			obj->mesh->Draw(device->graphicsCommandList());
		}
	}
	lightingShader_->End();

#pragma region 追加
	//シャドウのデバッグ用のコード
	debugShader_->Begin(device->graphicsCommandList());
	{
		debugShader_->SetDescriptorHeap(cbvSrvHeap_.Get(), samplerHeap_.Get());

		debugShader_->SetShadowMap(CD3DX12_GPU_DESCRIPTOR_HANDLE(
			cbvSrvHeap_->GetGPUDescriptorHandleForHeapStart(), 100,
			device->srvDescriptorSize()));

		debugShader_->SetSampler(CD3DX12_GPU_DESCRIPTOR_HANDLE(
			samplerHeap_->GetGPUDescriptorHandleForHeapStart(), 0,
			device->samplerDesctiptorSize()));

		debugShader_->Apply();

		quadMesh_->Draw(device->graphicsCommandList());
	}
	debugShader_->End();
#pragma endregion

}

void Scene::Impl::CreateSamplerHeap(Device* device) {
  auto dev = device->device();

  // サンプラー用のデスクリプタヒープを作る
  D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
      D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,  //ヒープタイプはサンプラ
      100,                                 // 作るのは100個
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
    mat->SetRoughness(0.5f);
    mat->SetFresnel({0.4f, 0.4f, 0.4f});
    materials_.emplace("travertine", std::move(mat));
    offset++;
  }
}

#pragma region 追加
void Scene::Impl::CreateDSVHeap(Device* device)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{ D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 10,
								  D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
	device->device()->CreateDescriptorHeap(
		&desc, IID_PPV_ARGS(dsvDescriptorHeap_.ReleaseAndGetAddressOf()));
}

void Scene::Impl::CreateShadowMapSampler(Device* device)
{
	D3D12_SAMPLER_DESC desc{};
	desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	desc.AddressU =
		D3D12_TEXTURE_ADDRESS_MODE_BORDER;  // テクスチャの領域外(1.0以上)
	desc.AddressV =
		D3D12_TEXTURE_ADDRESS_MODE_BORDER;  // は下のBorderColorの色となる
	desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] =
		desc.BorderColor[3] = 1.0f;
	desc.MaxLOD = FLT_MAX;
	desc.MinLOD = -FLT_MAX;
	desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	auto handleSampler = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		samplerHeap_->GetCPUDescriptorHandleForHeapStart(),
		1,  // 0番は使ってるので1番をもらう
		device->samplerDesctiptorSize());

	// サンプラー生成
	device->device()->CreateSampler(&desc, handleSampler);
}

void Scene::Impl::CreateShadowMapObject(Device* device)
{
	auto dev = device->device();

	shadowSceneCB_.resize(device->backBufferSize());
	for (auto& cb : shadowSceneCB_) {
		CreateBufferObject(cb, device->device(),
			sizeof(LightingShader::SceneParam));
	}

	shadowMapShader_ = std::make_unique<ShadowMapShader>();
	shadowMapShader_->Initialize(device);

	CreateDSVHeap(device);
	CreateShadowMapSampler(device);

	// シャドウマップになるデプスステンシルテクスチャを作成
	{
		D3D12_RESOURCE_DESC desc{};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Alignment = 0;
		desc.Width = ShadowMapSize_;
		desc.Height = ShadowMapSize_;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_R24G8_TYPELESS;  // R8G8B8A8ではない！
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		// DXGI_FORMAT_R24G8_TYPELESSでもない
		clearDepth_.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		clearDepth_.DepthStencil.Depth = 1.0f;
		clearDepth_.DepthStencil.Stencil = 0;

		auto type = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		dev->CreateCommittedResource(&type, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			&clearDepth_, IID_PPV_ARGS(&shadowMap_));
	}

	// デプスステンシルビューのデスクリプタハンドルをもらう
	{
		shadowMapDsv_ = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
			0,  // 0番を使う
			dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV));

		D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
		desc.Flags = D3D12_DSV_FLAG_NONE;
		desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.Texture2D.MipSlice = 0;

		// デプスステンシルビュー作成
		dev->CreateDepthStencilView(shadowMap_.Get(), &desc, shadowMapDsv_);
	}

	// シェーダーリソースビューのデスクリプタハンドルをもらう
	{
		// シャドウマップのSRVは通常のテクスチャで使っているcbvSrvHeap_を
		// 使うよ。SRVはデスクリプタテーブルでセットする必要があるが
		// テーブルが読み込むヒープは1個しか渡せないので・・・！めんどい！
		shadowMapSrv_ = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			cbvSrvHeap_->GetCPUDescriptorHandleForHeapStart(),
			100,  // 100番を使う (100番なら開いているだろうという適当な数値）
			dev->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		// 適当に100番を使ったがちゃんと作る時は開いてる番号を取得する
		// 仕組みを作るといいね

		D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MostDetailedMip = 0;
		desc.Texture2D.MipLevels = 1;
		desc.Texture2D.ResourceMinLODClamp = 0.0f;
		desc.Texture2D.PlaneSlice = 0;

		// フォーマットがデプスステンシルのとかわっているので注意
		// ピクセルシェーダからDXGI_FORMAT_D24_UNORM_S8_UINTはアクセスできない
		desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

		// デプスステンシルビュー作成
		dev->CreateShaderResourceView(shadowMap_.Get(), &desc, shadowMapSrv_);
	}

	// デバッグ用
	quadMesh_ = GeometoryMesh::CreateQuad(device->device(), { 0.0f, 0.0f, 0.0f },
		1.0f, 1.0f);
	debugShader_ = std::make_unique<ShadowMapDebugShader>();
	debugShader_->Initialize(device);
}
#pragma endregion

void Scene::Impl::CreateRenderObj(Device* device) {
  // メッシュ作成
  teapotMesh_ = GeometoryMesh::CreateTeapot(device->device());
  floorMesh_ = GeometoryMesh::CreateBox(device->device(), 15.0f, 1.0f, 15.0f);
  wallMesh_ = GeometoryMesh::CreateBox(device->device(), 0.5f, 1.0f, 10.0f);
  pillarMesh_ = GeometoryMesh::CreateBox(device->device(), 1.0f, 10.0f, 1.0f);

  auto bufferSize = device->backBufferSize();
  {
    auto teapot = std::make_unique<RenderObject>();
    teapot->mesh = teapotMesh_.get();
    teapot->transform.texTrans = XMMatrixIdentity();
    teapot->transform.world = XMMatrixIdentity();
    teapot->transCb.resize(bufferSize);
    for (auto& cb : teapot->transCb) {
      CreateBufferObject(cb, device->device(),
                         sizeof(LightingShader::ObjectParam));
    }
    teapot->material = materials_.at("travertine").get();
    renderObjs_.emplace_back(std::move(teapot));
  }

  {
    auto ro = std::make_unique<RenderObject>();
    ro->mesh = floorMesh_.get();
    ro->transform.texTrans = XMMatrixIdentity();
    ro->transform.pos = {0.0f, -1.0f, 0.0f};
    ro->transform.world = XMMatrixTranslation(
        ro->transform.pos.x, ro->transform.pos.y, ro->transform.pos.z);
    ro->transCb.resize(bufferSize);
    for (auto& cb : ro->transCb) {
      CreateBufferObject(cb, device->device(),
                         sizeof(LightingShader::ObjectParam));
    }
    ro->material = materials_.at("grass").get();
    renderObjs_.emplace_back(std::move(ro));
  }

  {
    auto ro = std::make_unique<RenderObject>();
    ro->mesh = wallMesh_.get();
    ro->transform.texTrans = XMMatrixIdentity();
    ro->transform.pos = {-5.0f, 0.0f, 0.0f};
    ro->transform.world = XMMatrixTranslation(
        ro->transform.pos.x, ro->transform.pos.y, ro->transform.pos.z);
    ro->transCb.resize(bufferSize);
    for (auto& cb : ro->transCb) {
      CreateBufferObject(cb, device->device(),
                         sizeof(LightingShader::ObjectParam));
    }
    ro->material = materials_.at("bricks").get();
    renderObjs_.emplace_back(std::move(ro));
  }

  {
    auto ro = std::make_unique<RenderObject>();
    ro->mesh = pillarMesh_.get();
    ro->transform.texTrans = XMMatrixIdentity();
    ro->transform.pos = {3.0f, 0.0f, 2.0f};
    ro->transform.world = XMMatrixTranslation(
        ro->transform.pos.x, ro->transform.pos.y, ro->transform.pos.z);

    ro->transCb.resize(bufferSize);
    for (auto& cb : ro->transCb) {
      CreateBufferObject(cb, device->device(),
                         sizeof(LightingShader::ObjectParam));
    }
    ro->material = materials_.at("bricks").get();
    renderObjs_.emplace_back(std::move(ro));
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
