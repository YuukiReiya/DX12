#include "Scene.hpp"

#include "BufferObject.hpp"
#include "Camera.hpp"
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
}

void Scene::Impl::Render(Device* device) {
  auto index = device->backBufferIndex();
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
