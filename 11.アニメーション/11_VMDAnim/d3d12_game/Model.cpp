#include "BufferObject.hpp"
#include "Device.hpp"
#include "ModelMaterial.hpp"
#include "PmdFile.hpp"

#include "Model.hpp"

namespace dxapp {
using namespace DirectX;

#pragma region Impl
class Model::Impl {
 public:
  Impl();
  ~Impl();

  bool Initialize(Device* device, const PmdFile* pmd);
  void UpdateTransform(float deltaTime);
  void CalculateBoneMatrices();
  void CalculateMorph();
  void UpdateBuffers();

  uint32_t backbufferIndex_{};

  std::vector<MeshParts> mesh_;  //!< パーツを集めてメッシュとする
  std::vector<ModelVertex> vertices_;  //!< PMDから取り出した頂点
  std::vector<uint16_t> indices_;  //!< PMDから取り出したインデクス

  std::vector<std::unique_ptr<BufferObject>> vertexBuffers_;  //!< 頂点バッファ
  std::unique_ptr<BufferObject> indexBuffer_;  //!< インデクスバッファ
  D3D12_VERTEX_BUFFER_VIEW vbView_{};          //!< バッファビュー
  D3D12_INDEX_BUFFER_VIEW ibView_{};           //!< バッファビュー

  std::vector<std::unique_ptr<ModelMaterial>> materials_;  //!< 描画用マテリアル
  Transform transform_;
  ObjectParameter objectParam_{};
  std::vector<std::unique_ptr<BufferObject>> objectCBuffers_;

  std::vector<std::unique_ptr<Bone>> bones_;  //!< ボーン
  std::vector<IKBone> ikBones_;               //!< IKボーン
  MorphData morphData_;                       //!< モーフ
};

Model::Impl::Impl() {}
Model::Impl::~Impl() {}

bool Model::Impl::Initialize(Device* device, const PmdFile* pmd) {
  objectCBuffers_.resize(device->backBufferSize());
  for (auto& cb : objectCBuffers_) {
    cb = std::make_unique<BufferObject>();
    cb->Initialize(device->device(), BufferObjectType::ConstantBuffer,
                   sizeof(objectParam_));
  }

  // pmdからモデルを構築していくよ
  {  // 頂点データ構築
    auto count = pmd->vertexCount();
    vertices_.resize(count);

    for (uint32_t i = 0; i < count; i++) {
      auto& v = vertices_[i];
      auto data = pmd->vertex(i);

      v.position = data.position;
      v.normal = data.normal;
      v.uv = data.uv;
      v.boneID = XMUINT2(data.boneID[0], data.boneID[1]);
      // boneWeightsは頂点がボーンから受ける影響度（重みといいます）のこと
      // 0～100で入っているので 0.0から1.0で正規化
      // あとで計算するときに1.0までのほうが楽
      v.boneWeights.x = data.boneWeight / 100.0f;
      v.boneWeights.y = (100 - data.boneWeight) / 100.0f;
    }

    // 頂点バッファ作成
    vertexBuffers_.resize(device->backBufferSize());
    for (auto& vb : vertexBuffers_) {
      vb = std::make_unique<BufferObject>();
      vb->Initialize(device->device(), BufferObjectType::VertexBuffer,
                     sizeof(ModelVertex) * count);
      vb->Update(vertices_.data(), sizeof(ModelVertex) * count);
    }
  }

  {  // インデクス
    auto count = pmd->indexCount();
    indices_.resize(count);
    for (uint32_t i = 0; i < count; i++) {
      indices_[i] = pmd->index(i);
    }
    // インデクスバッファ
    indexBuffer_ = std::make_unique<BufferObject>();
    indexBuffer_->Initialize(device->device(), BufferObjectType::IndexBuffer,
                             sizeof(uint16_t) * count);
    indexBuffer_->Update(indices_.data(), sizeof(uint16_t) * count);
    // バッファービュー
    ibView_.BufferLocation = indexBuffer_->resource()->GetGPUVirtualAddress();
    ibView_.Format = DXGI_FORMAT_R16_UINT;
    ibView_.SizeInBytes = sizeof(uint16_t) * count;
  }

  {  //マテリアル
    auto count = pmd->materialCount();
    materials_.resize(count);
    for (uint32_t i = 0; i < count; i++) {
      materials_[i] = std::make_unique<ModelMaterial>();
      auto& m = materials_[i];
      m->Initialize(device);

      auto data = pmd->material(i);
      ModelMaterial::Material mat{};
      // Pmdのアルファはディフューズカラーのαにまとめる
      mat.diffuse =
          XMFLOAT4(data.diffuse.x, data.diffuse.y, data.diffuse.z, data.alpha);
      // specularity（スペキュラーの強度）はspecularのαにまとめる
      mat.specular = XMFLOAT4(data.specular.x, data.specular.y, data.specular.z,
                              data.specularity);
      mat.ambient = data.ambient;

      TextureView view{};
      if (data.textureFile.empty()) {
        //　ファイル名がないときはテクスチャなし
        //　ダミーテクスチャを渡しておく
        mat.useTexture = 0;
        auto resource =
            Singleton<TextureManager>::instance().texture("uv_checker");
        view = Singleton<TextureViewManager>::instance().CreateView(
            device, resource.Get());
      } else {
        mat.useTexture = 1;
        auto resource =
            Singleton<TextureManager>::instance().texture(data.textureFile);
        view = Singleton<TextureViewManager>::instance().CreateView(
            device, resource.Get());
      }
      m->SetMaterial(mat, view);
    }
  }

  {  // メッシュ構築
    // mmdはメッシュの頂点がずらっとならんでいるだけなのでそのままだと描画には使えない
    // んでmmdは1マテリアルと1メッシュパーツでできている
    // つまりマテリアル数とメッシュのパーツ数が同じ.
    auto count = pmd->materialCount();

    // マテリアルには使っている頂点数がはいっている.
    // さらに頂点データの並び順はマテリアルの順序と一緒
    // 下図のようになってるわけですな

    // *マテリアルの並びと使う頂点数*
    // |      mat0     |      mat1       |      mat2       |
    // |     100vtx    |     20vtx       |     200vtx      |
    // *頂点の並び*
    // | vtx0 - vtx99  | vtx100 - vtx120 | vtx121 - vtx320 |

    // というわけで、マテリアルを先頭から見ていく
    uint32_t offset = 0;  //!< メッシュパーツが使っている頂点の開始地点
    for (uint32_t i = 0; i < count; i++) {
      auto& data = pmd->material(i);
      // マテリアルの頂点数と、offsetによりパーツで使われている
      // 頂点インデクスがわかりやす
      mesh_.emplace_back(MeshParts{offset, data.vertexCount});
      offset += data.vertexCount;
    }
  }

  {    // 表情データ構築
    {  // 基本表情データ（Pmdは0番目は基本表情）
      auto base = pmd->morph(0);
      morphData_.basePositions = base.positions;
      morphData_.baseIndices = base.indices;
    }

    auto count = pmd->mourphCount() - 1;
    morphData_.morphs.resize(count);
    for (uint32_t i = 0; i < count; i++) {
      auto data = pmd->morph(i + 1);
      auto& morph = morphData_.morphs[i];

      morph.name = data.name;
      morph.offsetPositions = data.positions;
      morph.offsetIndices = data.indices;
      morph.weight = 0.0f;
    }
  }

  {  // ボーン構築
    auto count = pmd->boneCount();
    bones_.resize(count);

    // ボーンデータ作成
    for (uint32_t i = 0; i < count; i++) {
      const auto& data = pmd->bone(i);
      auto pos = data.position;
      // 親がいるか？(0xFFFFだと親なし)
      if (data.parentIndex != 0xFFFF) {
        const auto& parent = pmd->bone(data.parentIndex);
        // 親がいるときは、骨の位置を親との差分にする
        pos.x -= parent.position.x;
        pos.y -= parent.position.y;
        pos.z -= parent.position.z;
      }

      bones_[i] = std::make_unique<Bone>(data.name);
      auto& bone = bones_[i];
      bone->baseTranslation(pos);
      bone->translation(pos);

      //
      auto m = XMMatrixTranslationFromVector(XMLoadFloat3(&data.position));
      bone->invBindPoseMatrix(XMMatrixInverse(nullptr, m));
    }

    // 親子の関連付け
    for (uint32_t i = 0; i < count; i++) {
      const auto& data = pmd->bone(i);
      if (data.parentIndex != 0xFFFF) {
        bones_[i]->parent(bones_[data.parentIndex].get());
      }
    }
    CalculateBoneMatrices();
  }

  {  // IK構築
    auto count = pmd->ikCount();
    ikBones_.resize(count);

    for (uint32_t i = 0; i < count; i++) {
      const auto& data = pmd->ik(i);

      auto& ik = ikBones_[i];
      auto targetBone = bones_[data.targetBoneIndex].get();
      auto effectorBone = bones_[data.boneIndex].get();
      ik = IKBone(targetBone, effectorBone);
      ik.recursiveCount(data.recursiveCount);
      ik.limitAngle(data.countrolWeight);

      // このIKで計算されるボーン覚える
      std::vector<Bone*> ikChain;
      for (auto index : data.childBoneIndices) {
        ikChain.emplace_back(bones_[index].get());
      }
      ik.ikChain(ikChain);
    }
  }
#pragma endregion
  return true;
}

void Model::Impl::UpdateTransform(float deltaTime) {
  auto keyState = Keyboard::Get().GetState();
  auto mouseState = Mouse::Get().GetState();
  if (!mouseState.rightButton) {
    if (keyState.W) {
      transform_.rotation.x += XMConvertToRadians(50 * deltaTime);
    }
    if (keyState.S) {
      transform_.rotation.x -= XMConvertToRadians(50 * deltaTime);
    }
    if (keyState.A) {
      transform_.rotation.y -= XMConvertToRadians(50 * deltaTime);
    }
    if (keyState.D) {
      transform_.rotation.y += XMConvertToRadians(50 * deltaTime);
    }
  }

  auto t = XMMatrixTranslation(0, -10, 0);
  auto r = XMMatrixRotationRollPitchYaw(
      transform_.rotation.x, transform_.rotation.y, transform_.rotation.z);
  auto s = XMMatrixScaling(transform_.scale.x, transform_.scale.y,
                           transform_.scale.z);
  auto m = t * s * r;
  XMStoreFloat4x4(&objectParam_.world, XMMatrixTranspose(m));
}

void Model::Impl::CalculateMorph() {
  {  // モーフで変わった頂点座標を初期値に戻す
    const auto count = morphData_.basePositions.size();
    for (uint32_t i = 0; i < count; i++) {
      auto index = morphData_.baseIndices[i];
      vertices_[index].position = morphData_.basePositions[i];
    }
  }

  // すべてのモーフィングデータを計算
  const auto morphCount = morphData_.morphs.size();
  for (uint32_t i = 0; i < morphCount; i++) {
    const auto& morph = morphData_.morphs[i];
    const auto weight = morph.weight;

    const auto indexCount = morph.offsetIndices.size();
    for (uint32_t count = 0; count < indexCount; count++) {
      auto baseFaicialIndex = morph.offsetIndices[count];
      auto offsetPosition = morph.offsetPositions[count];

      // 書き換え先のインデクス
      auto index = morphData_.baseIndices[baseFaicialIndex];
      // オフセット座標にウェイトを掛け合わせてベースの座標に加算
      vertices_[index].position.x += offsetPosition.x * weight;
      vertices_[index].position.y += offsetPosition.y * weight;
      vertices_[index].position.z += offsetPosition.z * weight;
    }
  }
}

void Model::Impl::CalculateBoneMatrices() {
  for (auto& b : bones_) {
    if (b->parent()) {
      continue;  // 子供の場合は、親が更新してくれるのでスルー
    }
    b->Update();
  }
}

void Model::Impl::UpdateBuffers() {
  // 定数バッファのデータ更新
  auto count = bones_.size();
  for (uint32_t i = 0; i < count; i++) {
    auto bone = bones_[i].get();
    auto m = bone->invBindPoseMatrix() * bone->worldMatrix();
    XMStoreFloat4x4(&objectParam_.bone[i], XMMatrixTranspose(m));
  }

  // モデル用の定数バッファ更新
  objectCBuffers_[backbufferIndex_]->Update(&objectParam_,
                                            sizeof(objectParam_));

  // 頂点バッファ更新, モーフで頂点を書き換えているので。
  vertexBuffers_[backbufferIndex_]->Update(
      vertices_.data(), sizeof(ModelVertex) * vertices_.size());
  vbView_.BufferLocation =
      vertexBuffers_[backbufferIndex_]->resource()->GetGPUVirtualAddress();
  vbView_.StrideInBytes = sizeof(ModelVertex);
  vbView_.SizeInBytes = vertices_.size() * sizeof(ModelVertex);

  // マテリアル更新
  for (auto& m : materials_) {
    m->Update(backbufferIndex_);
  }
}

#pragma endregion

//---

#pragma region Model
Model::Model() : impl_(std::make_unique<Impl>()) {}
Model::~Model() {}

bool Model::Initialize(Device* device, const PmdFile* pmd) {
  return impl_->Initialize(device, pmd);
}

void Model::Update(float deltaTime, Device* device) {
  ImGui::SetNextWindowSize(ImVec2(350, 300), ImGuiCond_Once);
  ImGui::Begin(
      u8"モーフィング");  // 文字列リテラルのまえにu8つけるとutf8エンコード
  {
    for (auto& f : impl_->morphData_.morphs) {
      // モーフィングの名前はマルチバイト文字なのでutf8にエンコードしないと日本語だとでないのよ・・・
      ImGui::SliderFloat(f.name.c_str(), &f.weight, 0.0f, 1.0f);
    }
  }
  ImGui::End();

  impl_->backbufferIndex_ = device->backBufferIndex();
  impl_->UpdateTransform(deltaTime);
  impl_->CalculateMorph();
  impl_->UpdateBuffers();
}

void Model::CalculateBoneMatrices() { impl_->CalculateBoneMatrices(); }

void Model::rotation(const DirectX::XMFLOAT3& rot) {
  impl_->transform_.rotation = rot;
}

D3D12_GPU_VIRTUAL_ADDRESS Model::objectParamBufferAddress() const {
  return impl_->objectCBuffers_[impl_->backbufferIndex_]
      ->resource()
      ->GetGPUVirtualAddress();
}

D3D12_VERTEX_BUFFER_VIEW* Model::vertexBufferView() const {
  return &impl_->vbView_;
}

D3D12_INDEX_BUFFER_VIEW* Model::indexBufferView() const {
  return &impl_->ibView_;
}

std::size_t Model::meshPartsCount() const { return impl_->mesh_.size(); }

std::size_t Model::materialCount() const { return impl_->materials_.size(); }

const Model::MeshParts& Model::meshParts(uint32_t i) const {
  return impl_->mesh_[i];
}

const ModelMaterial& Model::material(uint32_t i) const {
  return *(impl_->materials_[i].get());
}

std::size_t Model::boneCount() const { return impl_->bones_.size(); }

std::size_t Model::ikBoneCount() const { return impl_->ikBones_.size(); }

void Model::morphWeight(const std::string& name, float weight) {
  if (name == "base") {
    // 基本表情は変化しない
    return;
  }

  for (auto& m : impl_->morphData_.morphs) {
    if (m.name == name) {
      m.weight = weight;
      return;
    }
  }
}

Bone* Model::bone(uint32_t i) const { return impl_->bones_[i].get(); }

IKBone& Model::ikBone(uint32_t i) const { return impl_->ikBones_[i]; }

#pragma endregion
}  // namespace dxapp
