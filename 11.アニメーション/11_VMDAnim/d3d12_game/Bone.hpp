#pragma once
namespace dxapp {
// ボーン
class Bone {
 public:
  Bone() = delete;
  Bone(const std::string& name);
  ~Bone();

  void Update();
  void UpdateWorldMatrix();
  void AddChild(Bone* child) { children_.emplace_back(child); }

  const std::string& name() const { return name_; }

  void parent(Bone* parent);
  Bone* parent() const { return parent_; };

  void baseTranslation(const DirectX::XMFLOAT3& t) {
    baseTranslation_ = DirectX::XMLoadFloat3(&t);
  }
  void baseTranslation(DirectX::FXMVECTOR t) { baseTranslation_ = t; };
  DirectX::XMVECTOR baseTranslation() const { return baseTranslation_; }

  void translation(const DirectX::XMFLOAT3& trans) {
    translation_ = DirectX::XMLoadFloat3(&trans);
  }
  void translation(DirectX::FXMVECTOR t) { translation_ = t; }
  DirectX::XMVECTOR translation() const { return translation_; }

  void rotation(const DirectX::XMFLOAT4& r) {
    rotation_ = DirectX::XMLoadFloat4(&r);
  }
  void rotation(DirectX::FXMVECTOR r) { rotation_ = r; }
  DirectX::XMVECTOR rotation() const { return rotation_; }

  void invBindPoseMatrix(DirectX::FXMMATRIX m) { invBindPose_ = m; }
  DirectX::XMMATRIX invBindPoseMatrix() const { return invBindPose_; }

  DirectX::XMMATRIX localMatrix() const { return local_; }
  DirectX::XMMATRIX worldMatrix() const { return world_; }

 private:
  std::string name_{};
  Bone* parent_{nullptr};
  std::vector<Bone*> children_;

  DirectX::XMVECTOR baseTranslation_{};
  DirectX::XMVECTOR translation_{};
  DirectX::XMVECTOR rotation_{};
  DirectX::XMMATRIX invBindPose_;
  DirectX::XMMATRIX local_;
  DirectX::XMMATRIX world_;
};

// IK
class IKBone {
 public:
  IKBone(){};
  IKBone(Bone* target, Bone* effector) : target_(target), effector_(effector){};

  Bone* effector() const { return effector_; }
  Bone* target() const { return target_; }

  void ikChain(std::vector<Bone*>& ikChain) { ikChain_ = ikChain; }
  const std::vector<Bone*>& ikChain() const { return ikChain_; }

  void recursiveCount(uint32_t count) { recursiveCount_ = count; }
  uint32_t recursiveCount() const { return recursiveCount_; }

  void limitAngle(float angle) { limitAngle_ = angle; }
  float limitAngle() const { return limitAngle_; }

 private:
  Bone* effector_{nullptr};
  Bone* target_{nullptr};

  std::vector<Bone*> ikChain_;

  uint32_t recursiveCount_{};
  float limitAngle_{};
};

}  // namespace dxapp
