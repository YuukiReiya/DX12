#pragma once
namespace dxapp {
class Device;

/*!
 * @brief シャドウマップを表示するだけのシェーダー
 */

class ShadowMapDebugShader {
 public:
  /*!
   * @brief コンストラクタ
   */
  ShadowMapDebugShader();

  /*!
   * @brief デストラクタ
   */
  ~ShadowMapDebugShader();

  /*!
   * @brief 初期化
   */
  void Initialize(Device* device);

  /*!
   * @brief 終了処理
   */
  void Terminate();

  /*!
   * @brief フレームでシェーダを使用するときに最初に呼び出す関数
   * @details Endを呼ぶまでは引数のコマンドリストを使う
   * @param[in,ont] commandList コマンドを積むためのコマンドリスト
   */
  void Begin(ID3D12GraphicsCommandList* commandList);

  /*!
   * @brief フレームでシェーダを使い追わったら必ず呼ぶ関数
   */
  void End();

  /*!
   * @brief シェーダが持っているパラメータでコマンドを発行
   */
  void Apply();

  /*!
   * @brief デスクリプタテーブルが参照するヒープをセット
   */
  void SetDescriptorHeap(ID3D12DescriptorHeap* srv, ID3D12DescriptorHeap* sampler);

  /*!
   * @brief シャドウマップ設定
   */
  void SetShadowMap(D3D12_GPU_DESCRIPTOR_HANDLE handle);

  /*!
   * @brief サンプラ設定
   */
  void SetSampler(D3D12_GPU_DESCRIPTOR_HANDLE handle);

 private:
  //! 内部実装クラス
  class Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace dxapp