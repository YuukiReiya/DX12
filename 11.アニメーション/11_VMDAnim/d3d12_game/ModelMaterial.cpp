#include "ModelMaterial.hpp"

#include "BufferObject.hpp"
#include "Device.hpp"

namespace dxapp {
class  ModelMaterial::Impl {
public:
  Material material_{};  //!< マテリアルデータ
  std::vector<std::unique_ptr<dxapp::BufferObject>> cbuffers_{};  //!< コンスタントバッファ
  dxapp::TextureView view_{};  //!< テクスチャビュー
  bool dirty_{};               //!< バッファアップデートフラグ

};

ModelMaterial::ModelMaterial() :  impl_(std::make_unique<Impl>()) {}
ModelMaterial::~ModelMaterial() {}

void ModelMaterial::Initialize(dxapp::Device* device) {
  impl_->cbuffers_.resize(device->backBufferSize());
  for (auto& cb : impl_->cbuffers_) {
    cb = std::make_unique<dxapp::BufferObject>();
    cb->Initialize(device->device(), dxapp::BufferObjectType::ConstantBuffer,
                   sizeof(impl_->material_));
  }
  impl_->dirty_ = true;
}

void ModelMaterial::Update(std::uint32_t backbufferIndex) {
  if (!impl_->dirty_) return;

  for (auto& cb : impl_->cbuffers_) {
    cb->Update(&impl_->material_, sizeof(impl_->material_));
  }
  impl_->dirty_ = false;
}

void ModelMaterial::SetMaterial(const Material& m, dxapp::TextureView view) {
  impl_->material_ = m;
  impl_->view_ = view;
  impl_->dirty_ = true;
}

void ModelMaterial::SetDiffuse(DirectX::XMFLOAT4 diffuse) {
  impl_->material_.diffuse = diffuse;
  impl_->dirty_ = true;
}

void ModelMaterial::SetSpecular(DirectX::XMFLOAT4 specular) {
  impl_->material_.specular = specular;
  impl_->dirty_ = true;
}

void ModelMaterial::SetAbient(DirectX::XMFLOAT3 ambient) {
  impl_->material_.ambient = ambient;
  impl_->dirty_ = true;
}

void ModelMaterial::SetTexture(bool useTexture, dxapp::TextureView view) {
  impl_->material_.useTexture = useTexture ? 1 : 0;
  impl_->view_ = view;
  impl_->dirty_ = true;
}

D3D12_GPU_VIRTUAL_ADDRESS ModelMaterial::bufferAddress(
    std::uint32_t backbufferIndex) const {
  return impl_->cbuffers_[backbufferIndex]->resource()->GetGPUVirtualAddress();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE ModelMaterial::textureHandle() const {
  return CD3DX12_GPU_DESCRIPTOR_HANDLE(
      impl_->view_.handle.gpuHandle, impl_->view_.handle.index,
      impl_->view_.handle.allocator->incrementSize());
}

bool ModelMaterial::HasTexture() const { return impl_->material_.useTexture != 0; }
ModelMaterial::Material ModelMaterial::material() const { return impl_->material_; }


}  // namespace dxapp