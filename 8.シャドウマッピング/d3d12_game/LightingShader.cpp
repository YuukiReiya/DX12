#include "LightingShader.hpp"

#include "BufferObject.hpp"
#include "Device.hpp"
#include "Utility.hpp"
#include "VertexType.hpp"

namespace dxapp {
#pragma region LightingShader::Impl
/*!
 * @brief LightingShaderの実装
 */
class LightingShader::Impl {
 public:
  /*!
   * @brief コンストラクタ
   */
  Impl() = default;
  /*!
   * @brief デストラクタ
   */
  ~Impl() = default;

  /*!
   * @brief シェーダー生成
   */
  void CreateShader();

  /*!
   * @brief ルートシグネチャ生成
   */
  void CreatRootSignature(ID3D12Device* device);

  /*!
   * @brief パイプライン生成
   */
  void CreatePipelineState(ID3D12Device* device);

  //! 頂点シェーダ
  Microsoft::WRL::ComPtr<ID3DBlob> vs_{};
  //! ピクセルシェーダ
  Microsoft::WRL::ComPtr<ID3DBlob> ps_{};
  //! ルートシグネチャ
  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_{};
  //! パイプラインステート
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_{};

  // デスクリプタサイズのキャッシュ
  UINT srvDescriptorSize_{}, samplerDescriptorSize_{};

  // Begin/Endで使うコマンドリスト
  ID3D12GraphicsCommandList* commandList{};

  // コマンドリストに積むデスクリプタ・GPUハンドルのキャッシュ
  ID3D12DescriptorHeap *srvHeap_{}, *samplerHeap_{};
  D3D12_GPU_DESCRIPTOR_HANDLE srv_, sampler_;

  // コマンドリストに積む定数バッファへのGPUハンドルのキャッシュ
  D3D12_GPU_VIRTUAL_ADDRESS objParam_, sceneParam_, matParam_;

  // テクスチャなしの時でも動かす。しかしちゃんとヒープやハンドルは渡す必要がある
  ID3D12DescriptorHeap* defaultSamplerHeap_{};
  CD3DX12_GPU_DESCRIPTOR_HANDLE defaultSampler_;

  // テクスチャなしの時でも動かす。しかしちゃんとヒープやハンドルは渡す必要がある
  ID3D12DescriptorHeap* dammySrvHeap_{};
  CD3DX12_GPU_DESCRIPTOR_HANDLE dammySrv_;
};

void LightingShader::Impl::CreateShader() {
  {
    Microsoft::WRL::ComPtr<ID3DBlob> error;
    utility::CompileShaderFromFile(L"Shaders/LightingVS.hlsl", L"vs_6_0", vs_,
                                   error);
    utility::CompileShaderFromFile(L"Shaders/LightingPS.hlsl", L"ps_6_0", ps_,
                                   error);
  }
}

// ルートシグネチャ生成
void LightingShader::Impl::CreatRootSignature(ID3D12Device* device) {
  CD3DX12_ROOT_PARAMETER rootParams[5];

  //-----------------------------------------------------------------
  // BasicShaderとは違い定数バッファ部分をInitAsConstantBufferViewで作成。
  // InitAsConstantBufferViewはDescriptorTableを経由せずに
  // 直接シェーダーに渡せるのがメリット。
  // ただし渡せるパラメータ数が減ったり、多分速度的には遅いなどのデメリットもある。
  // 現状は学習用なので良しにする。
  //-----------------------------------------------------------------
  // 引数の数字はレジスタ番号.
  rootParams[0].InitAsConstantBufferView(0);  // b0:ObjectParam
  rootParams[1].InitAsConstantBufferView(1);  // b1:SceneParam
  rootParams[2].InitAsConstantBufferView(2);  // b2:MaterialParam

  // テクスチャとサンプラーは今まで通りデスクリプタテーブルで渡す
  CD3DX12_DESCRIPTOR_RANGE srv, sampler;
  srv.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
  sampler.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

  rootParams[3].InitAsDescriptorTable(1, &srv, D3D12_SHADER_VISIBILITY_PIXEL);
  rootParams[4].InitAsDescriptorTable(1, &sampler,
                                      D3D12_SHADER_VISIBILITY_PIXEL);

  CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc{};
  rootSigDesc.Init(
      _countof(rootParams), rootParams, 0, nullptr,
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  Microsoft::WRL::ComPtr<ID3DBlob> signature;
  Microsoft::WRL::ComPtr<ID3DBlob> error;
  D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
                              &signature, &error);

  device->CreateRootSignature(0, signature->GetBufferPointer(),
                              signature->GetBufferSize(),
                              IID_PPV_ARGS(&rootSignature_));
}

// パイプラインステートオブジェクト生成
void LightingShader::Impl::CreatePipelineState(ID3D12Device* device) {
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
  // シェーダーのセット
  psoDesc.VS = CD3DX12_SHADER_BYTECODE(vs_.Get());
  psoDesc.PS = CD3DX12_SHADER_BYTECODE(ps_.Get());
  // ブレンドステート設定
  psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  // ラスタライザーステート
  psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  // 出力先は1ターゲット
  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  psoDesc.InputLayout = {VertexPositionColorNormalTexturElement,
                         _countof(VertexPositionColorNormalTexturElement)};

  // ルートシグネチャのセット
  psoDesc.pRootSignature = rootSignature_.Get();
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  // マルチサンプル設定
  psoDesc.SampleDesc = {1, 0};
  psoDesc.SampleMask = UINT_MAX;

  // デプスバッファのフォーマットを設定
  psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
  psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

  device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline_));
}
#pragma endregion

//-------------------------------------------------------------------
// LightingShader
//-------------------------------------------------------------------
LightingShader::LightingShader() : impl_(new Impl) {}

LightingShader::~LightingShader() {}

void LightingShader::Initialize(Device* device) {
  auto dev = device->device();

  impl_->CreateShader();
  impl_->CreatRootSignature(dev);
  impl_->CreatePipelineState(dev);

  impl_->srvDescriptorSize_ = dev->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  impl_->samplerDescriptorSize_ =
      dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

void LightingShader::Terminate() {}

void LightingShader::Begin(ID3D12GraphicsCommandList* commandList) {
  assert(impl_->commandList == nullptr);
  impl_->commandList = commandList;

  // シェーダー内部ではテクスチャ参照はフラグで切り分けている
  // しかしリソースは送る必要がある、そこで描画開始時に適当なテクスチャと
  // デフォルトのサンプラーを渡してあげる。とりあえずこれで止まることはなくなる。
  // 実際のゲームで使うならこの辺はもっと工夫がいるね

  // さんぷらーにデフォルト値を設定
  impl_->samplerHeap_ = impl_->defaultSamplerHeap_;
  impl_->sampler_ = impl_->defaultSampler_;

  // とりあえずダミーを渡しておく
  impl_->srvHeap_ = impl_->dammySrvHeap_;
  impl_->srv_ = impl_->dammySrv_;

  commandList->SetGraphicsRootSignature(impl_->rootSignature_.Get());
  commandList->SetPipelineState(impl_->pipeline_.Get());
}

void LightingShader::End() {
  assert(impl_->commandList);
  impl_->commandList = nullptr;
}

void LightingShader::Apply() {
  ID3D12DescriptorHeap* heaps[] = {impl_->srvHeap_, impl_->samplerHeap_};

  // _countofは生配列の要素数を数えてくれるマクロ
  impl_->commandList->SetDescriptorHeaps(_countof(heaps), heaps);

  // 定数バッファはデスクリプタテーブルを使ってない
  // その場合はSetGraphicsRootConstantBufferViewを使って直接アドレスを投げる

  // b0: ObjectParam
  impl_->commandList->SetGraphicsRootConstantBufferView(
      0,                  // シェーダのレジスタ番号
      impl_->objParam_);  // GPU側のアドレス

  // b1: SceneParam
  impl_->commandList->SetGraphicsRootConstantBufferView(1, impl_->sceneParam_);

  // b2: MaterialParam
  impl_->commandList->SetGraphicsRootConstantBufferView(2, impl_->matParam_);

  // テクスチャとサンプラはSetGraphicsRootDescriptorTableで設定
  impl_->commandList->SetGraphicsRootDescriptorTable(3, impl_->srv_);
  impl_->commandList->SetGraphicsRootDescriptorTable(4, impl_->sampler_);
}

void LightingShader::SetObjectParam(D3D12_GPU_VIRTUAL_ADDRESS addr) {
  impl_->objParam_ = addr;
}

void LightingShader::SetSceneParam(D3D12_GPU_VIRTUAL_ADDRESS addr) {
  impl_->sceneParam_ = addr;
}

void LightingShader::SetMaterialParam(D3D12_GPU_VIRTUAL_ADDRESS addr) {
  impl_->matParam_ = addr;
}

void LightingShader::SetSrvDescriptorHeap(ID3D12DescriptorHeap* heap) {
  impl_->srvHeap_ = heap;
}

void LightingShader::SetSamplerDescriptorHeap(ID3D12DescriptorHeap* heap) {
  impl_->samplerHeap_ = heap;
}

void LightingShader::SetDammySrvDescriptorHeap(ID3D12DescriptorHeap* heap,
                                               const int offset) {
  impl_->dammySrvHeap_ = heap;
  impl_->dammySrv_ =
      CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->GetGPUDescriptorHandleForHeapStart(),
                                    offset, impl_->samplerDescriptorSize_);
}

void LightingShader::SetDefaultSamplerDescriptorHeap(ID3D12DescriptorHeap* heap,
                                                     const int offset) {
  impl_->defaultSamplerHeap_ = heap;
  impl_->defaultSampler_ =
      CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->GetGPUDescriptorHandleForHeapStart(),
                                    offset, impl_->samplerDescriptorSize_);
}

void LightingShader::SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE handle) {
  impl_->srv_ = handle;
}

void LightingShader::SetTextureSampler(D3D12_GPU_DESCRIPTOR_HANDLE handle) {
  impl_->sampler_ = handle;
}
}  // namespace dxapp
