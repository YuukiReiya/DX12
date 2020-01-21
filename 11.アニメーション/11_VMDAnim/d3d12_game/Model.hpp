#pragma once
#include "ModelMaterial.hpp"
#include   "Bone.hpp"

namespace dxapp {

class Device;
class PmdFile;

class Model {
 public:
  // PMD�̒��_�t�H�[�}�b�g
  // �A�E�g���C���`��͂��Ȃ��̂ŃG�b�W�t���O�͏ȗ�
  struct ModelVertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 uv;
    DirectX::XMUINT2 boneID;
    DirectX::XMFLOAT2 boneWeights;
  };

  static constexpr D3D12_INPUT_ELEMENT_DESC ModelVertexElement[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
      D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
      0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
      D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
      0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"BONEINDICES", 0, DXGI_FORMAT_R32G32_UINT, 0,
      D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
      0},
    {"BONEWEIGHTS", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
      D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
      0},
  };

  // MMD�p�̃��b�V�����\������f�[�^
  // ���Ԃ͒P���Ɋe���b�V���̃C���f�b�N�X���o���Ă��邾��
  struct MeshParts {
    uint32_t offset;      //!< �I�t�Z�b�g
    uint32_t indexCount;  //!< �g�p����C���f�N�X��
  };

  // �I�u�W�F�N�g�萔�Ƃ��V�[���萔�̌^
  struct Transform {
    DirectX::XMFLOAT3 position{};
    DirectX::XMFLOAT3 rotation{};
    DirectX::XMFLOAT3 scale{1.0f, 1.0f, 1.0f};
  };

  //---
  struct ObjectParameter {
    DirectX::XMFLOAT4X4 world;      //!< ���[���h
    DirectX::XMFLOAT4X4 bone[256];  //!< �{�[��
  };


#pragma region add_0121
// ���[�t�f�[�^�Z�b�g
  struct MorphData {
    std::vector<DirectX::XMFLOAT3> basePositions;  //!< ��{�\��̒��_���W
    std::vector<uint32_t> baseIndices;  //!< ���_�C���f�N�X

    // 1�̃��[�t�B���O�f�[�^�\����
    struct Morph {
      std::string name; //!< ���O
      float weight; //!< �ω��̏d�� 0.0(�ω��Ȃ�) �` 1.0�i�\���)
      std::vector<DirectX::XMFLOAT3> offsetPositions;  //!< ��{��Ԃ���̃I�t�Z�b�g���W
      std::vector<uint32_t> offsetIndices;  //!< ����������̒��_�C���f�N�X
    };

    std::vector<Morph> morphs;
  };

#pragma endregion

public:
  Model();
  ~Model();

  bool Initialize(Device* device, const PmdFile* pmd);
  void Update(float deltaTime, Device* device);
  void CalculateBoneMatrices();

  void rotation(const DirectX::XMFLOAT3& rot);
  const Model::MeshParts& meshParts(uint32_t i) const;
  const ModelMaterial& material(uint32_t i) const;
  void morphWeight(const std::string& name, float weight);

  Bone* bone(uint32_t i) const;
  IKBone& ikBone(uint32_t i) const;

  std::size_t meshPartsCount() const;
  std::size_t materialCount() const;
  std::size_t boneCount() const;
  std::size_t ikBoneCount() const;


  D3D12_GPU_VIRTUAL_ADDRESS objectParamBufferAddress() const;
  D3D12_VERTEX_BUFFER_VIEW* vertexBufferView() const;
  D3D12_INDEX_BUFFER_VIEW* indexBufferView() const;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace dxapp
