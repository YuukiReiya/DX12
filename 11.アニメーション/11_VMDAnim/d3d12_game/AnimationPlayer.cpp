#include "Model.hpp"
#include "PmdFile.hpp"

#include "AnimationPlayer.h"

using namespace DirectX;

namespace {
struct BoneAnimFrame {
  uint32_t frame;        //!< キーフレーム
  XMFLOAT3 translation;  //!< 移動量
  XMFLOAT4 rotation;     //!< 回転
  // 以下ベジェ補完パラメータ
  XMFLOAT4 interpX;
  XMFLOAT4 interpY;
  XMFLOAT4 interpZ;
  XMFLOAT4 interpR;
};

struct MorphAnimFrame {
  uint32_t frame;  //!< キーフレーム
  float weight;    //!< モーフィング　ウェイト
};

struct BoneAnimation {
  std::vector<BoneAnimFrame> keyframes;
};

struct MorphAnimation {
  std::vector<MorphAnimFrame> keyframes;
};

//---
// モーション計算用関数

static float dFx(float ax, float ay, float t) {
  float s = 1.0f - t;
  float v = -6.0f * s * t * t * ax + 3.0f * s * s * ax - 3.0f * t * t * ay +
            6.0f * s * t * ay + 3.0f * t * t;
  return v;
}

static float fx(float ax, float ay, float t, float x0) {
  float s = 1.0f - t;
  return 3.0f * s * s * t * ax + 3.0f * s * t * t * ay + t * t * t - x0;
}

static float funcBezierY(XMFLOAT4 k, float t) {
  float s = 1.0f - t;
  return 3.0f * s * s * t * k.y + 3.0f * s * t * t * k.w + t * t * t;
}

static XMFLOAT3 QuaternionToEuler(FXMVECTOR q) {
  XMFLOAT4 quat{};
  XMStoreFloat4(&quat, q);

  float x2 = quat.x + quat.x;
  float y2 = quat.y + quat.y;
  float z2 = quat.z + quat.z;
  float xz2 = quat.x * z2;
  float wy2 = quat.w * y2;

  float t = -(xz2 - wy2);
  if (t >= 1.0f) {
    t = 1.0f;
  } else if (t <= -1.0f) {
    t = -1.0f;
  }
  float yRadian = asinf(t);

  float xx2 = quat.x * x2;
  float xy2 = quat.x * y2;
  float zz2 = quat.z * z2;
  float wz2 = quat.w * z2;

  XMFLOAT3 euler{};
  if (yRadian < XM_PI * 0.5f) {
    if (yRadian > -XM_PI * 0.5f) {
      float yz2 = quat.y * z2;
      float wx2 = quat.w * x2;
      float yy2 = quat.y * y2;
      euler.x = atan2f((yz2 + wx2), (1.0f - (xx2 + yy2)));
      euler.y = yRadian;
      euler.z = atan2f((xy2 + wz2), (1.0f - (yy2 + zz2)));
    } else {
      euler.x = -atan2f((xy2 - wz2), (1.0f - (xx2 + zz2)));
      euler.y = yRadian;
      euler.z = 0.0f;
    }
  } else {
    euler.x = atan2f((xy2 - wz2), (1.0f - (xx2 + zz2)));
    euler.y = yRadian;
    euler.z = 0.0f;
  }
  return euler;
}

static XMVECTOR EulerToQuaternion(const XMFLOAT3& e) {
  float xr = e.x * 0.5f;
  float yr = e.y * 0.5f;
  float zr = e.z * 0.5f;

  float sinX = sinf(xr);
  float cosX = cosf(xr);
  float sinY = sinf(yr);
  float cosY = cosf(yr);
  float sinZ = sinf(zr);
  float cosZ = cosf(zr);

  XMFLOAT4 q{};
  q.x = sinX * cosY * cosZ - cosX * sinY * sinZ;
  q.y = cosX * sinY * cosZ + sinX * cosY * sinZ;
  q.z = cosX * cosY * sinZ - sinX * sinY * cosZ;
  q.w = cosX * cosY * cosZ + sinX * sinY * sinZ;

  return XMLoadFloat4(&q);
}
}  // namespace

namespace dxapp {

#pragma region Impl

class AnimationPlayer::Impl {
 public:
  Impl();
  ~Impl();

  bool Initialize(VmdFile* vmd);

  //! 指定時間でのボーンアニメーション計算
  void CalculateBone(uint32_t time);
  //! 指定時間でのモーフ計算
  void CalculatMorph(uint32_t time);
  //! 現在のボーンデータからIK計算
  void CalculatIK();

  Model* model_{nullptr};  //!< アニメーション対象のモデル

  float frameTime_{};         //!< 再生時間
  uint32_t keyframeCount_{};  //!< フレームの終わりの時間

  float playSpeed_{1.0f};  //!< 再生速度
  bool isPause_{false};    //!< 一時停止トグル
  bool isInterp{false};    //!< アニメーション補完トグル
  bool isIK_{false};       //!< IK トグル

 private:
  //! IK計算
  void IKSolve(IKBone& ikBone);
  //! ベジエ補完計算
  float InterpBezier(const XMFLOAT4& bezier, float x);

  //! ボーンアニメーションの集合
  std::unordered_map<std::string, BoneAnimation> boneAnimations_;
  //! ボーンアニメーションの集合
  std::unordered_map<std::string, MorphAnimation> morphAnimations_;
};

AnimationPlayer::Impl::Impl() {}
AnimationPlayer::Impl::~Impl() {}

bool AnimationPlayer::Impl::Initialize(VmdFile* vmd) {
  keyframeCount_ = vmd->keyframeCount();

  {  // アニメーション構築
    auto motionCount = vmd->motionCount();

    for (uint32_t i = 0; i < motionCount; i++) {
      const auto& name = vmd->motionName(i);
      boneAnimations_[name] = BoneAnimation();

      auto& frameData = vmd->motion(name);
      auto frameCount = frameData.size();

      auto& keyframes = boneAnimations_[name].keyframes;
      keyframes.resize(frameCount);

      for (uint32_t count = 0; count < frameCount; count++) {
        auto& data = frameData[count];
        auto& keyframe = keyframes[count];

        keyframe.frame = data.keyframe;
        keyframe.translation = data.location;
        keyframe.rotation = data.rotation;
        keyframe.interpX = data.bezierParam(0);
        keyframe.interpY = data.bezierParam(1);
        keyframe.interpZ = data.bezierParam(2);
        keyframe.interpR = data.bezierParam(3);
      }
    }
  }

  {  // モーフアニメーション構築
    auto morphCount = vmd->morphCount();
    for (uint32_t i = 0; i < morphCount; i++) {
      const auto& name = vmd->morphName(i);
      morphAnimations_[name] = MorphAnimation();

      auto& frameData = vmd->morph(name);
      auto frameCount = frameData.size();

      auto& keyframes = morphAnimations_[name].keyframes;
      keyframes.resize(frameCount);

      for (uint32_t count = 0; count < frameCount; count++) {
        auto& data = frameData[count];
        auto& keyframe = keyframes[count];

        keyframe.frame = data.keyframe;
        keyframe.weight = data.weight;
      }
    }
  }
  return true;
}

float AnimationPlayer::Impl::InterpBezier(const XMFLOAT4& bezier, float x) {
  float t = 0.5f;
  float ft = fx(bezier.x, bezier.z, t, x);
  for (int i = 0; i < 32; ++i) {
    auto dfx = dFx(bezier.x, bezier.z, t);
    t = t - ft / dfx;
    ft = fx(bezier.x, bezier.z, t, x);
  }
  t = std::min(std::max(0.0f, t), 1.0f);
  return funcBezierY(bezier, t);
}

// ボーン更新
void AnimationPlayer::Impl::CalculateBone(uint32_t time) {
  auto boneCount = model_->boneCount();
  for (uint32_t i = 0; i < boneCount; i++) {
    auto bone = model_->bone(i);

    auto it = boneAnimations_.find(bone->name());
    if (it == boneAnimations_.end()) {
      continue;
    }
    auto& keyframes = it->second.keyframes;

    if (!isInterp) {
      // 補完しないときはフレームデータをそのまま突っ込む
      // コマ送りみたいになる
      BoneAnimFrame* frame = nullptr;
      for (auto& f : keyframes) {
        if (f.frame == time) {
          frame = &f;
          break;
        }
      }
      if (!frame) continue;
      auto t = bone->baseTranslation();
      t += XMLoadFloat3(&frame->translation);
      bone->translation(t);
      bone->rotation(frame->rotation);
    } else {
      // フレームのモーションを補完する

      // 再生時間から次のキーフレームを探す
      BoneAnimFrame* frame2 = nullptr;
      for (auto& f : keyframes) {
        if (f.frame > frameTime_) {
          frame2 = &f;
          break;
        }
      }

      // 直近で通過したキーフレーム
      BoneAnimFrame* frame1 = nullptr;
      if (frame2 != nullptr) {
        frame1 = frame2 - 1;
      } else {
        //
        frame1 = &keyframes[keyframes.size() - 1];
        frame2 = &keyframes[0];
      }

      // キーフレーム間の時間 = つぎのキーフレーム -  直近キーフレーム
      float value = static_cast<float>(frame2->frame - frame1->frame);
      if (value == 0) {
        continue;
      }

      // ベジェ補完
      auto rate = (time - frame1->frame) / value;

      // 補完パラメータ計算
      XMFLOAT4 interp{};
      interp.x = InterpBezier(frame1->interpX, rate);
      interp.y = InterpBezier(frame1->interpY, rate);
      interp.z = InterpBezier(frame1->interpZ, rate);
      interp.w = InterpBezier(frame1->interpR, rate);
      XMVECTOR bezier = XMLoadFloat4(&interp);

      auto t = XMLoadFloat3(&frame1->translation);
      // frame間での移動量の差分
      XMVECTOR s = XMLoadFloat3(&frame2->translation) -
                   XMLoadFloat3(&frame1->translation);
      // 補完してこの時間での移動量を加算
      t += s * bezier;
      // boneの基本位置も可算
      t += bone->baseTranslation();
      bone->translation(t);

      // 回転は球面線形補間
      auto from = XMLoadFloat4(&frame1->rotation);
      auto to = XMLoadFloat4(&frame2->rotation);
      auto r = XMQuaternionSlerp(from, to, interp.w);
      bone->rotation(r);
    }
  }
  // アニメーションによりボーンの位置がきまったので、骨の行列を計算
  model_->CalculateBoneMatrices();
}

// モーフ更新
void AnimationPlayer::Impl::CalculatMorph(uint32_t time) {
  for (auto& morph : morphAnimations_) {
    auto& name = morph.first;

    auto& keyframes = morph.second.keyframes;
    // 再生時間から次のキーフレームを探す
    MorphAnimFrame* frame2 = nullptr;
    for (auto& f : keyframes) {
      if (f.frame > time) {
        frame2 = &f;
        break;
      }
    }

    // 直近のキーフレーム
    MorphAnimFrame* frame1 = nullptr;
    if (frame2 != nullptr) {
      frame1 = frame2 - 1;
    } else {
      frame1 = &keyframes[keyframes.size() - 1];
      frame2 = &keyframes[0];
    }

    // キーフレーム間の時間 = つぎのキーフレーム -  直近のキーフレーム
    auto value = static_cast<float>(frame2->frame - frame1->frame);
    auto weight = frame1->weight;
    if (value > 0) {
      // モーフはキーフレーム間の進行度からモーフィングのウェイトを求めるだけなのでお手軽
      auto rate = (time - frame1->frame) / value;
      weight += (frame2->weight - frame1->weight) * rate;
    }

    model_->morphWeight(name, weight);
  }
}

// IKの更新
void AnimationPlayer::Impl::CalculatIK() {
  auto count = model_->ikBoneCount();
  for (uint32_t i = 0; i < count; ++i) {
    IKSolve(model_->ikBone(i));
  }
}

// IKの解決
void AnimationPlayer::Impl::IKSolve(IKBone& ikBone) {
  // IKの末端
  auto target = ikBone.target();

  // IKの根本
  auto effector = ikBone.effector();

  // IKにより動く骨たち
  auto& ikChain = ikBone.ikChain();

  // VMDに記録してある計算回数（recursiveCount）だけIKの計算をする
  // これにより徐々にIKのターゲットに近付けていく
  // 一気に大きく動かすとボーンが暴れたりする・・・
  const auto recursiveCount = ikBone.recursiveCount();
  for (int count = 0; count < recursiveCount; count++) {
    // IKチェーンをめぐって計算していくよ
    const auto chainCount = ikChain.size();
    for (uint32_t i = 0; i < chainCount; i++) {
      auto& b = ikChain[i];
      auto m = XMMatrixInverse(nullptr, b->worldMatrix());

      // ターゲットとエフェクターの座標をもらう
      // 行列の4行目が座標
      auto targetPos = target->worldMatrix().r[3];
      auto effectorPos = effector->worldMatrix().r[3];
      // 現在のボーンを使ってトランスフォーム
      targetPos = XMVector3Transform(targetPos, m);
      effectorPos = XMVector3Transform(effectorPos, m);

      // ターゲットとエフェクターの位置が近いので処理しない
      auto len = XMVectorGetX(XMVector3LengthSq(effectorPos - targetPos));
      if (len < 0.0001f) {
        return;
      }

      // ボーンの回転を計算する
      // ボーンからターゲット（末端）とエフェクター（根本）へのベクトルを計算
      auto toTarget = XMVector3Normalize(targetPos);
      auto toEffector = XMVector3Normalize(effectorPos);

      // 角度計算
      auto dot = XMVectorGetX(XMVector3Dot(toEffector, toTarget));
      dot = std::min(1.0f, std::max(-1.0f, dot));
      // 十分に小さい角度なので処理中断
      float radian = acosf(dot);
      if (radian < 0.0001f) {
        continue;
      }
      // 回転には1回で回転できる制限角度がある
      auto limitAngle = ikBone.limitAngle();
      // 回転角を制限
      radian = std::min(limitAngle, std::max(-limitAngle, radian));

      // 回転軸を求める
      auto axis = XMVector3Normalize(XMVector3Cross(toTarget, toEffector));
      if (radian < 0.0001f) {
        continue;
      }

      // ひざは特別処理！
      if (b->name().find("ひざ") != std::string::npos) {
        // 何度も計算するとひざが暴れるから
        // 最初の1回だけでやめるようにする（逆関節になったりする）
        if (count == 0) {
          // なんか面倒な計算、とりあえずこういうもんだと思っておこう
          auto& root = ikChain[ikChain.size() - 1];
          auto rootPos = root->worldMatrix().r[3];

          auto ep = effector->worldMatrix().r[3] - rootPos;
          auto bp = b->worldMatrix().r[3] - rootPos;
          auto tp = target->worldMatrix().r[3] - b->worldMatrix().r[3];

          auto el = XMVectorGetX(XMVector3Length(ep));
          auto bl = XMVectorGetX(XMVector3Length(bp));
          auto tl = XMVectorGetX(XMVector3Length(tp));

          float c = (el * el - bl * bl - tl * tl) / (2.0f * bl * tl);
          c = std::max(-1.0f, std::min(1.0f, c));

          auto angle = acos(c);
          // ひざ関節はX軸にしか回転しない
          axis = XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f);

          auto r = XMQuaternionRotationAxis(axis, angle);
          r = XMQuaternionMultiply(r, b->rotation());
          b->rotation(XMQuaternionNormalize(r));
        }
      } else {
        // ひざ以外は上で計算した軸で回転
        auto r = XMQuaternionRotationAxis(axis, radian);
        r = XMQuaternionMultiply(r, b->rotation());
        b->rotation(XMQuaternionNormalize(r));
      }

      // ボーン更新
      for (int index = i; index >= 0; index--) {
        ikChain[index]->UpdateWorldMatrix();
      }
      effector->UpdateWorldMatrix();
      target->UpdateWorldMatrix();
    }
  }
}
#pragma endregion

//---

#pragma region AnimationPlayer

AnimationPlayer::AnimationPlayer() : impl_(std::make_unique<Impl>()) {}

AnimationPlayer::~AnimationPlayer() {}

bool AnimationPlayer::Initialize(VmdFile* vmd) {
  return impl_->Initialize(vmd);
}

void AnimationPlayer::Play() {
  if (impl_->model_ == nullptr) return;
  if (impl_->isPause_) return;

  impl_->frameTime_ += impl_->playSpeed_;
  // 時間を0 ～ keyframeCount_にクランプ
  std::clamp(impl_->frameTime_, 0.0f,
             static_cast<float>(impl_->keyframeCount_));
  auto time = static_cast<uint32_t>(impl_->frameTime_);

  impl_->CalculatMorph(time);
  impl_->CalculateBone(time);

  if (impl_->isIK_) {
    impl_->CalculatIK();
  }
}

void AnimationPlayer::BindModel(Model* model) {
  assert(model != nullptr);
  impl_->model_ = model;
}

void AnimationPlayer::Reset() {
  playSpeed(1.0f);
  currentFrameTime(0.0f);
}

void AnimationPlayer::TogglePause() { impl_->isPause_ = !impl_->isPause_; }

bool AnimationPlayer::IsPause() { return impl_->isPause_; }

float AnimationPlayer::frameCount() const {
  return static_cast<float>(impl_->keyframeCount_);
}

void AnimationPlayer::currentFrameTime(float t) {
  impl_->frameTime_ = std::min(t, static_cast<float>(impl_->keyframeCount_));
}

float AnimationPlayer::currentFrameTime() const { return impl_->frameTime_; }

float AnimationPlayer::playSpeed() const { return impl_->playSpeed_; }

void AnimationPlayer::playSpeed(float s) { impl_->playSpeed_ = s; }

void AnimationPlayer::EnableAnimationInterprep(bool enable) {
  impl_->isInterp = enable;
}

bool AnimationPlayer::IsAnimationInterprep() const { return impl_->isInterp; }

void AnimationPlayer::EnableIK(bool enable) { impl_->isIK_ = enable; }

bool AnimationPlayer::IsIK() const { return impl_->isIK_; }

#pragma endregion
}  // namespace dxapp
