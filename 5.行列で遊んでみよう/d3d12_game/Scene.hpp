#pragma once

namespace dxapp {
class Device;
class BasicShader;
class BasicNonTexShader;  // BasicShaderのテクスチャ無しver

class Scene {
 public:
  /*
   * @brief コンストラクタ
   */
  Scene();

  /*
   * @brief デストラクタ
   */
  virtual ~Scene();

  /*
   * @brief 初期化
   */
  void Initialize(Device* device);

  /*
   * @brief 終了処理
   */
  void Terminate();

  /*
   * @brief 更新
   */
  void Update(float deltaTime);

  /*
   * @brief 描画
   */
  void Render(Device* device);

 private:
  class Impl; //!< 内部実装クラス
  std::unique_ptr<Impl> impl_;
};
}  // namespace dxapp
