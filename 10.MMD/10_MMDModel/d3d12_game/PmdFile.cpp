//�Q�l:https://blog.goo.ne.jp/torisu_tetosuki/e/209ad341d3ece2b1b4df24abf619d6e4
#include "Device.hpp"
#include "Singleton.hpp"
#include "TextureManager.hpp"

#include "PmdFile.hpp"

using namespace DirectX;

namespace {

// PMD�f�[�^�ǂݍ��ݗp�̒��Ԍ`��

#pragma pack(push, 1)
// pragma pack �̓N���X��\���̃����o�[�̃A���C�������g���w�肷��
// #pragma pack(push, 1) �����1�o�C�g�Ńp�b�L���O�����

//  ��j
//      struct Hoge{
//           uint16_t a;//  1byte�̃f�[�^
//           int 4;// 4byte�̃f�[�^
//       }
//        �S�̂�1byte�̃f�[�^����CPU����4byte(int�^�T�C�Y)�ɕϊ�����Ă��܂�
//        �A���C�����g�͂���̖h�~
//  <��>
//        �V�F�[�_�[���̒萔�o�b�t�@��256byte�Ȃ̂�256�Ńp�b�L���O���Ă�����ƗǁB

struct PmdHeaderData {
  unsigned char magic[3];
  float version;
  char name[20];
  char comment[256];
};

struct PmdMaterialData {
  XMFLOAT3 diffuse;
  float alpha;
  float specularity;
  XMFLOAT3 specular;
  XMFLOAT3 ambient;
  uint8_t toonIndex;
  uint8_t edgeFlag;
  uint32_t vertexCount;
  char textureName[20];
};
struct PmdBoneData {
  char name[20];
  uint16_t parentIndex;
  uint16_t childIndex;
  uint8_t  boneType;
  uint16_t IKBoneIndex;
  XMFLOAT3 position;
};
struct PmdIkData {
  uint16_t    boneIndex;
  uint16_t    targetBoneIndex;
  uint8_t     chainCount;
  uint16_t    recursiveCount;
  float       countrolWeight;
};

struct PmdMorphData {
  char name[20];
  uint32_t vertexCount;
  uint8_t type;
};
// #pragma pack(pop) �ő΂ɂȂ� pack����������
#pragma pack(pop)

//----
//���p��(union)�̎d�g��
//
//4byte |    u32    |
//      |b0|b1|b2|b3|

union uint32_char4 {
  uint32_t u32;
  char b[4];
};

union uint16_char2 {
  uint16_t u16;
  char b[2];
};

// uint32_t��ǂ��v�Ɋi�[
void ReadUint32(std::ifstream& ifs, uint32_t& v) {
  uint32_char4 data{};
  ifs.read(data.b, 4);
  v = data.u32;
}

// uint16_t��ǂ��v�Ɋi�[
void ReadUint16(std::ifstream& ifs, uint16_t& v) {
  uint16_char2 data{};
  ifs.read(data.b, 2);
  v = data.u16;
}
} // unnamed namespace

namespace dxapp {

bool PmdFile::Load(const std::wstring& fileName, Device* device) {

  std::ifstream file(fileName, std::ios::binary);

  PmdHeaderData header{};
  // read�֐��̓o�C�g�P�ʂŃf�[�^��ǂ�
  file.read(reinterpret_cast<char*>(&header), sizeof(header));

  if (header.magic[0] != 'P' || header.magic[1] != 'm' || header.magic[2] != 'd') {
    // ����̓t�@�C���}�W�b�N�i�t�@�C���̎��ʏ��j������Ă���I��
    wchar_t buf[256] = {};
    swprintf(buf, 256, L"%s �� Pmd����Ȃ��悤�ł���\n", fileName.c_str());
    OutputDebugString(buf);
    return false;
  }

  name_ = header.name;
  version_ = header.version;
  comment_ = header.comment;

  // �o�C�i���t�@�C���Ȃ̂�read�Ń`�N�`�N�ǂ�ł����܂���

  // �w�b�_�̂��Ƃ�4�o�C�g�Œ��_�����͂����Ă��܂� 
  uint32_t vertexCount{};
  ReadUint32(file, vertexCount);

  // ���_�z����m��
  vertices_.resize(vertexCount);

  // ���_���̌�ɂ́A���_�f�[�^�����_��������ł��܂���
  // ���_������C�ɓǂ񂶂Ⴆ
  file.read(reinterpret_cast<char*>(vertices_.data()), sizeof(PmdVertex) * vertexCount);


  // ���_�̂��Ƃ�4�o�C�g�ŃC���f�b�N�X��
  uint32_t indexCount{};
  ReadUint32(file, indexCount);

  // �C���f�b�N�X�z����m�ۂ��ēǂݍ���
  indices_.resize(indexCount);
  file.read(reinterpret_cast<char*>(indices_.data()), sizeof(uint16_t) * indexCount);


  // ����4�o�C�g�Ń}�e���A����
  uint32_t materialCount{};
  ReadUint32(file, materialCount);

  // �}�e���A���f�[�^��ǂݍ���
  materials_.resize(materialCount);
  for (auto& m : materials_) {
    // �}�e���A���̓e�N�X�`���t�@�C����������̂�PmdMaterialData�œǂ�ł���
    // PmdMaterial�ɓ���Ȃ����܂���
    PmdMaterialData data{};
    file.read(reinterpret_cast<char*>(&data), sizeof(data));

    m.diffuse = data.diffuse;
    m.alpha = data.alpha;
    m.specularity = data.specularity;
    m.specular = data.specular;
    m.ambient = data.ambient;
    m.toonIndex = data.toonIndex;
    m.edgeFlag = data.edgeFlag;
    m.vertexCount = data.vertexCount;

    // �e�N�X�`���t�@�C����������
    std::string assetName = data.textureName;
    if (assetName.empty()) {
      continue;   // �t�@�C�������Ȃ��Ȃ�X�L�b�v
    }

    // �t�@�C�����ɂ̓f�B���N�g����񂪂Ȃ��̂ŋ����ɑ����Ă��܂��i�X�}�[�g����Ȃ��ˁj
    assetName = "Assets/" + assetName;

    // mmd�̃e�N�X�`���t�@�C�����ɂ̓J���[�}�b�v�����X�t�B�A�}�b�v���̂ǂ��炩���A
    // �܂��͓�� **�ꏏ **�Ɋi�[����Ă���
    // �ꏏ�Ɋi�[����Ă���ꍇ�� "�J���[�}�b�v*�X�t�B�A�}�b�v" �� "*" �ŋ�؂���
    // �����ł܂� textureName��"*"������΂���ȍ~���̂Ă�
    auto idx = assetName.find("*");
    if (idx != std::string::npos) {
      assetName = assetName.substr(0, idx);
    }

    // ����ɃX�t�B�A�}�b�v�P�̂̂Ƃ�������̂ł��̏ꍇ����������
    // ���Ȃ݂ɃX�t�B�A�}�b�v�͂���n�_�������360�x�̕��i����荞�񂾓���ȃe�N�X�`��
    // ���}�b�v�Ƃ��Ă΂ꂽ�������B����������ĉf�荞�݂⃉�C�e�B���O�Ɏg�����肷��
    // ����������͕s�v�Ȃ̂Ŗ����I

    // �蔲���̊g���q����
    idx = assetName.find(".spa");
    if (idx != std::string::npos) {
      continue; // �X�t�B�A�}�b�v���ۂ��̂Ŗ���
    }
    
    idx = assetName.find(".sph");
    if (idx != std::string::npos) {
      continue; // �X�t�B�A�}�b�v���ۂ��̂Ŗ���
    }


    // LoadWICTextureFromFile�� wchar_t* �ł����t�@�C�������󂯎���Ă���Ȃ��̂�
    // string����wstring�ɕϊ������i�߂�ǂ��j
    
    // �܂��̓t�@�C���������C�h�����ɕϊ������Ƃ��̃o�b�t�@�T�C�Y�����߂�
    auto bufSize = MultiByteToWideChar(CP_ACP, 0, assetName.c_str(), -1, nullptr, 0);
    // �ϊ�������������i�[����z��
    wchar_t* buffer = new wchar_t[bufSize];
    // �}���`�o�C�g����������C�h�����ɕϊ�
    MultiByteToWideChar(CP_ACP, 0, assetName.c_str(), -1, reinterpret_cast<LPWSTR>(buffer), bufSize);

    // �e�N�X�`���t�@�C�����[�h, �r���[�쐬
    Singleton<TextureManager>::instance().LoadWICTextureFromFile(device, buffer, assetName);
    auto resource = Singleton<TextureManager>::instance().texture(assetName);
    Singleton<TextureViewManager>::instance().CreateView(device, resource.Get());

    // ������o�b�t�@�̂Ă܂�
    delete[] buffer;

    m.textureFile = assetName;

    // �Q�[����MMD���g���Ȃ玖�O�ɂ��̂悤�ȑO�������{������Ԃ̃f�[�^�ɂ���
    // ���̃t�@�C����ǂݍ��ނƌ������悢
  }

  // �{�[����
  uint16_t boneCount{};
  ReadUint16(file, boneCount);

  // �{�[���ǂݍ���
  bones_.resize(boneCount);
  for (auto& b : bones_) {
    PmdBoneData data{};
    file.read(reinterpret_cast<char*>(&data), sizeof(data));

    b.name = data.name;
    b.parentIndex = data.parentIndex;
    b.childIndex = data.childIndex;
    b.boneType = data.boneType;
    b.IKBoneIndex = data.IKBoneIndex;
    b.position = data.position;
  }

  // IK��
  uint16_t ikCount{};
  ReadUint16(file, ikCount);

  // IK�ǂݍ���
  iks_.resize(ikCount);
  for (auto& ik : iks_) {
    PmdIkData data{};
    file.read(reinterpret_cast<char*>(&data), sizeof(data));

    ik.boneIndex = data.boneIndex;
    ik.targetBoneIndex = data.targetBoneIndex;
    ik.recursiveCount = data.recursiveCount;
    ik.countrolWeight = data.countrolWeight;

    // IK�̉e�����󂯂�{�[����IK���ɗv�f�����Ⴄ
    ik.childBoneIndices.resize(data.chainCount);
    for (auto& v : ik.childBoneIndices) {
      ReadUint16(file, v);
    }
  }

  // ����̓��[�t�B���O�͂��Ȃ����Ǔǂ�ł݂�
  // �\��[�t��
  uint16_t morphCount{};
  ReadUint16(file, morphCount);

  // MMD�̊�̕\��̓��[�t�B���O�ō���Ă����
  morphs_.resize(morphCount);
  for (auto& m : morphs_) {
    PmdMorphData data{};
    file.read(reinterpret_cast<char*>(&data), sizeof(data));

    m.name = data.name;
    m.vertexCount = data.vertexCount;
    m.type = data.type;
  }

  // ���̂��Ƃ��f�[�^�͑������Ǎ���͎g��Ȃ��̂ł����ł�߂Ă����܂�
  return true;
}

}  // namespace dxapp