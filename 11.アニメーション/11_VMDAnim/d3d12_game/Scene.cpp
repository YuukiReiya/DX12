#include "Scene.hpp"

#include "BufferObject.hpp"
#include "Camera.hpp"
#include "Device.hpp"
#include "PmdFile.hpp"
#include "TextureManager.hpp"
#include "Utility.hpp"

#include "Model.hpp"

#include "AnimationPlayer.h"

using namespace DirectX;
using namespace Microsoft::WRL;

namespace {

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
  void Update(float deltaTime, Device* device);

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
  std::wstring pmdFilename_{L"Assets/Lat式ミクVer2.31_Sailor夏服.pmd"};
  // std::wstring pmdFilename_{L"Assets/プロ生ちゃん.pmd"};
  // std::wstring pmdFilename_{L"Assets/初音ミクVer2.pmd"};
  PmdFile pmdFile_;  //!< PMDファイル

  //! PmdFileを描画可能な状態にするためのデータ
  std::unique_ptr<Model> model_;

  std::wstring vmdFilename_{L"Assets/wavefile_full_lat.vmd"};

  VmdFile vmdFile_;  //!< VMDファイル

  //! Modelをアニメーションさせるクラス
  std::unique_ptr<AnimationPlayer> animation_;

  //---
  // 細々したものたち
  //---
  FpsCamera camera_;

  SceneParameter sceneParam_;
  std::vector<std::unique_ptr<BufferObject>> sceneCBuffers_;
};

//---

Scene::Impl::Impl(){};

void Scene::Impl::Initialize(Device* device) {
  CreateModel(device);

  vmdFile_.Load(vmdFilename_);
  animation_ = std::make_unique<AnimationPlayer>();
  animation_->Initialize(&vmdFile_);

  animation_->BindModel(model_.get());

  CreateRS(device);
  CreatePSO(device);

  //---
  // カメラとかシーンパラメータとか
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
  pmdFile_.Load(pmdFilename_, device);

  model_ = std::make_unique<Model>();
  model_->Initialize(device, &pmdFile_);
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
  desc.InputLayout = {Model::ModelVertexElement,
                      _countof(Model::ModelVertexElement)};
  desc.pRootSignature = rs_.Get();
  desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  desc.SampleDesc = {1, 0};
  desc.SampleMask = UINT_MAX;
  desc.DSVFormat = device->depthStencil()->GetDesc().Format;
  desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

  device->device()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso_));
}

//---

void Scene::Impl::Update(float deltaTime, Device* device) {
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
        XMFLOAT3 r{};
        model_->rotation(r);
      }
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(700, 5), ImGuiCond_Once);
    ImGui::Begin("Anim");
    {
      if (ImGui::Button("Reset")) {
        animation_->Reset();
      }
      if (ImGui::Button("Pause")) {
        animation_->TogglePause();
      }

      auto speed = animation_->playSpeed();
      ImGui::SliderFloat("Speed", &speed, -2.0f, 2.0f);
      animation_->playSpeed(speed);

      auto enable = animation_->IsAnimationInterprep();
      ImGui::Checkbox("Anim Interp", &enable);
      animation_->EnableAnimationInterprep(enable);

      enable = animation_->IsIK();
      ImGui::Checkbox("IK", &enable);
      animation_->EnableIK(enable);
    }
    ImGui::End();
  }
  camera_.UpdateViewMatrix();

  animation_->Play();
  model_->Update(deltaTime, device);

  {
    // シーン定数バッファ更新
    auto backbufferIndex = device->backBufferIndex();
    sceneParam_.eyePos = camera_.position();
    auto view = camera_.view();
    auto proj = camera_.proj();
    XMStoreFloat4x4(&sceneParam_.view, XMMatrixTranspose(view));
    XMStoreFloat4x4(&sceneParam_.proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&sceneParam_.viewProj, XMMatrixTranspose(view * proj));
    sceneCBuffers_[backbufferIndex]->Update(&sceneParam_, sizeof(sceneParam_));
  }
}

void Scene::Impl::Render(Device* device) {
  auto backbufferIndex = device->backBufferIndex();
  auto commandList = device->graphicsCommandList();

  //---

  ID3D12DescriptorHeap* heaps[]{device->viewDescriptorHeap()->heap()};
  commandList->SetDescriptorHeaps(_countof(heaps), heaps);

  commandList->SetGraphicsRootSignature(rs_.Get());
  commandList->SetPipelineState(pso_.Get());
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  commandList->SetGraphicsRootConstantBufferView(
      1, sceneCBuffers_[backbufferIndex]->resource()->GetGPUVirtualAddress());

  commandList->SetGraphicsRootConstantBufferView(
      0, model_->objectParamBufferAddress());
  commandList->IASetIndexBuffer(model_->indexBufferView());
  commandList->IASetVertexBuffers(0, 1, model_->vertexBufferView());

  const auto count = model_->meshPartsCount();
  for (uint32_t i = 0; i < count; i++) {
    auto& mesh = model_->meshParts(i);
    auto& mat = model_->material(i);

    // マテリアルとテクスチャをセット
    commandList->SetGraphicsRootConstantBufferView(
        2, mat.bufferAddress(backbufferIndex));
    commandList->SetGraphicsRootDescriptorTable(3, mat.textureHandle());

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

void Scene::Update(float deltaTime, Device* device) {
  ImGui_ImplDX12_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  impl_->Update(deltaTime, device);
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
