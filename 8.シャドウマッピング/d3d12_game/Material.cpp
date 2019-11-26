#include "Material.hpp"

#include "BufferObject.hpp"
#include "Device.hpp"

namespace dxapp {
Material::Material(const MaterialParameter& param) : material_(param) {}

void Material::Initialize(dxapp::Device* device) {
  // マテリアルは変化しなさそうだけど、、、
  // 点滅したりテクスチャがスクロールするので一応ダブルバッファ化
  matCb_.resize(device->backBufferSize());
  for (auto& cb : matCb_) {
    cb = std::make_unique<dxapp::BufferObject>();
    cb->Initialize(device->device(), dxapp::BufferObjectType::ConstantBuffer,
                   sizeof(material_));
  }
}

void Material::Update(std::uint32_t index) {
  matCb_[index]->Update(&material_, sizeof(MaterialParameter));
}

// 値の範囲チェックとかを本当はするんだよ
void Material::SetDiffuseAlbedo(DirectX::XMFLOAT4 diffuseAlbedo) {
  material_.diffuseAlbedo = diffuseAlbedo;
}

void Material::SetFresnel(DirectX::XMFLOAT3 fresnel) {
  material_.fresnel = fresnel;
}

void Material::SetRoughness(float roughness) {
  material_.roughness = roughness;
}

void Material::SetTexture(ID3D12DescriptorHeap* heap, std::uint32_t offset) {
  srvHeap_ = heap;
  srvOffset_ = offset;
  material_.useTexture = 1;
}

void Material::SetMatrix(DirectX::XMMATRIX m) {
  DirectX::XMStoreFloat4x4(&material_.matTrans, DirectX::XMMatrixTranspose(m));
}

void Material::ClearTexture() {
  srvHeap_ = nullptr;
  srvOffset_ = 0;
  material_.useTexture = 0;
}

void Material::textureDescHeap(ID3D12DescriptorHeap** heap,
                               std::uint32_t* offset) {
  *heap = srvHeap_;
  *offset = srvOffset_;
}

bool Material::HasTexture() const { return (material_.useTexture != 0); }

D3D12_GPU_VIRTUAL_ADDRESS Material::materialCb(std::uint32_t index) const {
  return matCb_[index]->resource()->GetGPUVirtualAddress();
}

}  // namespace dxapp
