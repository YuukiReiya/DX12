#pragma once
namespace dxapp {

class BufferObject;
class Device;

/*!
 * @brief ���f���̎�����\���f�[�^
 */
class Material {
 public:
  struct MaterialParameter {
    DirectX::XMFLOAT4X4 matTrans{};
    DirectX::XMFLOAT4 diffuseAlbedo{1, 1, 1, 1};
    DirectX::XMFLOAT3 fresnel{0.5f, 0.5f, 0.5f};
    float roughness{0.1f};
    int useTexture = 0;
  };

  /*!
   * @brief �R���X�g���N�^
   */
  Material() = default;
  Material(const MaterialParameter& param);

  /*!
   * @brief ������
   */
  void Initialize(dxapp::Device* device);

  /*!
   * @brief �萔�o�b�t�@�X�V
   */
  void Update(std::uint32_t index);

  /*!
   * @brief �g�U���ˌ��̐ݒ�
   */
  void SetDiffuseAlbedo(DirectX::XMFLOAT4 diffuseAlbedo);

  /*!
   * @brief ���ʔ��ˌ��̐ݒ�
   */
  void SetFresnel(DirectX::XMFLOAT3 fresnel);

  /*!
   * @brief �ʂ̑e���ݒ�
   */
  void SetRoughness(float roughness);

  /*!
   * @brief �e�N�X�`���̐ݒ�
   */
  void SetTexture(ID3D12DescriptorHeap* heap, std::uint32_t offset);

  /*!
   * @brief �e�N�X�`���̐ݒ�
   */
  void SetMatrix(DirectX::XMMATRIX m);

  /*!
   * @brief �e�N�X�`���̉���
   */
  void ClearTexture();

  /*!
   * @brief �e�N�X�`���̃q�[�v�ƃI�t�Z�b�g�ʒu���擾
   */
  void textureDescHeap(ID3D12DescriptorHeap** heap, std::uint32_t* offset);

  /*!
   * @brief �e�N�X�`���̊��蓖�Ċm�F
   */
  bool HasTexture() const;

  /*!
   * @brief ���̃}�e���A���̒萔�o�b�t�@���擾
   */
  D3D12_GPU_VIRTUAL_ADDRESS materialCb(std::uint32_t index) const;

 private:
  MaterialParameter material_;  //!< �萔�o�b�t�@�ɏ������ޒl
  std::vector<std::unique_ptr<dxapp::BufferObject>>
      matCb_{};  //!< MaterialParameter�̒萔�o�b�t�@�̈�
  ID3D12DescriptorHeap* srvHeap_{nullptr};  //!< SRV�f�X�N���v�^�q�[�v
  std::uint32_t srvOffset_{0};              //! �A�h���X�I�t�Z�b�g
};
}  // namespace dxapp