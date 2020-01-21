#pragma once
#include <list>

namespace dxapp {
class DescriptorAllocator;

/*!
 * CPU/GPUのデスクリプタハンドルとオフセットを格納する
 * ハンドルを使い終わったらFreeを呼び出そう
 */
struct DescriptorHandle {
  DescriptorAllocator* allocator{nullptr};
  D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{0};
  D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{0};
  uint32_t index{};

  void Free();
};

/*!
 * デスクリプタハンドルの確保・解放を行うクラス
 * 解放したハンドルはフリーリストをつかって再利用できるようにする
 */
class DescriptorAllocator {
public:
  DescriptorAllocator() = default;
  ~DescriptorAllocator() { Terminate(); }

  bool Initialize(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& desc);
  void Terminate();
  void Reset();

  /*!
   * デスクリプタハンドルを確保します
   */
  DescriptorHandle Allocate();

  /*!
   * 不用なデスクリプタハンドルを解放します
   */
  void Free(DescriptorHandle* handle);

  UINT incrementSize() const { return incrementSize_; }
  uint32_t allocCount() const { return allocCount_; }
  ID3D12DescriptorHeap* heap() const { return heap_.Get(); }

private:
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_{};
  D3D12_DESCRIPTOR_HEAP_DESC desc_{};
  D3D12_CPU_DESCRIPTOR_HANDLE cpuStart_{};
  D3D12_GPU_DESCRIPTOR_HANDLE gpuStart_{};
  UINT incrementSize_{};
  uint32_t index_{};
  uint32_t allocCount_{};
  std::list<uint32_t> freeList_;
};
}  // namespace dxapp