#include "Bone.hpp"

namespace dxapp {
using namespace DirectX;

Bone::Bone(const std::string& name)
    : name_(name),
      rotation_{XMQuaternionIdentity()},
      invBindPose_{XMMatrixIdentity()},
      local_{XMMatrixIdentity()},
      world_{XMMatrixIdentity()} {}

Bone::~Bone() {}

void Bone::parent(Bone* parent) {
  parent_ = parent;
  if (parent_ != nullptr) {
    // 親が設定されたら自分を子供にしてもらう
    parent_->AddChild(this);
  }
}

void Bone::Update() {
  // 自分を更新
  UpdateWorldMatrix();
  // 子供も更新
  for (auto child : children_) {
    child->Update();
  }
}

void Bone::UpdateWorldMatrix() {
  // まずは自分の行列を更新
  local_ = XMMatrixRotationQuaternion(rotation_) *
           XMMatrixTranslationFromVector(translation_);

  //　親（がいれば親）の行列を合成してボーンのワールド行列にする
  auto p = XMMatrixIdentity();
  if (parent_ != nullptr) {
    p = parent_->worldMatrix();
  }
  world_ = local_ * p;
}
}  // namespace dxapp
