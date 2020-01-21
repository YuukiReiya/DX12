#pragma once
#include <list>

namespace dxapp {
class DescriptorAllocator;

/*!
 * CPU/GPU�̃f�X�N���v�^�n���h���ƃI�t�Z�b�g���i�[����
 * �n���h�����g���I�������Free���Ăяo����
 */
struct DescriptorHandle {
  DescriptorAllocator* allocator{nullptr};
  D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{0};
  D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{0};
  uint32_t index{};

  void Free();
};

/*!
 * �f�X�N���v�^�n���h���̊m�ہE������s���N���X
 * ��������n���h���̓t���[���X�g�������čė��p�ł���悤�ɂ���
 */
class DescriptorAllocator {
public:
  DescriptorAllocator() = default;
  ~DescriptorAllocator() { Terminate(); }

  bool Initialize(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& desc);
  void Terminate();
  void Reset();

  /*!
   * �f�X�N���v�^�n���h�����m�ۂ��܂�
   */
  DescriptorHandle Allocate();

  /*!
   * �s�p�ȃf�X�N���v�^�n���h����������܂�
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