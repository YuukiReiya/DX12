#include <algorithm>

#include "Device.hpp"
#include "Singleton.hpp"
#include "TextureManager.hpp"
#include "Utility.hpp"

#include "PmdFile.hpp"

using namespace DirectX;

namespace {

// PMDデータ読み込み用の中間形式

#pragma pack(push, 1)
// pragma pack はクラスや構造体メンバーのアラインメントを指定する
// #pragma pack(push, 1) すると1バイトでパッキングされる

#pragma region pmd
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
  uint8_t boneType;
  uint16_t IKBoneIndex;
  XMFLOAT3 position;
};
struct PmdIkData {
  uint16_t boneIndex;
  uint16_t targetBoneIndex;
  uint8_t chainCount;
  uint16_t recursiveCount;
  float countrolWeight;
};

struct PmdMorphData {
  char name[20];
  uint32_t vertexCount;
  uint8_t type;
};
#pragma endregion

#pragma region vmd
struct VmdHeaderData {
  char header[30];
  char modelName[20];
};

struct VmdMorphFrameData {
  char name[15];
  uint32_t keyframe;
  float weight;
};

#pragma endregion

// #pragma pack(pop) で対になる packを解除する
#pragma pack(pop)

union uint32_char4 {
  uint32_t u32;
  char b[4];
};

union uint16_char2 {
  uint16_t u16;
  char b[2];
};

union float_char4 {
  float f;
  char b[4];
};

// uint32_tを読んでvに格納
void ReadUint32(std::ifstream& ifs, uint32_t& v) {
  uint32_char4 data{};
  ifs.read(data.b, 4);
  v = data.u32;
}

// uint16_tを読んでvに格納
void ReadUint16(std::ifstream& ifs, uint16_t& v) {
  uint16_char2 data{};
  ifs.read(data.b, 2);
  v = data.u16;
}

// floatを読んでvに格納
void ReadFloat(std::ifstream& ifs, float& v) {
  float_char4 data{};
  ifs.read(data.b, 4);
  v = data.f;
}

void ReadXMFLOAT3(std::ifstream& ifs, DirectX::XMFLOAT3& v) {
  ReadFloat(ifs, v.x);
  ReadFloat(ifs, v.y);
  ReadFloat(ifs, v.z);
}

void ReadXMFLOAT4(std::ifstream& ifs, DirectX::XMFLOAT4& v) {
  ReadFloat(ifs, v.x);
  ReadFloat(ifs, v.y);
  ReadFloat(ifs, v.z);
  ReadFloat(ifs, v.w);
}
}  // unnamed namespace

namespace dxapp {

#pragma region pmd
bool PmdFile::Load(const std::wstring& fileName, Device* device) {
  std::ifstream file(fileName, std::ios::binary);

  PmdHeaderData header{};
  // read関数はバイト単位でデータを読む
  file.read(reinterpret_cast<char*>(&header), sizeof(header));

  if (header.magic[0] != 'P' || header.magic[1] != 'm' ||
      header.magic[2] != 'd') {
    // 今回はファイルマジック（ファイルの識別情報）が違ってたら終了
    wchar_t buf[256] = {};
    swprintf(buf, 256, L"%s は Pmdじゃないようですよ\n", fileName.c_str());
    OutputDebugString(buf);
    return false;
  }

  name_ = header.name;
  version_ = header.version;
  comment_ = header.comment;

  // バイナリファイルなのでreadでチクチク読んでいきますよ

  // ヘッダのあとに4バイトで頂点数がはいっています
  uint32_t vertexCount{};
  ReadUint32(file, vertexCount);

  // 頂点配列を確保
  vertices_.resize(vertexCount);

  // 頂点数の後には、頂点データが頂点数分並んでいますよ
  // 頂点数分一気に読んじゃえ
  file.read(reinterpret_cast<char*>(vertices_.data()),
            sizeof(PmdVertex) * vertexCount);

  // 頂点のあとに4バイトでインデックス数
  uint32_t indexCount{};
  ReadUint32(file, indexCount);

  // インデックス配列を確保して読み込み
  indices_.resize(indexCount);
  file.read(reinterpret_cast<char*>(indices_.data()),
            sizeof(uint16_t) * indexCount);

  // つぎは4バイトでマテリアル数
  uint32_t materialCount{};
  ReadUint32(file, materialCount);

  // マテリアルデータを読み込み
  materials_.resize(materialCount);
  for (auto& m : materials_) {
    // マテリアルはテクスチャファイル名があるのでPmdMaterialDataで読んでから
    // PmdMaterialに入れなおしますね
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

    // テクスチャファイル名を処理
    std::string assetName = data.textureName;
    if (assetName.empty()) {
      continue;  // ファイル名がないならスキップ
    }

    // ファイル名にはディレクトリ情報がないので強引に足しています（スマートじゃないね）
    assetName = "Assets/" + assetName;

    // mmdのテクスチャファイル名にはカラーマップ名かスフィアマップ名のどちらかが、
    // または二つが **一緒 **に格納されている
    // 一緒に格納されている場合は "カラーマップ*スフィアマップ" と "*"
    // で区切られる そこでまず textureNameに"*"があればそれ以降を捨てる
    auto idx = assetName.find("*");
    if (idx != std::string::npos) {
      assetName = assetName.substr(0, idx);
    }

    // さらにスフィアマップ単体のときもあるのでその場合も無視する
    // ちなみにスフィアマップはある地点から周囲360度の風景を取り込んだ特殊なテクスチャ
    // 環境マップとか呼ばれたりもする。それをつかって映り込みやライティングに使ったりする
    // ただし今回は不要なので無視！

    // 手抜きの拡張子判定
    idx = assetName.find(".spa");
    if (idx != std::string::npos) {
      continue;  // スフィアマップっぽいので無視
    }

    idx = assetName.find(".sph");
    if (idx != std::string::npos) {
      continue;  // スフィアマップっぽいので無視
    }

    // LoadWICTextureFromFileが wchar_t*
    // でしかファイル名を受け取ってくれないので
    // stringからwstringに変換するよ（めんどい）

    // まずはファイル名をワイド文字に変換したときのバッファサイズを求める
    auto bufSize =
        MultiByteToWideChar(CP_ACP, 0, assetName.c_str(), -1, nullptr, 0);
    // 変換した文字列を格納する配列
    wchar_t* buffer = new wchar_t[bufSize];
    // マルチバイト文字列をワイド文字に変換
    MultiByteToWideChar(CP_ACP, 0, assetName.c_str(), -1,
                        reinterpret_cast<LPWSTR>(buffer), bufSize);

    // テクスチャファイルロード, ビュー作成
    Singleton<TextureManager>::instance().LoadWICTextureFromFile(device, buffer,
                                                                 assetName);
    auto resource = Singleton<TextureManager>::instance().texture(assetName);
    Singleton<TextureViewManager>::instance().CreateView(device,
                                                         resource.Get());

    // 文字列バッファ捨てます
    delete[] buffer;

    m.textureFile = assetName;

    // ゲームでMMDを使うなら事前にこのような前処理を施した状態のデータにして
    // そのファイルを読み込むと効率がよい
  }

  // ボーン数
  uint16_t boneCount{};
  ReadUint16(file, boneCount);

  // ボーン読み込み
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

  // IK数
  uint16_t ikCount{};
  ReadUint16(file, ikCount);

  // IK読み込み
  iks_.resize(ikCount);
  for (auto& ik : iks_) {
    PmdIkData data{};
    file.read(reinterpret_cast<char*>(&data), sizeof(data));

    ik.boneIndex = data.boneIndex;
    ik.targetBoneIndex = data.targetBoneIndex;
    ik.recursiveCount = data.recursiveCount;
    ik.countrolWeight = data.countrolWeight;

    // IKの影響を受けるボーンはIK毎に要素数が違う
    ik.childBoneIndices.resize(data.chainCount);
    for (auto& v : ik.childBoneIndices) {
      ReadUint16(file, v);
    }
  }

  // 表情モーフ数
  uint16_t morphCount{};
  ReadUint16(file, morphCount);

  // MMDの顔の表情はモーフィングで作られているよ
  morphs_.resize(morphCount);
  for (auto& m : morphs_) {
    PmdMorphData data{};
    auto s = sizeof(data);
    file.read(reinterpret_cast<char*>(&data), s);

    m.name = data.name;
    m.vertexCount = data.vertexCount;
    // モーフタイプにキャスト
    m.type = static_cast<PmdMorph::Type>(data.type);

    //---
    // モーフデータの読み込みをしていなくてバグっていたので
    // 今回はちゃんとリードするようにしました！
    m.indices.resize(m.vertexCount);
    m.positions.resize(m.vertexCount);
    for (uint32_t i = 0; i < m.vertexCount; i++) {
      ReadUint32(file, m.indices[i]);
      ReadXMFLOAT3(file, m.positions[i]);
    }
  }
  // このあともデータは続くけど今回は使わないのでここでやめておきます
  return true;
}
#pragma endregion

#pragma region vmd
bool VmdFile::Load(const std::wstring& fileName) {
  std::ifstream file(fileName, std::ios::binary);

  VmdHeaderData header{};
  file.read(reinterpret_cast<char*>(&header), sizeof(header));
  header_ = header.header;

  {  // モーションデータ構築
    uint32_t count{};
    ReadUint32(file, count);

    // フレームデータを読み込み
    for (uint32_t i = 0; i < count; i++) {
      VmdMotionFrame frame{};

      char name[15];
      file.read(name, sizeof(name));
      frame.name = name;

      ReadUint32(file, frame.keyframe);
      ReadXMFLOAT3(file, frame.location);
      ReadXMFLOAT4(file, frame.rotation);

      file.read(reinterpret_cast<char*>(&frame.interpolation),
                sizeof(frame.interpolation));

      // とりあえずフレームをコンテナに突っ込む
      motions_[frame.name].emplace_back(frame);
    }

    for (auto& motion : motions_) {
      // vmdのキーフレームは昇順で並んでないので自分でソートしよう
      std::sort(
          std::begin(motion.second), std::end(motion.second),
          [](const auto& a, const auto& b) { return a.keyframe < b.keyframe; });

      // モーションの名前を覚える
      motionNames_.emplace_back(motion.first);

      // キーフレームの終わりを探す
      auto it = motion.second.rbegin();
      keyframeCount_ = std::max(keyframeCount_, it->keyframe);
    }
  }

  {  // モーフデータ構築
    uint32_t count{};
    ReadUint32(file, count);

    // フレームデータを読み込み
    for (uint32_t i = 0; i < count; i++) {
      VmdMorphFrameData data{};
      file.read(reinterpret_cast<char*>(&data), sizeof(data));

      VmdMorphFrame frame{};
      frame.name = data.name;
      frame.keyframe = data.keyframe;
      frame.weight = data.weight;

      morphs_[frame.name].emplace_back(frame);
    }

    for (auto& morph : morphs_) {
      // モーフのキーフレームもソートしよう
      std::sort(
          std::begin(morph.second), std::end(morph.second),
          [](const auto& a, const auto& b) { return a.keyframe < b.keyframe; });

      // モーフの名前を覚える
      morphNames_.emplace_back(morph.first);
    }
  }
  return true;
}
#pragma endregion
}  // namespace dxapp
