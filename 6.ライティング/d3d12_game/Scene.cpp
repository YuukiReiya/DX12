#include "Scene.hpp"

#include "BufferObject.hpp"
#include "Camera.hpp"
#include "Device.hpp"
#include "GeometoryMesh.hpp"
#include "TextureManager.hpp"
#pragma region add_1112
#include "LightingShader.hpp"
#pragma endregion
namespace {
#pragma region add_1112
	/*
	 * @brief モデルの質感を表すデータ
	 */
	class Material {
	public:
		struct MaterialParameter {
			DirectX::XMFLOAT4 diffuseAlbedo{ 1, 1, 1, 1 };
			DirectX::XMFLOAT3 fresnel{ 0.5f, 0.5f, 0.5f };
			float roughness{ 0.1f };
			int useTexture = 0;
		};

		/*
		 * @brief コンストラクタ
		 */
		Material() = default;
		Material(const MaterialParameter& param) : material_(param) {}

		/*
		 * @brief 初期化
		 */
		void Initialize(dxapp::Device* device) {
			// マテリアルは変化しなさそうだけど、、、
			// 点滅したりテクスチャがスクロールするので一応ダブルバッファ化
			matCb_.resize(device->backBufferSize());
			for (auto& cb : matCb_) {
				cb = std::make_unique<dxapp::BufferObject>();
				cb->Initialize(device->device(), dxapp::BufferObjectType::ConstantBuffer,
					sizeof(material_));
			}
		}

		void Update(std::uint32_t index) {
			matCb_[index]->Update(&material_, sizeof(MaterialParameter));
		}

		// 値の範囲チェックとかを本当はするんだよ

		/*
		 * @brief 拡散反射光の設定
		 */
		void SetDiffuseAlbedo(DirectX::XMFLOAT4 diffuseAlbedo) {
			material_.diffuseAlbedo = diffuseAlbedo;
		}

		/*
		 * @brief 鏡面反射光の設定
		 */
		void SetFresnel(DirectX::XMFLOAT3 fresnel) { material_.fresnel = fresnel; }

		/*
		 * @brief 面の粗さ設定
		 */
		void SetRoughness(float roughness) { material_.roughness = roughness; }

		/*
		 * @brief テクスチャの設定
		 */
		void SetTexture(ID3D12DescriptorHeap* heap, std::uint32_t offset) {
			srvHeap_ = heap;
			srvOffset_ = offset;
			material_.useTexture = 1;
		}

		/*
		 * @brief テクスチャの解除
		 */
		void ClearTexture() {
			srvHeap_ = nullptr;
			srvOffset_ = 0;
			material_.useTexture = 0;
		}

		/*
		 * @brief テクスチャのヒープとオフセット位置を取得
		 */
		void textureDescHeap(ID3D12DescriptorHeap** heap, std::uint32_t* offset) {
			*heap = srvHeap_;
			*offset = srvOffset_;
		}

		/*
		 * @brief テクスチャの割り当て確認
		 */
		bool HasTexture() const { return (material_.useTexture != 0); }

		/*
		 * @brief このマテリアルの定数バッファを取得
		 */
		D3D12_GPU_VIRTUAL_ADDRESS materialCb(std::uint32_t index) const {
			return matCb_[index]->resource()->GetGPUVirtualAddress();
		}

	private:
		MaterialParameter material_;  //! 定数バッファに書き込む値
		std::vector<std::unique_ptr<dxapp::BufferObject>>
			matCb_{};  //! MaterialParameterの定数バッファ領域
		ID3D12DescriptorHeap* srvHeap_{ nullptr };  //! SRVデスクリプタヒープ
		std::uint32_t srvOffset_{ 0 };              //! アドレスオフセット
	};
#pragma endregion
/*
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

/*
 * @brief 描画オブジェクト
 */
struct RenderObject {
  Transform transform;  //! オブジェクトのトランスフォーム
  std::vector<std::unique_ptr<dxapp::BufferObject>>
      transCb;  //! transformの定数バッファへのポインタ

  // 下のデータはほかのオブジェクトと共有できる情報なのでポインタでもらっておく
  dxapp::GeometoryMesh* mesh;  //! メッシュ

#pragma region add_1112
  Material* material;  //! マテリアル
#pragma endregion
};
}  // namespace

namespace dxapp {
using namespace DirectX;
using namespace Microsoft::WRL;

/*
 * @brief Sceneクラスの内部実装
 */
class Scene::Impl {
 public:
  Impl();
  ~Impl() = default;

  /*
   * @brief 初期化
   */
  void Initialize(Device* device);

  /*
   * @brief 描画
   */
  void Render(Device* device);

  /*
   * @brief 更新
   */
  void Update(float deltaTime);

 private:
  /*
   * @brief CBV/SRVデスクリプタヒープ生成
   */
  void CreateCbvSrvHeap(Device* device);

  /*
   * @brief サンプラーデスクリプタヒープ生成
   */
  void CreateSamplerHeap(Device* device);

  /*
   * @brief SRV生成
   */
  void CreateSrv(Device* device, ID3D12Resource* tex,
                 D3D12_CPU_DESCRIPTOR_HANDLE handle, int offset);

  /*
   * @brief BufferObject生成
   */
  void CreateBufferObject(std::unique_ptr<BufferObject>& buffer,
                          ID3D12Device* device, std::size_t bufferSize);

  /*
   * @brief BufferObjectからViewを生成
   */
  void CreateBufferView(std::unique_ptr<BufferObject>& buffer, Device* device,
                        D3D12_CPU_DESCRIPTOR_HANDLE heapStart, int offset);

#pragma region add_1112
  /*
   * @brief テクスチャありのRenderObject生成
   */
  void CreateRenderObj(Device* device);

  /*
   * @brief テクスチャありのRenderObject生成
   */
  void CreateMaterial(Device* device);
#pragma endregion
  /*
   * デスクリプタヒープの管理周りは、いろいろと間違っていた！
   * すまん！！
   *
   * なのでいったん今までのようにID3D12DescriptorHeapをそのまま使うよ
   */
  // サンプラ デスクリプタヒープ
  ComPtr<ID3D12DescriptorHeap> samplerHeap_{};
  // CBV/SRV デスクリプタヒープ
  ComPtr<ID3D12DescriptorHeap> cbvSrvHeap_{};

  // カメラ
  FpsCamera camera_;
#pragma region add_1112
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
#pragma endregion
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
    Singleton<TextureManager>::instance().LoadWICTextureFromFile(
        device, L"Assets/bricks.png", "bricks");

    Singleton<TextureManager>::instance().LoadWICTextureFromFile(
        device, L"Assets/fabric.png", "fabric");

    Singleton<TextureManager>::instance().LoadWICTextureFromFile(
        device, L"Assets/grass.png", "grass");

    Singleton<TextureManager>::instance().LoadWICTextureFromFile(
        device, L"Assets/travertine.png", "travertine");

    Singleton<TextureManager>::instance().LoadWICTextureFromFile(
        device, L"Assets/uv_checker.png", "uv_checker");
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
  {
	  // 環境光（最低限これくらいは明るい）
	  sceneParam_.ambientLight = { 0.25f, 0.25f, 0.25f, 1.0f };

	  // メインのライト（キーライトといいます）
	  // 光の向き
	  sceneParam_.lights[0].direction = { 0.57f, -0.57f, 0.57f };
	  // メインなのでライトの明るさがほかのより強い
	  sceneParam_.lights[0].strength = { 0.8f, 0.8f, 0.8f };

	  // 補助ライト1（フィルライト）
	  // キーライトの反対側において、キーライトの陰を弱くします
	  sceneParam_.lights[1].direction = { -0.57f, -0.57f, 0.57f };
	  // ライトはメインより暗くします
	  sceneParam_.lights[1].strength = { 0.4f, 0.4f, 0.4f };

	  // 補助ライト2（バックライト）
	  // ほんとはオブジェクトの反対側において輪郭を浮き上がらせるためのライト
	  // 動かすのが面倒なのでいまは固定
	  sceneParam_.lights[2].direction = { 0.0f, -0.707f, -0.707f };
	  // ライトは暗め
	  sceneParam_.lights[2].strength = { 0.2f, 0.2f, 0.2f };

	  // SceneParamの定数バッファを作成
	  sceneParamCb_.resize(device->backBufferSize());
	  for (auto& cb : sceneParamCb_) {
		  CreateBufferObject(cb, device->device(),
			  sizeof(LightingShader::SceneParam));
	  }
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
  }
  camera_.UpdateViewMatrix();

  // 今回はティーポットは回るだけ。動かしたい人は自分で追加してみよう
  {
	  auto& transform = renderObjs_[0]->transform;
	  transform.rot.y += XMConvertToRadians(45.f * deltaTime);
	  auto rotY = XMMatrixRotationY(transform.rot.y);

	  // 0度の時にティーポットの先端がZ+を向くように回転補正
	  auto fixRot = XMMatrixRotationY(XMConvertToRadians(180.f));

	  transform.world = fixRot * rotY;
  }
#pragma region 追記
  // 2個目
  {
	  auto& transform = renderObjs_[1]->transform;
	  transform.rot.y += XMConvertToRadians(45.f * deltaTime);
	  auto rotY = XMMatrixRotationY(transform.rot.y);

	  // x = +2の位置
	  auto trans = XMMatrixTranslation(2.f, 0.f, 0.f);
	  // 0度の時にティーポットの先端がZ+を向くように回転補正
	  auto fixRot = XMMatrixRotationY(XMConvertToRadians(180.f));

	  transform.world = fixRot * rotY * trans;
  }

  // 3個目
  {
	  auto& transform = renderObjs_[2]->transform;
	  transform.rot.y += XMConvertToRadians(45.f * deltaTime);
	  auto rotY = XMMatrixRotationY(transform.rot.y);

	  // x = -2の位置
	  auto trans = XMMatrixTranslation(-2.f, 0.f, 0.f);
	  // 0度の時にティーポットの先端がZ+を向くように回転補正
	  auto fixRot = XMMatrixRotationY(XMConvertToRadians(180.f));

	  transform.world = fixRot * rotY * trans;
  }
#pragma endregion

#pragma region 課題2
  if (keyState.L)
  {
	  // 1 初期状態(マジックナンバーで実装しないでほしかった。。。)
	  if(keyState.D1)
	  {
		  sceneParam_.lights[0].direction = { 0.57f, -0.57f, 0.57f };
		  sceneParam_.lights[0].strength = { 0.8f, 0.8f, 0.8f };
		  sceneParam_.lights[1].direction = { -0.57f, -0.57f, 0.57f };
		  sceneParam_.lights[1].strength = { 0.4f, 0.4f, 0.4f };
		  sceneParam_.lights[2].direction = { 0.0f, -0.707f, -0.707f };
		  sceneParam_.lights[2].strength = { 0.2f, 0.2f, 0.2f };
	  }
	  // 2
	  if (keyState.D2)
	  {
		  constexpr XMFLOAT3 mainLightDir = { 1,1,1, };
		  constexpr XMFLOAT3 fillLightDir = { 1,1,1, };
		  constexpr XMFLOAT3 backLightDir = { 1,1,1, };
		  constexpr float strength = 1.0f;
		  sceneParam_.lights[0].direction = mainLightDir;
		  sceneParam_.lights[0].strength = { strength, strength, strength };
		  sceneParam_.lights[1].direction = fillLightDir;
		  sceneParam_.lights[1].strength = { strength / 2, strength / 2, strength / 2 };
		  sceneParam_.lights[2].direction = backLightDir;
		  sceneParam_.lights[2].strength = { strength / 4, strength / 4, strength / 4 };
	  }
	  // 3
	  if (keyState.D3)
	  {
		  sceneParam_.lights[0].direction = { 0.2f, -0.2f, 0.2f };
		  sceneParam_.lights[0].strength = { 0.2f, 0.4f, 0.8f };
		  sceneParam_.lights[1].direction = { -0.2f, -0.2f, 0.2f };
		  sceneParam_.lights[1].strength = { 0.8f, 0.4f, 0.2f };
		  sceneParam_.lights[2].direction = { 0.0f, -0.707f, -0.707f };
		  sceneParam_.lights[2].strength = { 0.4f, 0.4f, 0.4f };

	  }
  }
#pragma endregion
}

void Scene::Impl::Render(Device* device) {
  auto index = device->backBufferIndex();

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
};

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

void Scene::Impl::CreateRenderObj(Device* device)
{
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
		// マテリアルを設定
		teapot->material = materials_.at("travertine").get();
		renderObjs_.emplace_back(std::move(teapot));
	}

#pragma region 追記

	// 基本は上と同じ
	{
		auto teapot = std::make_unique<RenderObject>();
		teapot->mesh = teapotMesh_.get();
		teapot->transform.texTrans = XMMatrixIdentity();
		teapot->transCb.resize(bufferSize);
		for (auto& cb : teapot->transCb) {
			CreateBufferObject(cb, device->device(),
				sizeof(LightingShader::ObjectParam));
		}
		// マテリアルをfabricに
		//teapot->material = materials_.at("fabric").get();
		
		//マテリアル変更
		//課題1
		teapot->material = materials_.at("sample1").get();

		renderObjs_.emplace_back(std::move(teapot));
	}

	// 基本は上と同じ
	{
		auto teapot = std::make_unique<RenderObject>();
		teapot->mesh = teapotMesh_.get();
		teapot->transform.texTrans = XMMatrixIdentity();
		teapot->transCb.resize(bufferSize);
		for (auto& cb : teapot->transCb) {
			CreateBufferObject(cb, device->device(),
				sizeof(LightingShader::ObjectParam));
		}
		// マテリアルをbricksに
		//teapot->material = materials_.at("bricks").get();
		
		//マテリアル変更
		//課題1
		teapot->material = materials_.at("sample2").get();

		renderObjs_.emplace_back(std::move(teapot));
	}
#pragma endregion

#pragma region 課題
	//add_mat1
	{

	}
#pragma endregion

}

void Scene::Impl::CreateMaterial(Device* device)
{
	// マテリアルを作ります
 // とりあえずすべてマテリアルはデフォルトパラメータ
 // テクスチャだけが違う状態

	int offset = 0; // デスクリプタヒープの位置
	{
		auto t = Singleton<TextureManager>::instance().texture("uv_checker");

		CreateSrv(device, t.Get(),
			cbvSrvHeap_->GetCPUDescriptorHandleForHeapStart(), offset);
		auto mat = std::make_unique<Material>();
		mat->Initialize(device);
		mat->SetTexture(cbvSrvHeap_.Get(), offset);

		materials_.emplace("uv_checker", std::move(mat));
		offset++;
	}

	//	bricks
	{
		auto t = Singleton<TextureManager>::instance().texture("bricks");

		CreateSrv(device, t.Get(),
			cbvSrvHeap_->GetCPUDescriptorHandleForHeapStart(), offset);
		auto mat = std::make_unique<Material>();
		mat->Initialize(device);
		mat->SetTexture(cbvSrvHeap_.Get(), offset);

		materials_.emplace("bricks", std::move(mat));
		offset++;
	}

	//	fabric
	{
		auto t = Singleton<TextureManager>::instance().texture("fabric");

		CreateSrv(device, t.Get(),
			cbvSrvHeap_->GetCPUDescriptorHandleForHeapStart(), offset);
		auto mat = std::make_unique<Material>();
		mat->Initialize(device);
		mat->SetTexture(cbvSrvHeap_.Get(), offset);

#pragma region 追記
		mat->SetRoughness(0.9f);
		mat->SetFresnel({ 0.05f, 0.05f, 0.05f });
#pragma endregion


		materials_.emplace("fabric", std::move(mat));
		offset++;
	}

	//	grass
	{
		auto t = Singleton<TextureManager>::instance().texture("grass");

		CreateSrv(device, t.Get(),
			cbvSrvHeap_->GetCPUDescriptorHandleForHeapStart(), offset);
		auto mat = std::make_unique<Material>();
		mat->Initialize(device);
		mat->SetTexture(cbvSrvHeap_.Get(), offset);

		materials_.emplace("grass", std::move(mat));
		offset++;
	}

	//	travertine
	{
		auto t = Singleton<TextureManager>::instance().texture("travertine");

		CreateSrv(device, t.Get(),
			cbvSrvHeap_->GetCPUDescriptorHandleForHeapStart(), offset);
		auto mat = std::make_unique<Material>();
		mat->Initialize(device);
		mat->SetTexture(cbvSrvHeap_.Get(), offset);

		materials_.emplace("travertine", std::move(mat));
		offset++;
	}

#pragma region 課題
	//sample1
	{
		auto t = Singleton<TextureManager>::instance().texture("travertine");

		CreateSrv(device, t.Get(),
			cbvSrvHeap_->GetCPUDescriptorHandleForHeapStart(), offset);
		auto mat = std::make_unique<Material>();
		mat->Initialize(device);
		mat->SetTexture(cbvSrvHeap_.Get(), offset);

		mat->SetFresnel({ 0.1f, 0.9f, 0.1f });

		materials_.emplace("sample1", std::move(mat));
		offset++;
	}

	//sample2
	{
		auto t = Singleton<TextureManager>::instance().texture("travertine");

		CreateSrv(device, t.Get(),
			cbvSrvHeap_->GetCPUDescriptorHandleForHeapStart(), offset);
		auto mat = std::make_unique<Material>();
		mat->Initialize(device);
		mat->SetTexture(cbvSrvHeap_.Get(), offset);

		mat->SetDiffuseAlbedo({ 0.2f, 0.2f, 0.2f ,1.0f });

		materials_.emplace("sample2", std::move(mat));
		offset++;
	}

	//追加マテリアル
	{

	}
#pragma endregion

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
