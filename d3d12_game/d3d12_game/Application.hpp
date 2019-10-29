#pragma once
#include "External/StepTimer.h"

namespace dxapp {

// 前方宣言
class Device;
class SimplePolygon;

/*!
 * @brief ゲームアプリケーションクラス
 */
class Application {
 public:
  /*!
   * @brief デフォルトコンストラクタ
   */
  Application();

  /*!
   * @brief デストラクタ
   */
  virtual ~Application();

  // コピー、代入はさせない
  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  /*!
   * @brief 初期化
   * @param[in] hWnd ウィンドウハンドル
   * @param[in] screenWidth 描画スクリーンの幅
   * @param[in] screenHeight 描画スクリーンの高さ
   */
  void Initialize(HWND hWnd, std::uint32_t screenWidth,
                  std::uint32_t screenHeight);

  /*!
   * @brief ゲームの実行
   */
  void Run();

  /*!
   * @brief 終了処理
   */
  void Terminate();

 private:
  /*!
   * @brief ゲームの更新
   * @param[in] timer タイマー
   */
  void Update(const DX::StepTimer& timer);

  /*!
   * @brief ゲームの描画
   */
  void Render();

  /*!
   * @brief レンダーターゲットをクリア
   */
  void ClearRenderTarget();

  //! ウィンドウハンドル
  HWND hWnd_{nullptr};

  //! デバイスオブジェクト
  std::unique_ptr<Device> device_;

  //! タイマー
  DX::StepTimer timer_;
};
}  // namespace dxapp
