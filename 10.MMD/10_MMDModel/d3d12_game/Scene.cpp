#include "Scene.hpp"

#include "BufferObject.hpp"
#include "Camera.hpp"
#include "Device.hpp"
#include "PmdFile.hpp"
#include "TextureManager.hpp"
#include "Utility.hpp"

using namespace DirectX;
using namespace Microsoft::WRL;

namespace {
// PMDの頂点フォーマット
// アウトライン描画はしないのでエッジフラグは省略
struct ModelVertex {
  DirectX::XMFLOAT3 position;
  DirectX::XMFLOAT3 normal;
  DirectX::XMFLOAT2 uv;
  DirectX::XMUINT2 boneID;
  DirectX::XMFLOAT2 boneWeights;
};

static constexpr D3D12_INPUT_ELEMENT_DESC ModelVertexElement[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
     D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
     0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"BONEINDICES", 0, DXGI_FORMAT_R32G32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"BONEWEIGHTS", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
     D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
     0},
};

//---

class ModelMaterial {
 public:
  struct Material {
    DirectX::XMFLOAT4 diffuse{};
    DirectX::XMFLOAT4 specular{};
    DirectX::XMFLOAT3 ambient{};
    uint32_t useTexture{};
  };

  ModelMaterial() = default;
  ~ModelMaterial() = default;

  void Initialize(dxapp::Device* device) {
    cbuffers_.resize(device->backBufferSize());
    for (auto& cb : cbuffers_) {
      cb = std::make_unique<dxapp::BufferObject>();
      cb->Initialize(device->device(), dxapp::BufferObjectType::ConstantBuffer,
                     sizeof(material_));
    }
    dirty_ = true;
  }

  void Update(std::uint32_t backbufferIndex) {
    if (!dirty_) return;

    for (auto& cb : cbuffers_) {
      cb->Update(&material_, sizeof(material_));
    }
    dirty_ = false;
  }

  void SetMaterial(const Material& m, dxapp::TextureView view) {
    material_ = m;
    view_ = view;
    dirty_ = true;
  }

  void SetDiffuse(DirectX::XMFLOAT4 diffuse) {
    material_.diffuse = diffuse;
    dirty_ = true;
  }

  void SetSpecular(DirectX::XMFLOAT4 specular) {
    material_.specular = specular;
    dirty_ = true;
  }

  void SetAbient(DirectX::XMFLOAT3 ambient) {
    material_.ambient = ambient;
    dirty_ = true;
  }

  void SetTexture(bool useTexture, dxapp::TextureView view) {
    material_.useTexture = useTexture ? 1 : 0;
    view_ = view;
    dirty_ = true;
  }

  bool HasTexture() const { return material_.useTexture != 0; }

  Material material() const { return material_; }

  D3D12_GPU_VIRTUAL_ADDRESS bufferAddress(std::uint32_t backbufferIndex) const {
    return cbuffers_[backbufferIndex]->resource()->GetGPUVirtualAddress();
  }

  CD3DX12_GPU_DESCRIPTOR_HANDLE textureHandle() const {
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(
        view_.handle.gpuHandle, view_.handle.index,
        view_.handle.allocator->incrementSize());
  }

 private:
  Material material_{};  //!< マテリアルデータ
  std::vector<std::unique_ptr<dxapp::BufferObject>>
      cbuffers_{};             //!< コンスタントバッファ
  dxapp::TextureView view_{};  //!< テクスチャビュー
  bool dirty_{};               //!< バッファアップデートフラグ
};

//---

// MMD用のメッシュを構成するデータ
// 実態は単純に各メッシュのインデックスを覚えているだけ
struct MeshParts {
  uint32_t offset;      //!< オフセット
  uint32_t indexCount;  //!< 使用するインデクス数
};

//---
// オブジェクト定数とかシーン定数の型
struct Transform {
  DirectX::XMFLOAT3 position{};
  DirectX::XMFLOAT3 rotation{};
  DirectX::XMFLOAT3 scale{1.0f, 1.0f, 1.0f};
};

struct ObjectParameter {
  DirectX::XMFLOAT4X4 world;  //!< ワールド
};

struct SceneParameter {
  DirectX::XMFLOAT4X4 view;      //!< ビュー行列
  DirectX::XMFLOAT4X4 proj;      //!< 射影行列
  DirectX::XMFLOAT4X4 viewProj;  //!< ビュー射影
  DirectX::XMFLOAT3 eyePos;      //!< 視点
};
}  // namespace

namespace dxapp {

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
   * @brief モデルデータ構築
   */
  void CreateModel(Device* device);

  /*!
   * @brief パイプライン構築
   */
  void CreatePSO(Device* device);

  /*!
   * @brief ルートシグネチャ構築
   */
  void CreateRS(Device* device);

  //---

  ComPtr<ID3DBlob> vs_;              //!< 頂点シェーダ
  ComPtr<ID3DBlob> ps_;              //!< ピクセルシェーダ
  ComPtr<ID3D12RootSignature> rs_;   //!< ルートシグネチャ
  ComPtr<ID3D12PipelineState> pso_;  //!< パイプラインステート

  //---
  // std::wstring filename_{L"Assets/Lat式ミクVer2.31_White.pmd"};
  std::wstring filename_{L"Assets/プロ生ちゃん.pmd"};
  PmdFile pmdFile_;  //!< PMDファイル

  //---
  // PmdFileを描画可能な状態にするためのデータ群
  //---
  std::vector<MeshParts> mesh_;  //!< パーツを集めてメッシュとする
  std::vector<ModelVertex> vertices_;  //!< PMDから取り出した頂点
  std::vector<uint16_t> indices_;  //!< PMDから取り出したインデクス

  std::unique_ptr<BufferObject> vertexBuffer_;  //!< 頂点バッファ
  std::unique_ptr<BufferObject> indexBuffer_;   //!< インデクスバッファ
  D3D12_VERTEX_BUFFER_VIEW vbView_{};           //!< バッファビュー
  D3D12_INDEX_BUFFER_VIEW ibView_{};            //!< バッファビュー

  std::vector<std::unique_ptr<ModelMaterial>> materials_;  //!< 描画用マテリアル

  //---
  // 細々したものたち
  //---
  FpsCamera camera_;

  Transform transform_;
  ObjectParameter objectParam_;
  std::vector<std::unique_ptr<BufferObject>> objectCBuffers_;

  SceneParameter sceneParam_;
  std::vector<std::unique_ptr<BufferObject>> sceneCBuffers_;
};

//---

Scene::Impl::Impl(){};

void Scene::Impl::Initialize(Device* device) {
  CreateModel(device);
  CreateRS(device);
  CreatePSO(device);

  //---
  // カメラとかシーンパラメータとか
  objectCBuffers_.resize(device->backBufferSize());
  for (auto& cb : objectCBuffers_) {
    cb = std::make_unique<BufferObject>();
    cb->Initialize(device->device(), BufferObjectType::ConstantBuffer,
                   sizeof(ObjectParameter));
  }

  camera_.LookAt({0, 10, -30}, {0, 0, 0}, {0, 1, 0});
  camera_.UpdateViewMatrix();

  sceneCBuffers_.resize(device->backBufferSize());
  for (auto& cb : sceneCBuffers_) {
    cb = std::make_unique<BufferObject>();
    cb->Initialize(device->device(), BufferObjectType::ConstantBuffer,
                   sizeof(SceneParameter));
  }
}

//---

void Scene::Impl::CreateModel(Device* device) {
  // pmdからモデルを構築していくよ
  pmdFile_.Load(filename_, device);

  {  // 頂点データ構築
    auto count = pmdFile_.vertexCount();
    vertices_.resize(count);

    for (uint32_t i = 0; i < count; i++) {
      auto& v = vertices_[i];
      auto data = pmdFile_.vertex(i);

      v.position = data.position;
      v.normal = data.normal;
      v.uv = data.uv;
      v.boneID = XMUINT2(data.boneID[0], data.boneID[1]);
      // boneWeightsは頂点がボーンから受ける影響度（重みといいます）のこと
      // 0～100で入っているので 0.0から1.0
      // で正規化(あとで計算するときに1.0までのほうが楽)
      v.boneWeights.x = data.boneWeight / 100.0f;
      v.boneWeights.y = (100 - data.boneWeight) / 100.0f;
    }

    // 頂点バッファ作成
    vertexBuffer_ = std::make_unique<BufferObject>();
    vertexBuffer_->Initialize(device->device(), BufferObjectType::VertexBuffer,
                              sizeof(ModelVertex) * count);
    vertexBuffer_->Update(vertices_.data(), sizeof(ModelVertex) * count);
    // バッファービュー
    vbView_.BufferLocation = vertexBuffer_->resource()->GetGPUVirtualAddress();
    vbView_.StrideInBytes = sizeof(ModelVertex);
    vbView_.SizeInBytes = vertices_.size() * sizeof(ModelVertex);
  }

  {  // インデクス
    auto count = pmdFile_.indexCount();
    indices_.resize(count);
    for (uint32_t i = 0; i < count; i++) {
      indices_[i] = pmdFile_.index(i);
    }
    // インデクスバッファ
    indexBuffer_ = std::make_unique<BufferObject>();
    indexBuffer_->Initialize(device->device(), BufferObjectType::IndexBuffer,
                             sizeof(uint16_t) * count);
    indexBuffer_->Update(indices_.data(), sizeof(uint16_t) * count);
    // バッファービュー
    ibView_.BufferLocation = indexBuffer_->resource()->GetGPUVirtualAddress();
    ibView_.Format = DXGI_FORMAT_R16_UINT;
    ibView_.SizeInBytes = sizeof(uint16_t) * count;
  }

  {  //マテリアル
    auto count = pmdFile_.materialCount();
    materials_.resize(count);
    for (uint32_t i = 0; i < count; i++) {
      materials_[i] = std::make_unique<ModelMaterial>();
      auto& m = materials_[i];
      m->Initialize(device);

      auto data = pmdFile_.material(i);
      ModelMaterial::Material mat{};
      // Pmdのアルファはディフューズカラーのαにまとめる
      mat.diffuse =
          XMFLOAT4(data.diffuse.x, data.diffuse.y, data.diffuse.z, data.alpha);
      // specularity（スペキュラーの強度）はspecularのαにまとめる
      mat.specular = XMFLOAT4(data.specular.x, data.specular.y, data.specular.z,
                              data.specularity);
      mat.ambient = data.ambient;

      TextureView view{};
      if (data.textureFile.empty()) {
        //　ファイル名がないときはテクスチャなし
        //　ダミーテクスチャを渡しておく
        mat.useTexture = 0;
        auto resource =
            Singleton<TextureManager>::instance().texture("uv_checker");
        view = Singleton<TextureViewManager>::instance().CreateView(
            device, resource.Get());
      } else {
        mat.useTexture = 1;
        auto resource =
            Singleton<TextureManager>::instance().texture(data.textureFile);
        view = Singleton<TextureViewManager>::instance().CreateView(
            device, resource.Get());
      }
      m->SetMaterial(mat, view);
    }
  }

  {  // メッシュ構築
    // mmdはメッシュの頂点がずらっとならんでいるだけなのでそのままだと描画には使えない
    // んでmmdは1マテリアルと1メッシュパーツでできている
    // つまりマテリアル数とメッシュのパーツ数が同じ.
    auto count = pmdFile_.materialCount();

    // マテリアルには使っている頂点数がはいっている.
    // さらに頂点データの並び順はマテリアルの順序と一緒
    // 下図のようになってるわけですな

    // *マテリアルの並びと使う頂点数*
    // |      mat0     |      mat1       |      mat2       |
    // |     100vtx    |     20vtx       |     200vtx      |
    // *頂点の並び*
    // | vtx0 - vtx99  | vtx100 - vtx120 | vtx121 - vtx320 |

    // というわけで、マテリアルを先頭から見ていく
    uint32_t offset = 0;  //!< メッシュパーツが使っている頂点の開始地点
    for (uint32_t i = 0; i < count; i++) {
      auto& data = pmdFile_.material(i);
      // マテリアルの頂点数と、offsetによりパーツで使われている
      // 頂点インデクスがわかりやす
      mesh_.emplace_back(MeshParts{offset, data.vertexCount});
      offset += data.vertexCount;
    }
  }
}

//---

void Scene::Impl::CreateRS(Device* device) {
  CD3DX12_ROOT_PARAMETER rootParams[4]{};
  rootParams[0].InitAsConstantBufferView(0);  // オブジェクト
  rootParams[1].InitAsConstantBufferView(1);  // シーン
  rootParams[2].InitAsConstantBufferView(2);  // マテリアル

  CD3DX12_DESCRIPTOR_RANGE texRange{};
  texRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
  rootParams[3].InitAsDescriptorTable(
      1, &texRange, D3D12_SHADER_VISIBILITY_PIXEL);  // テクスチャ

  //---

  // テクスチャサンプラーにはスタティックサンプラーとダイナミックサンプラーがある
  // いままではダイナミックを使っていた。

  // スタティックサンプラーは作成が容易
  // 下記のようにCD3DX12_STATIC_SAMPLER_DESCを使い
  // RootSignatureの作成に渡すと一緒に作成してもらえる
  CD3DX12_STATIC_SAMPLER_DESC samplerDesc[1]{};
  samplerDesc[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                      D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

  // ただしダイナミックサンプラーと違ってコマンドリストにサンプラーを渡す方法は使えないので
  // サンプラーを動的に切り替える方法は使えなくなるデメリットもある
  CD3DX12_ROOT_SIGNATURE_DESC rsDesc{};
  rsDesc.Init(_countof(rootParams), rootParams, _countof(samplerDesc),
              samplerDesc,
              D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> signature, blob;
  D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
                              &signature, &blob);

  device->device()->CreateRootSignature(0, signature->GetBufferPointer(),
                                        signature->GetBufferSize(),
                                        IID_PPV_ARGS(&rs_));
}

//---

void Scene::Impl::CreatePSO(Device* device) {
  ComPtr<ID3DBlob> error;

  utility::CompileShaderFromFile(L"Shaders/modelVS.hlsl", L"vs_6_0", vs_,
                                 error);
  utility::CompileShaderFromFile(L"Shaders/modelPS.hlsl", L"ps_6_0", ps_,
                                 error);

  D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
  desc.VS = CD3DX12_SHADER_BYTECODE(vs_.Get());
  desc.PS = CD3DX12_SHADER_BYTECODE(ps_.Get());
  desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

  // アルファブレンド（半透明）描画ができるパイプラインとする
  // とりあえずデフォルト地で初期化
  desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

  // この下の設定で通常半透明設定になります
  //どのように半透明のけいさんを行うのかを設定しています

  // アルファブレンドを有効
  desc.BlendState.RenderTarget[0].BlendEnable = TRUE;

  // まずはソースカラー（これから描画する色）と
  // デストカラー・ディスティネーションカラー（描画済みの色）というワードを覚える

  // SrcBlendはソースアルファのブレンド係数
  // D3D12_BLEND_SRC_ALPHAでソースのアルファを使う
  desc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;

  // SrcBlendはソースカラーのブレンド係数
  // D3D12_BLEND_SRC_ALPHAでソースのアルファを使う
  desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;

  // ソースカラーは次の式となる:　SrcR*SrcA, SrcG*SrcA, SrcB*SrcA

  // DestBlendAlphaはソースアルファのブレンド係数
  // D3D12_BLEND_INV_SRC_ALPHAで(1-ソースのアルファ)を使う
  desc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;

  // DestBlendはデストカラーのブレンド係数
  // D3D12_BLEND_INV_SRC_ALPHAで(1-ソースのアルファ)を使う
  desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

  // デストカラーは次の式となる:　DestR*(1-SrcA), DestG*(1-SrcA), DestB*(1-SrcA)

  // 計算したソースとデストカラーの合成式
  // D3D12_BLEND_OP_ADDで上記で計算したソース・デストを足す
  desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
  // ほかにも引き算するD3D12_BLEND_OP_SUBTRACT,小さい値を使うD3D12_BLEND_OP_MIN
  //　大きい値を使うD3D12_BLEND_OP_MAXなどもあります

  desc.NumRenderTargets = 1;
  desc.RTVFormats[0] = device->currentRenderTarget()->GetDesc().Format;
  desc.InputLayout = {ModelVertexElement, _countof(ModelVertexElement)};
  desc.pRootSignature = rs_.Get();
  desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  desc.SampleDesc = {1, 0};
  desc.SampleMask = UINT_MAX;
  desc.DSVFormat = device->depthStencil()->GetDesc().Format;
  desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

  device->device()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso_));
}

//---
void Scene::Impl::Update(float deltaTime) {
  auto keyState = Keyboard::Get().GetState();
  auto mouseState = Mouse::Get().GetState();

  // マウスの右ボタン押してるとカメラ操作
  if (mouseState.rightButton) {
    if (keyState.W) {
      camera_.Dolly(+5.0f * deltaTime);
    }
    if (keyState.S) {
      camera_.Dolly(-5.0f * deltaTime);
    }
    if (keyState.A) {
      camera_.Truck(+5.0f * deltaTime);
    }
    if (keyState.D) {
      camera_.Truck(-5.0f * deltaTime);
    }
    if (keyState.Q) {
      camera_.Boom(-5.0f * deltaTime);
    }
    if (keyState.E) {
      camera_.Boom(+5.0f * deltaTime);
    }
  } else {
    // モデルの回転
    if (keyState.W) {
      transform_.rotation.x += XMConvertToRadians(50 * deltaTime);
    }
    if (keyState.S) {
      transform_.rotation.x -= XMConvertToRadians(50 * deltaTime);
    }
    if (keyState.A) {
      transform_.rotation.y -= XMConvertToRadians(50 * deltaTime);
    }
    if (keyState.D) {
      transform_.rotation.y += XMConvertToRadians(50 * deltaTime);
    }
  }

  {  // Imgui
    ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Once);
    ImGui::Begin("Info");
    {
      ImGui::Text("Framerate(avg) %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      if (ImGui::Button("Camera Reset")) {
        camera_.LookAt({0, 10, -30}, {0, 0, 0}, {0, 1, 0});
        camera_.UpdateViewMatrix();
      }
      if (ImGui::Button("Model Reset")) {
        transform_.rotation.x = 0;
        transform_.rotation.y = 0;
        XMStoreFloat4x4(&objectParam_.world, XMMatrixIdentity());
      }
    }
    ImGui::End();
  }

  camera_.UpdateViewMatrix();

  auto t = XMMatrixTranslation(0, -10, 0);
  auto r = XMMatrixRotationRollPitchYaw(
      transform_.rotation.x, transform_.rotation.y, transform_.rotation.z);
  auto s = XMMatrixScaling(transform_.scale.x, transform_.scale.y,
                           transform_.scale.z);
  auto m = t * s * r;
  XMStoreFloat4x4(&objectParam_.world, XMMatrixTranspose(m));
}

void Scene::Impl::Render(Device* device) {
  auto backbufferIndex = device->backBufferIndex();
  auto commandList = device->graphicsCommandList();

  // 定数バッファの更新系はUnity見たいにPostUpdateみたいなタイミングでやるといいカモメ
  objectCBuffers_[backbufferIndex]->Update(&objectParam_.world,
                                           sizeof(objectParam_));

  {
    sceneParam_.eyePos = camera_.position();
    auto view = camera_.view();
    auto proj = camera_.proj();
    XMStoreFloat4x4(&sceneParam_.view, XMMatrixTranspose(view));
    XMStoreFloat4x4(&sceneParam_.proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&sceneParam_.viewProj, XMMatrixTranspose(view * proj));

    sceneCBuffers_[backbufferIndex]->Update(&sceneParam_, sizeof(sceneParam_));
  }

  for (auto& m : materials_) {
    m->Update(backbufferIndex);
  }

  //---

  ID3D12DescriptorHeap* heaps[]{device->viewDescriptorHeap()->heap()};
  commandList->SetDescriptorHeaps(_countof(heaps), heaps);

  commandList->SetGraphicsRootSignature(rs_.Get());
  commandList->SetPipelineState(pso_.Get());

  commandList->SetGraphicsRootConstantBufferView(
      0, objectCBuffers_[backbufferIndex]->resource()->GetGPUVirtualAddress());
  commandList->SetGraphicsRootConstantBufferView(
      1, sceneCBuffers_[backbufferIndex]->resource()->GetGPUVirtualAddress());

  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->IASetIndexBuffer(&ibView_);
  commandList->IASetVertexBuffers(0, 1, &vbView_);

  auto count = mesh_.size();
  for (uint32_t i = 0; i < count; i++) {
    auto& mat = materials_[i];
    auto& mesh = mesh_[i];

    // マテリアルとテクスチャをセット
    commandList->SetGraphicsRootConstantBufferView(
        2, mat->bufferAddress(backbufferIndex));
    commandList->SetGraphicsRootDescriptorTable(3, mat->textureHandle());

    // メッシュのインデクスを使ってDraw
    commandList->DrawIndexedInstanced(mesh.indexCount, 1, mesh.offset, 0, 0);
  }
}

//-------------------------------------------------------------------
// Sceneの実装
//-------------------------------------------------------------------
Scene::Scene() : impl_(new Impl){};
Scene::~Scene() {}

void Scene::Initialize(Device* device) { impl_->Initialize(device); };

void Scene::Terminate(){};

void Scene::Update(float deltaTime) {
  ImGui_ImplDX12_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  impl_->Update(deltaTime);
};

void Scene::Render(Device* device) {
  auto commandList = device->graphicsCommandList();

  // レンダーターゲットクリア
  auto rtv = device->currentRenderTargetView();
  commandList->ClearRenderTargetView(rtv, Colors::CornflowerBlue, 0, nullptr);

  // デプスバッファクリア
  auto dsv = device->depthStencilView();
  auto clearVaule = device->depthStencilClearValue();
  commandList->ClearDepthStencilView(
      dsv, D3D12_CLEAR_FLAG_DEPTH, clearVaule.DepthStencil.Depth,
      clearVaule.DepthStencil.Stencil, 0, nullptr);

  // コマンドリストにこのフレームでのレンダーターゲットを設定する
  commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

  auto vp = device->screenViewport();
  commandList->RSSetViewports(1, &vp);
  auto sr = device->scissorRect();
  commandList->RSSetScissorRects(1, &sr);

  //---

  impl_->Render(device);

  //---

  // imgui
  ID3D12DescriptorHeap* heap[] = {device->viewDescriptorHeap()->heap()};
  commandList->SetDescriptorHeaps(1, heap);
  ImGui::Render();
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

  D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
      device->currentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET,
      D3D12_RESOURCE_STATE_PRESENT);
  commandList->ResourceBarrier(1, &barrier);

  // コマンドリストはCloseしておかないと実行できませんよ
  commandList->Close();
  // コマンドリストをキューに送る
  device->PushCommandList(reinterpret_cast<ID3D12CommandList*>(commandList));
}
}  // namespace dxapp
