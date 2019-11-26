#include "BufferObject.hpp"
namespace dxapp {

bool BufferObject::Initialize(ID3D12Device* device, const BufferObjectType type,
                              std::size_t size) {
  // いまはD3D12_HEAP_TYPE_UPLOAD、D3D12_RESOURCE_STATE_GENERIC_READしか考えない
  auto heapType = D3D12_HEAP_TYPE_UPLOAD;
  auto state = D3D12_RESOURCE_STATE_GENERIC_READ;

  std::size_t bufferSize = size;
  // コンスタントバッファのメモリアラインメント
  if (type == BufferObjectType::ConstantBuffer) {
    // コンスタントバッファは256バイト単位で作る必要がある
    bufferSize = ((bufferSize + 255) & ~(255));
  }

  // リソース作成
  auto prop = CD3DX12_HEAP_PROPERTIES(heapType);
  auto desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
  auto hr = device->CreateCommittedResource(
      &prop, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr,
      IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf()));

  if (FAILED(hr)) {
    return false;
  }

  heapProp_ = prop;
  resourceDesc_ = desc;
  bufferSize_ = bufferSize;

  return true;
}

void BufferObject::Terminate() { resource_.Reset(); }

void* BufferObject::Map() {
  void* p{nullptr};
  resource_->Map(0, nullptr, &p);  // バッファからアドレスをもらう
  return p;
}

void BufferObject::Unmap() { resource_->Unmap(0, nullptr); }

void BufferObject::Update(const void* data, std::size_t size,
                          std::size_t offset) {
  if (heapProp_.Type == D3D12_HEAP_TYPE_UPLOAD) {
    std::uint8_t* addr = static_cast<std::uint8_t*>(Map());
    memcpy(addr + offset, data, size);
    Unmap();
  } else {
    // D3D12_RESOURCE_STATE_COPY_DESTはそのうちやる(かも)
  }
}
}  // namespace dxapp
