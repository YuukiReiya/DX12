#include "ShadowMapDebugShader.hpp"

#include "BufferObject.hpp"
#include "Device.hpp"
#include "Utility.hpp"
#include "VertexType.hpp"

namespace dxapp {
#pragma region ShadowMapDebugShader::Impl
/*!
 * @brief ShadowMapDebugShaderの実装
 */
class ShadowMapDebugShader::Impl {
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

  // Begin/Endで使うコマンドリスト
  ID3D12GraphicsCommandList* commandList{};

  ID3D12DescriptorHeap *srvHeap_{}, *samplerHeap_{};
  D3D12_GPU_DESCRIPTOR_HANDLE srv_, sampler_;
};

void ShadowMapDebugShader::Impl::CreateShader() {
  {
    Microsoft::WRL::ComPtr<ID3DBlob> error;
    utility::CompileShaderFromFile(L"Shaders/ShadowMapDebugVS.hlsl", L"vs_6_0",
                                   vs_, error);
    utility::CompileShaderFromFile(L"Shaders/ShadowMapDebugPS.hlsl", L"ps_6_0",
                                   ps_, error);
  }
}

// ルートシグネチャ生成
void ShadowMapDebugShader::Impl::CreatRootSignature(ID3D12Device* device) {
  // t0, s0の2個のパラメータを使う
  CD3DX12_ROOT_PARAMETER rootParams[2];

  CD3DX12_DESCRIPTOR_RANGE srv, sampler;
  srv.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
  sampler.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

  rootParams[0].InitAsDescriptorTable(1, &srv, D3D12_SHADER_VISIBILITY_PIXEL);
  rootParams[1].InitAsDescriptorTable(1, &sampler,
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
void ShadowMapDebugShader::Impl::CreatePipelineState(ID3D12Device* device) {
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
  psoDesc.VS = CD3DX12_SHADER_BYTECODE(vs_.Get());
  psoDesc.PS = CD3DX12_SHADER_BYTECODE(ps_.Get());
  psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  psoDesc.InputLayout = {VertexPositionColorNormalTexturElement,
                         _countof(VertexPositionColorNormalTexturElement)};
  psoDesc.pRootSignature = rootSignature_.Get();
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.SampleDesc = {1, 0};
  psoDesc.SampleMask = UINT_MAX;
  psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
  psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

  device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline_));
}

ShadowMapDebugShader::ShadowMapDebugShader() : impl_(new Impl) {}

ShadowMapDebugShader::~ShadowMapDebugShader() {
  { Terminate(); };
}

void ShadowMapDebugShader::Initialize(Device* device) {
  auto dev = device->device();

  impl_->CreateShader();
  impl_->CreatRootSignature(dev);
  impl_->CreatePipelineState(dev);
}

void ShadowMapDebugShader::Terminate() {}

void ShadowMapDebugShader::Begin(ID3D12GraphicsCommandList* commandList) {
  assert(impl_->commandList == nullptr);
  impl_->commandList = commandList;
  commandList->SetGraphicsRootSignature(impl_->rootSignature_.Get());
  commandList->SetPipelineState(impl_->pipeline_.Get());
}

void ShadowMapDebugShader::End() {
  assert(impl_->commandList);
  impl_->commandList = nullptr;
}

void ShadowMapDebugShader::Apply() {
  ID3D12DescriptorHeap* heaps[] = {impl_->srvHeap_, impl_->samplerHeap_};
  impl_->commandList->SetDescriptorHeaps(_countof(heaps), heaps);

  impl_->commandList->SetGraphicsRootDescriptorTable(0, impl_->srv_);
  impl_->commandList->SetGraphicsRootDescriptorTable(1, impl_->sampler_);
}

void ShadowMapDebugShader::SetDescriptorHeap(ID3D12DescriptorHeap* srv,
                                             ID3D12DescriptorHeap* sampler) {
  impl_->srvHeap_ = srv;
  impl_->samplerHeap_ = sampler;
}

void ShadowMapDebugShader::SetShadowMap(D3D12_GPU_DESCRIPTOR_HANDLE handle) {
  impl_->srv_ = handle;
}

void ShadowMapDebugShader::SetSampler(D3D12_GPU_DESCRIPTOR_HANDLE handle) {
  impl_->sampler_ = handle;
}

}  // namespace dxapp
