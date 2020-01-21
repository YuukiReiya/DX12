#pragma once
#include <fstream>

class Device;

namespace dxapp {
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

struct PmdMorph {
  std::string name;
  uint32_t vertexCount;
  uint8_t type;
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

  const PmdVertex& vertex(int i) const { return vertices_[i]; }
  uint16_t index(int i) const { return indices_[i]; }
  const PmdMaterial& material(int i) const { return materials_[i]; }
  const PmdBone& bone(int i) const { return bones_[i]; }
  const PmdIk& ik(int i) const { return iks_[i]; }

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
}  // namespace dxapp
