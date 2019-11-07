#include "DescriptorHeapStackAllocator.h"

namespace dxapp {
void DescriptorHeapStackAllocator::Initialize(ID3D12Device* device,
                                              std::uint32_t numPerUnit,
                                              std::uint32_t stackSize) {
  assert(stackSize >= 1);
  assert(numPerUnit >= 1);

  indices_.resize(stackSize);
  stackSize_ = stackSize;
  numPerUnit_ = numPerUnit;

  for (std::uint32_t i = 0; i < stackSize; i++) {
    indices_[i] = i * numPerUnit;
  }
  pointer_ = stackSize_ - 1;

  D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, stackSize * numPerUnit,
      D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
  device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap_));
}

std::uint32_t DescriptorHeapStackAllocator::pop() {
  assert(pointer_ > 0);

  //ココ先に引いちゃダメじゃね？
  //--pointer_;
  return indices_[pointer_--];
}

void DescriptorHeapStackAllocator::push(std::uint32_t index) {
  assert(pointer_ < stackSize_);

  //ココ先に足しちゃダメじゃね？
  //++pointer_;
  indices_[pointer_++] = index;
}
}  // namespace dxapp