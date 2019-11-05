#pragma once

#include "VertexType.hpp"

namespace dxapp {

class GeometoryMesh {
 public:
  GeometoryMesh(const GeometoryMesh&) = delete;
  GeometoryMesh& operator=(const GeometoryMesh&) = delete;

  /*!
   * @brief デストラクタ
   */
  ~GeometoryMesh();

  /*!
   * @brief 描画コマンド発行
   */
  void Draw(ID3D12GraphicsCommandList* commandList);

  /*!
   * @brief 終了処理
   */
  void Terminate();

  /*!
   * @brief キューブメッシュを生成してGeometoryMeshを返す
   * @param[in] device d3d12デバイス
   * @param[in] size キューブの辺の長さ
   * @param[in] color 頂点カラー
   * @return 生成したGeometoryMeshのunique_ptr
   */
  static std::unique_ptr<GeometoryMesh> CreateCube(ID3D12Device* device,
                                                   float size = 1.0f,
                                                   DirectX::XMFLOAT4 color = {
                                                       1.0f, 1.0f, 1.0f, 1.0f});

  /*!
   * @brief ボックスメッシュを生成してGeometoryMeshを返す
   * @param[in] device d3d12デバイス
   * @param[in] width 横幅
   * @param[in] height 高さ
   * @param[in] depth 奥行き
   * @param[in] color 頂点カラー
   * @return 生成したGeometoryMeshのunique_ptr
   */
  static std::unique_ptr<GeometoryMesh> CreateBox(
      ID3D12Device* device, float width = 1.0f, float height = 1.0f,
      float depth = 1.0f, DirectX::XMFLOAT4 color = {1.0f, 1.0f, 1.0f, 1.0f});

  /*!
   * @brief 球メッシュ生成してGeometoryMeshを返す
   * @param[in] device d3d12デバイス
   * @param[in] radius 球の半径
   * @param[in] sliceCount 横方向の分割数
   * @param[in] stackCount 縦方向の分割数
   * @param[in] color 頂点カラー
   * @return 生成したGeometoryMeshのunique_ptr
   */
  static std::unique_ptr<GeometoryMesh> CreateSphere(
      ID3D12Device* device, float radius = 1.0f, std::uint32_t sliceCount = 16,
      std::uint32_t stackCount = 16,
      DirectX::XMFLOAT4 color = {1.0f, 1.0f, 1.0f, 1.0f});

 private:
  /*!
   * @brief コンストラクタ
   * @details オブジェクト構築はCreate***がする。外部から飛び出し禁止
   */
  GeometoryMesh();

  // pImplパターン
  // 内部実装を隠ぺいする
  class Impl;                   // 内部クラスを宣言
  std::unique_ptr<Impl> impl_;  // こいつがGeometoryMeshの実態になる。
};
}  // namespace dxapp
