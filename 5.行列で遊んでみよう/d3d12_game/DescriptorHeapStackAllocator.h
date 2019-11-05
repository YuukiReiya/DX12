#pragma once
namespace dxapp {
/*!
 * @brief デスクリプタをスタック風に管理する
 */
class DescriptorHeapStackAllocator {
 public:
  DescriptorHeapStackAllocator() = default;
  ~DescriptorHeapStackAllocator() = default;

  DescriptorHeapStackAllocator(const DescriptorHeapStackAllocator&) = delete;
  DescriptorHeapStackAllocator& operator=(const DescriptorHeapStackAllocator&) =
      delete;
  DescriptorHeapStackAllocator(DescriptorHeapStackAllocator&&) = delete;
  DescriptorHeapStackAllocator& operator=(DescriptorHeapStackAllocator&&) =
      delete;

  void Initialize(ID3D12Device* device, std::uint32_t numPerUnit = 1,
                  std::uint32_t stackSize = 128);

  std::uint32_t pop();
  void push(std::uint32_t index);

  D3D12_CPU_DESCRIPTOR_HANDLE heapStart() const {
    return heap_->GetCPUDescriptorHandleForHeapStart();
  }

  ID3D12DescriptorHeap* heap() { return heap_.Get(); }

 private:
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_{};
  std::vector<std::uint32_t> indices_;
  std::uint32_t numPerUnit_;
  std::uint32_t stackSize_;

  std::uint32_t pointer_{};
};
}  // namespace dxapp