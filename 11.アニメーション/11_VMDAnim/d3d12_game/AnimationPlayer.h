#pragma once

namespace dxapp {

class VmdFile;
class Model;

class AnimationPlayer {
 public:
  AnimationPlayer();
  ~AnimationPlayer();

  bool Initialize(VmdFile* vmd);
  void BindModel(Model* model);
  void Play();
  void Reset();

  void TogglePause();
  bool IsPause();

  float frameCount() const;

  void currentFrameTime(float t);
  float currentFrameTime() const;

  float playSpeed() const;
  void playSpeed(float s);

  void EnableAnimationInterprep(bool enable);
  bool IsAnimationInterprep() const;

  void EnableIK(bool enable);
  bool IsIK() const;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace dxapp
