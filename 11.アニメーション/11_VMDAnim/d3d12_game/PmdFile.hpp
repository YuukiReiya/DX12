#pragma once
#include <fstream>

class Device;

namespace dxapp {

#pragma region pmd

#pragma pack(push, 1)
struct PmdVertex {
  DirectX::XMFLOAT3 position;
  DirectX::XMFLOAT3 normal;
  DirectX::XMFLOAT2 uv;
  uint16_t boneID[2];
  uint8_t boneWeight;
  uint8_t edgeFlag;
};
#pragma pack(pop)

struct PmdMaterial {
  DirectX::XMFLOAT3 diffuse;
  float alpha;
  float specularity;
  DirectX::XMFLOAT3 specular;
  DirectX::XMFLOAT3 ambient;
  uint8_t toonIndex;
  uint8_t edgeFlag;
  uint32_t vertexCount;
  std::string textureFile;
};

struct PmdBone {
  std::string name;
  uint16_t parentIndex;
  uint16_t childIndex;
  uint8_t boneType;
  uint16_t IKBoneIndex;
  DirectX::XMFLOAT3 position;
};

struct PmdIk {
  uint16_t boneIndex;
  uint16_t targetBoneIndex;
  uint16_t recursiveCount;
  float countrolWeight;
  std::vector<uint16_t> childBoneIndices;
};

// モーフのタイプとモーフの頂点座標、頂点インデックスを追加しました
struct PmdMorph {
  // MMDのモーフは顔面向けで次の5種類
  enum Type {
    Base = 0,  //!< 基本の表情
    EyeBrow,   //!< まゆげ
    Eye,       //!< 目
    Lip,       //!< リップ
    Other,     //!< その他
  };

  std::string name;
  uint32_t vertexCount;
  Type type;  //!< new

  std::vector<uint32_t> indices;             //!< new 頂点インデックス
  std::vector<DirectX::XMFLOAT3> positions;  //!< new モーフ頂点
};


class PmdFile {
 public:
  PmdFile() = default;
  ~PmdFile() = default;

  bool Load(const std::wstring& fileName, Device* device);

  float vertion() const { return version_; }
  const std::string& name() const { return name_; }
  const std::string& comment() const { return comment_; }

  std::size_t vertexCount() const { return vertices_.size(); }
  std::size_t indexCount() const { return indices_.size(); }
  std::size_t materialCount() const { return materials_.size(); }
  std::size_t boneCount() const { return bones_.size(); }
  std::size_t ikCount() const { return iks_.size(); }
  std::size_t mourphCount() const { return morphs_.size(); }

  const PmdVertex& vertex(int i) const { return vertices_[i]; }
  uint16_t index(int i) const { return indices_[i]; }
  const PmdMaterial& material(int i) const { return materials_[i]; }
  const PmdBone& bone(int i) const { return bones_[i]; }
  const PmdIk& ik(int i) const { return iks_[i]; }
  const PmdMorph& morph(int i) const { return morphs_[i]; }

 private:
  float version_;
  std::string name_;
  std::string comment_;
  std::vector<PmdVertex> vertices_;
  std::vector<uint16_t> indices_;
  std::vector<PmdMaterial> materials_;
  std::vector<PmdBone> bones_;
  std::vector<PmdIk> iks_;
  std::vector<PmdMorph> morphs_;
};
#pragma endregion


//---

#pragma region vmd

struct VmdMotionFrame {
  std::string name;
  uint32_t keyframe;
  DirectX::XMFLOAT3 location;
  DirectX::XMFLOAT4 rotation;
  uint8_t interpolation[64];
  //---------ここまででキーフレームアニメーションに必要なデータ

  DirectX::XMFLOAT4 bezierParam(int i) {
    DirectX::XMFLOAT4 p{};
    p.x = interpolation[4 * 0 + i] / 127.0f;
    p.y = interpolation[4 * 1 + i] / 127.0f;
    p.z = interpolation[4 * 2 + i] / 127.0f;
    p.w = interpolation[4 * 3 + i] / 127.0f;
    return p;
  }
};

struct VmdMorphFrame {
  std::string name;
  uint32_t keyframe;
  float weight;
};

class VmdFile {
public:
  VmdFile() = default;
  ~VmdFile() = default;

  bool Load(const std::wstring& fileName);

  const std::string& header() const { return header_; }
  uint32_t keyframeCount() const { return keyframeCount_; }
  std::size_t motionCount() const { return motionNames_.size(); }
  std::size_t morphCount() const { return morphNames_.size(); }

  const std::string& motionName(int i) const { return motionNames_[i]; }
  const std::string& morphName(int i) const { return morphNames_[i]; }

  std::vector<VmdMotionFrame>& motion(const std::string& name) { return motions_[name]; }
  std::vector<VmdMorphFrame>& morph(const std::string& name) { return morphs_[name]; }

private:
  std::string header_{};
  uint32_t keyframeCount_{};

  std::vector<std::string> motionNames_;
  std::unordered_map<std::string, std::vector<VmdMotionFrame>> motions_;

  std::vector<std::string> morphNames_;
  std::unordered_map<std::string, std::vector<VmdMorphFrame>> morphs_;
};

#pragma endregion
}  // namespace dxapp
