#pragma once
namespace dxapp {

class BufferObject;
class Device;

/*!
 * @brief モデルの質感を表すデータ
 */
class Material {
 public:
  struct MaterialParameter {
    DirectX::XMFLOAT4X4 matTrans;
    DirectX::XMFLOAT4 diffuseAlbedo{1, 1, 1, 1};
    DirectX::XMFLOAT3 fresnel{0.5f, 0.5f, 0.5f};
    float roughness{0.1f};
    int useTexture = 0;
  };

  /*!
   * @brief コンストラクタ
   */
  Material() = default;
  Material(const MaterialParameter& param);

  /*!
   * @brief 初期化
   */
  void Initialize(dxapp::Device* device);

  /*!
   * @brief 定数バッファ更新
   */
  void Update(std::uint32_t index);

  /*!
   * @brief 拡散反射光の設定
   */
  void SetDiffuseAlbedo(DirectX::XMFLOAT4 diffuseAlbedo);

  /*!
   * @brief 鏡面反射光の設定
   */
  void SetFresnel(DirectX::XMFLOAT3 fresnel);

  /*!
   * @brief 面の粗さ設定
   */
  void SetRoughness(float roughness);

  /*!
   * @brief テクスチャの設定
   */
  void SetTexture(ID3D12DescriptorHeap* heap, std::uint32_t offset);

  /*!
   * @brief テクスチャの設定
   */
  void SetMatrix(DirectX::XMMATRIX m);

  /*!
   * @brief テクスチャの解除
   */
  void ClearTexture();

  /*!
   * @brief テクスチャのヒープとオフセット位置を取得
   */
  void textureDescHeap(ID3D12DescriptorHeap** heap, std::uint32_t* offset);

  /*!
   * @brief テクスチャの割り当て確認
   */
  bool HasTexture() const;

  /*!
   * @brief このマテリアルの定数バッファを取得
   */
  D3D12_GPU_VIRTUAL_ADDRESS materialCb(std::uint32_t index) const;

 private:
  MaterialParameter material_;  //!< 定数バッファに書き込む値
  std::vector<std::unique_ptr<dxapp::BufferObject>>
      matCb_{};  //!< MaterialParameterの定数バッファ領域
  ID3D12DescriptorHeap* srvHeap_{nullptr};  //!< SRVデスクリプタヒープ
  std::uint32_t srvOffset_{0};              //! アドレスオフセット
};
}  // namespace dxapp
