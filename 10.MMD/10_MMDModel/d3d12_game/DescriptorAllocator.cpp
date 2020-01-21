#include "DescriptorAllocator.hpp"

namespace dxapp {

void DescriptorHandle::Free() {
  assert(allocator != nullptr);
  allocator->Free(this);
}

bool DescriptorAllocator::Initialize(ID3D12Device* device,
                                     const D3D12_DESCRIPTOR_HEAP_DESC& desc) {
  auto hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap_));
  if (FAILED(hr)) {
    return false;
  }

  desc_ = desc;
  incrementSize_ = device->GetDescriptorHandleIncrementSize(desc.Type);
  cpuStart_ = heap_->GetCPUDescriptorHandleForHeapStart();
  gpuStart_ = heap_->GetGPUDescriptorHandleForHeapStart();

  return true;
}

void DescriptorAllocator::Terminate() {
  Reset();
  heap_.Reset();
}

DescriptorHandle DescriptorAllocator::Allocate() {
  allocCount_++;
  assert(allocCount_ != desc_.NumDescriptors);

  DescriptorHandle handle{};
  handle.allocator = this;
  handle.cpuHandle = cpuStart_;
  handle.gpuHandle = gpuStart_;

  if (!freeList_.empty()) {
    // �t���[�̃n���h��������΂�����g��
    // �����ƍ��Ȃ�DescriptorHandle���̂�o�������X�g�ɂ��ĂȂ����ق����������ɗD����������
    handle.index = freeList_.front();
    freeList_.pop_front();
  } else {
    handle.index = index_;
    index_++;
  }

  return handle;
}

void DescriptorAllocator::Free(DescriptorHandle* handle) {
  assert(handle->allocator == this);
  assert(handle->allocator != nullptr);

  // �C���f�b�N�X�̓��X�g�ɓ˂�����ōė��p�ł���悤�ɂ���
  freeList_.emplace_back(handle->index);

  // �������n���h���ɂ͖����Ȓl���l�߂Ă���
  handle->allocator = nullptr;
  handle->cpuHandle.ptr = 0;
  handle->gpuHandle.ptr = 0;
  handle->index = -1;

  allocCount_--;
}

void DescriptorAllocator::Reset() {
  freeList_.clear();
  index_ = 0;
  allocCount_ = 0;
}

}  // namespace dxapp