#pragma once
#include "TextureManager.hpp"
#include "BufferObject.hpp"

namespace dxapp {

class Device;

class ModelMaterial {
public:
  struct Material {
    DirectX::XMFLOAT4 diffuse{};
    DirectX::XMFLOAT4 specular{};
    DirectX::XMFLOAT3 ambient{};
    uint32_t useTexture{};
  };

  ModelMaterial();
  ~ModelMaterial();

  void Initialize(dxapp::Device* device);
  void Update(std::uint32_t backbufferIndex);

  void SetMaterial(const Material& m, dxapp::TextureView view);
  void SetDiffuse(DirectX::XMFLOAT4 diffuse);
  void SetSpecular(DirectX::XMFLOAT4 specular);
  void SetAbient(DirectX::XMFLOAT3 ambient);
  void SetTexture(bool useTexture, dxapp::TextureView view);

  bool HasTexture() const;
  Material material() const;

  D3D12_GPU_VIRTUAL_ADDRESS bufferAddress(std::uint32_t backbufferIndex) const;
  CD3DX12_GPU_DESCRIPTOR_HANDLE textureHandle() const;
private:
  class Impl;
  std::unique_ptr<Impl>  impl_;
};
}  // namespace dxapp