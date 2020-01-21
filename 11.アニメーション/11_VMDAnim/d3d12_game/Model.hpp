#pragma once
#include "ModelMaterial.hpp"
#include   "Bone.hpp"

namespace dxapp {

class Device;
class PmdFile;

class Model {
 public:
  // PMDの頂点フォーマット
  // アウトライン描画はしないのでエッジフラグは省略
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

  // MMD用のメッシュを構成するデータ
  // 実態は単純に各メッシュのインデックスを覚えているだけ
  struct MeshParts {
    uint32_t offset;      //!< オフセット
    uint32_t indexCount;  //!< 使用するインデクス数
  };

  // オブジェクト定数とかシーン定数の型
  struct Transform {
    DirectX::XMFLOAT3 position{};
    DirectX::XMFLOAT3 rotation{};
    DirectX::XMFLOAT3 scale{1.0f, 1.0f, 1.0f};
  };

  //---
  struct ObjectParameter {
    DirectX::XMFLOAT4X4 world;      //!< ワールド
    DirectX::XMFLOAT4X4 bone[256];  //!< ボーン
  };


#pragma region add_0121
// モーフデータセット
  struct MorphData {
    std::vector<DirectX::XMFLOAT3> basePositions;  //!< 基本表情の頂点座標
    std::vector<uint32_t> baseIndices;  //!< 頂点インデクス

    // 1つのモーフィングデータ構造体
    struct Morph {
      std::string name; //!< 名前
      float weight; //!< 変化の重み 0.0(変化なし) ～ 1.0（表情がつく)
      std::vector<DirectX::XMFLOAT3> offsetPositions;  //!< 基本状態からのオフセット座標
      std::vector<uint32_t> offsetIndices;  //!< 書き換え先の頂点インデクス
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
