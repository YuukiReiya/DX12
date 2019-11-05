#pragma once
// 今回はとりあえず1シェーダに対応する専用クラスとしてつくります。

namespace dxapp {
/*!
 * @brief BasicShaderで使用する定数バッファ
 */
struct BasicShaderCB {
  // XMFLOAT4X4とシェーダのfloat4x4は同じ形になる
  // Transformデータを行列(マトリクス)に変換して格納
  DirectX::XMFLOAT4X4 world{};
  DirectX::XMFLOAT4X4 wvp{};
};

/*!
 * @brief BasicShaderでのリソース、デスクリプタの開始インデクス
 */
enum class BasicShaderResourceIndex { Constant = 0, Srv = 1, Size = 2 };
enum class BasicNonTexShaderResourceIndex {
  Constant = 0,
  Size = 1
};


/*!
 * @brief BasicVS/BasicPSに対応するクラス
 */
class BasicShader {
 public:
  /*!
   * @brief コンストラクタ
   */
  BasicShader();

  /*!
   * @brief デストラクタ
   */
  ~BasicShader();

  /*!
   * @brief 初期化
   */
  void Initialize(ID3D12Device* device);

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
   * @brief 参照する定数バッファのデスクリプタ
   * @param[in] heaps デスクリプタヒープ
   * @param[in] offset ヒープ内でのオフセット位置
   */
  void SetCBufferDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset);

  /*!
   * @brief 参照するSRVのデスクリプタ
   * @param[in] heaps デスクリプタヒープ
   * @param[in] offset ヒープ内でのオフセット位置
   */
  void SetSrvDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset);

  /*!
   * @brief 参照するサンプラのデスクリプタ
   * @param[in] heaps デスクリプタヒープ
   * @param[in] offset ヒープ内でのオフセット位置
   */
  void SetSamplerDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

class BasicNonTexShader {
 public:
  /*!
   * @brief コンストラクタ
   */
  BasicNonTexShader();

  /*!
   * @brief デストラクタ
   */
  ~BasicNonTexShader();

  /*!
   * @brief 初期化
   */
  void Initialize(ID3D12Device* device);

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
   * @brief 参照する定数バッファのデスクリプタ
   * @param[in] heaps デスクリプタヒープ
   * @param[in] offset ヒープ内でのオフセット位置
   */
  void SetCBufferDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace dxapp
