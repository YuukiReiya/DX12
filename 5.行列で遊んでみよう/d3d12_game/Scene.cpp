#include "Scene.hpp"

#include "BasicShader.hpp"
#include "BufferObject.hpp"
#include "Camera.h"
#include "DescriptorHeapStackAllocator.h"
#include "Device.hpp"
#include "GeometoryMesh.hpp"
#include "TextureManager.hpp"

namespace {
/*
 * @brief ワールド行列を作成するためのデータ
 */
struct Transform {
  DirectX::XMFLOAT3 pos{};                               //!< 座標
  DirectX::XMFLOAT3 rot{};                               //!< 回転
  DirectX::XMFLOAT3 sca{1, 1, 1};                        //!< スケール
  DirectX::XMMATRIX world{DirectX::XMMatrixIdentity()};  //!< ワールド行列
};

/*
 * @brief 描画オブジェクト
 */
struct RenderObject {
  // 同じメッシュを何回も作るのはもったいないのでポインタで注入してもらう
  // テクスチャなどと同じような管理を考えるとよいかもね
  dxapp::GeometoryMesh* mesh;  //! メッシュ
  Transform trasnform;         //! オブジェクトのトランスフォーム
  std::vector<std::unique_ptr<dxapp::BufferObject>>
      cbuffer;                             //! コンスタントバッファ
  std::vector<std::uint32_t> heapIndices;  //! デスクリプタヒープの位置
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
   * @brief テクスチャありのRenderObject生成
   */
  void CreateTexObj(Device* device);

  /*
   * @brief SRV生成
   */
  void CreateSrv(Device* device, ID3D12Resource* tex,
                 D3D12_CPU_DESCRIPTOR_HANDLE handle, int offset);

  /*
   * @brief RenderObjectのバッファを初期化
   */
  void InitializeObject(std::vector<RenderObject>& objs, Device* device,
                        DescriptorHeapStackAllocator* heapStack,
                        std::uint32_t bufferCount, std::size_t bufferSize);

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

  // シェーダー
  std::unique_ptr<BasicShader> texShader_;
  std::unique_ptr<BasicNonTexShader> nonTexShader_;

  // デスクリプタヒープ
  // 二つのシェーダーは渡すバッファが異なるのでヒープも二つ
  DescriptorHeapStackAllocator texDescHeap_;
  DescriptorHeapStackAllocator nonTexDescHeap_;

  // サンプラは一個しか作らないのでこれでよし
  ComPtr<ID3D12DescriptorHeap> samplerHeap_{};

  // テクスチャ張れるオブジェクト
  enum class TexObjType {
    Cat = 0,  // 猫ちゃん
    Size,
  };
  std::vector<RenderObject> texObjs_;

  // 生ポリゴンオブジェクト
  void CreateNonTexObj(Device* device);
  enum class NonTexObjType {
    WhiteCube = 0,
    Size,
  };
  std::vector<RenderObject> nonTexObjs_;

  // メッシュデータ
  std::unique_ptr<GeometoryMesh> catMesh_;
  std::unique_ptr<GeometoryMesh> sunMesh_;
  std::unique_ptr<GeometoryMesh> floorMesh_;
  std::unique_ptr<GeometoryMesh> whiteCube_;

  // カメラ
  FpsCamera camera_;
};

Scene::Impl::Impl()
    : texShader_(new BasicShader), nonTexShader_(new BasicNonTexShader){};

void Scene::Impl::Initialize(Device* device) {
  texShader_->Initialize(device->device());
  nonTexShader_->Initialize(device->device());

  catMesh_ = GeometoryMesh::CreateCube(device->device());
  sunMesh_ = GeometoryMesh::CreateSphere(device->device());
  floorMesh_ = GeometoryMesh::CreateBox(device->device(), 10, 1, 10,
                                        {0.5f, 0.5f, 0.5f, 1.0f});

  whiteCube_ = GeometoryMesh::CreateCube(device->device());

  // 必要そうなテクスチャを読もう
  Singleton<TextureManager>::instance().LoadWICTextureFromFile(
      device, L"Assets/cat.png", "cat");

  Singleton<TextureManager>::instance().LoadWICTextureFromFile(
      device, L"Assets/earth.png", "earth");

  Singleton<TextureManager>::instance().LoadWICTextureFromFile(
      device, L"Assets/moon.png", "moon");

  Singleton<TextureManager>::instance().LoadWICTextureFromFile(
      device, L"Assets/sun.png", "sun");

  CreateSamplerHeap(device);
  CreateCbvSrvHeap(device);

  CreateTexObj(device);
  CreateNonTexObj(device);

  camera_.LookAt({0, 5, -5}, {0, 0, 0}, {0, 1, 0});
  camera_.UpdateViewMatrix();

  Mouse::Get().SetMode(Mouse::MODE_RELATIVE);
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
    // ネコちゃんがあるくよ
  }

  // 白い箱オブジェクト更新（原点でY軸回転）
  {
    nonTexObjs_[0].trasnform.rot.y +=
        static_cast<float>(DirectX::XM_PIDIV2 * deltaTime);

    auto& trans = nonTexObjs_[0].trasnform;
    XMMATRIX s = XMMatrixScaling(trans.sca.x, trans.sca.y, trans.sca.z);
    auto r = XMMatrixRotationY(trans.rot.y);
    auto t = XMMatrixTranslation(0, 0, 0);
    trans.world = t * r;
  }
}

void Scene::Impl::Render(Device* device) {
  auto buckBufferIndex = device->backBufferIndex();

  auto vp = camera_.view() * camera_.proj();

  texShader_->Begin(device->graphicsCommandList());
  auto srvPos = static_cast<int>(BasicShaderResourceIndex::Srv);
  for (int i = 0; i < texObjs_.size(); i++) {
    auto& obj = texObjs_[i];

    BasicShaderCB param{};
    auto w = obj.trasnform.world;
    auto wvp = w * vp;

    w = XMMatrixTranspose(w);
    XMStoreFloat4x4(&param.world, w);
    wvp = XMMatrixTranspose(wvp);
    XMStoreFloat4x4(&param.wvp, wvp);

    // バッファに書き込み
    auto& cbuffer = obj.cbuffer[buckBufferIndex];
    cbuffer->Update(&param, sizeof(BasicShaderCB));

    // シェーダー側のコマンド発行
    texShader_->SetCBufferDescriptorHeap(texDescHeap_.heap(),
                                         obj.heapIndices[buckBufferIndex]);
    texShader_->SetSrvDescriptorHeap(texDescHeap_.heap(),
                                     obj.heapIndices[buckBufferIndex] + srvPos);
    texShader_->SetSamplerDescriptorHeap(samplerHeap_.Get(), 0);

    texShader_->Apply();

    // メッシュ自体のコマンド
    obj.mesh->Draw(device->graphicsCommandList());
  }
  texShader_->End();

  // テクスチャなし（上のこぴぺ。よくない）
  // 上と同じ挙動だけど範囲forを使うとよりらくちん
  nonTexShader_->Begin(device->graphicsCommandList());
  for (auto& obj : nonTexObjs_) {
    BasicShaderCB param{};
    auto w = obj.trasnform.world;
    auto wvp = w * vp;

    w = XMMatrixTranspose(w);
    XMStoreFloat4x4(&param.world, w);
    wvp = XMMatrixTranspose(wvp);
    XMStoreFloat4x4(&param.wvp, wvp);

    // バッファに書き込み
    auto& cbuffer = obj.cbuffer[buckBufferIndex];
    cbuffer->Update(&param, sizeof(BasicShaderCB));

    // シェーダー側のコマンド発行
    nonTexShader_->SetCBufferDescriptorHeap(nonTexDescHeap_.heap(),
                                            obj.heapIndices[buckBufferIndex]);
    nonTexShader_->Apply();
    obj.mesh->Draw(device->graphicsCommandList());
  }
  nonTexShader_->End();
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

  // テクスチャ付き
  texDescHeap_.Initialize(
      dev, static_cast<std::uint32_t>(BasicShaderResourceIndex::Size));

  // テクスチャなし
  nonTexDescHeap_.Initialize(
      dev, static_cast<std::uint32_t>(BasicNonTexShaderResourceIndex::Size));
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

void Scene::Impl::InitializeObject(std::vector<RenderObject>& objs,
                                   Device* device,
                                   DescriptorHeapStackAllocator* heapStack,
                                   std::uint32_t bufferCount,
                                   std::size_t bufferSize) {
  for (auto& obj : objs) {
    obj.cbuffer.resize(bufferCount);
    obj.heapIndices.resize(bufferCount);

    int i = 0;
    for (auto& cbuffer : obj.cbuffer) {
      CreateBufferObject(cbuffer, device->device(), bufferSize);
      // ヒープの位置を取得
      obj.heapIndices[i] = heapStack->pop();

      CreateBufferView(cbuffer, device, heapStack->heapStart(),
                       obj.heapIndices[i]);
      i++;
    }
  }
}

void Scene::Impl::CreateTexObj(Device* device) {
  auto bufferCount = device->backBufferSize();
  auto cbSize = sizeof(BasicShaderCB);

  // CBVを作るよ
  texObjs_.resize(static_cast<int>(TexObjType::Size));
  InitializeObject(texObjs_, device, &texDescHeap_, bufferCount, cbSize);

  // SRVはオブジェクトごとの設定をがんばる
  auto srvPos = static_cast<int>(BasicShaderResourceIndex::Srv);

  auto tex = Singleton<TextureManager>::instance().texture("cat");
  auto type = static_cast<int>(TexObjType::Cat);
  texObjs_[type].mesh = catMesh_.get();
  // BufferObjectが2個あるのでループ回す
  for (auto index : texObjs_[type].heapIndices) {
    auto offset = index + srvPos;
    CreateSrv(device, tex.Get(), texDescHeap_.heapStart(), offset);
  }
}

void Scene::Impl::CreateNonTexObj(Device* device) {
  auto bufferCount = device->backBufferSize();
  auto cbSize = sizeof(BasicShaderCB);

  // CBVを作るよ
  nonTexObjs_.resize(static_cast<int>(NonTexObjType::Size));
  InitializeObject(nonTexObjs_, device, &nonTexDescHeap_, bufferCount, cbSize);

  auto type = static_cast<int>(NonTexObjType::WhiteCube);
  nonTexObjs_[type].mesh = whiteCube_.get();
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
