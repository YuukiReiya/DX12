#pragma once
namespace dxapp {
class Device;

/*!
 * @brief �V���h�E�}�b�v��\�����邾���̃V�F�[�_�[
 */

class ShadowMapDebugShader {
 public:
  /*!
   * @brief �R���X�g���N�^
   */
  ShadowMapDebugShader();

  /*!
   * @brief �f�X�g���N�^
   */
  ~ShadowMapDebugShader();

  /*!
   * @brief ������
   */
  void Initialize(Device* device);

  /*!
   * @brief �I������
   */
  void Terminate();

  /*!
   * @brief �t���[���ŃV�F�[�_���g�p����Ƃ��ɍŏ��ɌĂяo���֐�
   * @details End���ĂԂ܂ł͈����̃R�}���h���X�g���g��
   * @param[in,ont] commandList �R�}���h��ςނ��߂̃R�}���h���X�g
   */
  void Begin(ID3D12GraphicsCommandList* commandList);

  /*!
   * @brief �t���[���ŃV�F�[�_���g���ǂ������K���ĂԊ֐�
   */
  void End();

  /*!
   * @brief �V�F�[�_�������Ă���p�����[�^�ŃR�}���h�𔭍s
   */
  void Apply();

  /*!
   * @brief �f�X�N���v�^�e�[�u�����Q�Ƃ���q�[�v���Z�b�g
   */
  void SetDescriptorHeap(ID3D12DescriptorHeap* srv, ID3D12DescriptorHeap* sampler);

  /*!
   * @brief �V���h�E�}�b�v�ݒ�
   */
  void SetShadowMap(D3D12_GPU_DESCRIPTOR_HANDLE handle);

  /*!
   * @brief �T���v���ݒ�
   */
  void SetSampler(D3D12_GPU_DESCRIPTOR_HANDLE handle);

 private:
  //! ���������N���X
  class Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace dxapp