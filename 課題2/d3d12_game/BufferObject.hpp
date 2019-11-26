#pragma once
namespace dxapp {
/*!
 * @brief バッファオブジェクトの種類。作成方法の切り替えにつかう
 */
enum class BufferObjectType {
  VertexBuffer = 0,  //!< 頂点バッファ
  IndexBuffer,       //!< インデックスバッファ
  ConstantBuffer,    //!< 定数バッファ
  Max
};

/*!
 * @brief GPUに送るバッファオブジェクトの作成とデータ更新をする
 */
class BufferObject {
 public:
  BufferObject(const BufferObject&) = delete;
  BufferObject& operator=(const BufferObject&) = delete;

  /*!
   * @brief デフォルトコンストラクタ
   */
  BufferObject() = default;

  /*!
   * @brief デストラクタ
   */
  ~BufferObject() { Terminate(); }

  /*!
   * @brief BufferObjectTypeとサイズでバッファを取得
   * @prame[in] device デバイス
   * @prame[in] type BufferObjectTypeを指定
   * @prame[in] size バッファーサイズ(sizeofした値)
   */
  bool Initialize(ID3D12Device* device, const BufferObjectType type,
                  std::size_t size);
  /*!
   * @brief 終了処理
   */
  void Terminate();

  /*!
   * @brief バッファをマップする
   * @return マップしたアドレス
   */
  void* Map();

  /*!
   * @brief バッファをアンマップ
   */
  void Unmap();

  /*!
   * @brief バッファにデータを送る
   * @prame[in] data 書き込むデータのアドレス
   * @prame[in] size データのサイズ
   * @prame[in] offset 書き込むアドレスのオフセット
   */
  void Update(const void* data, std::size_t size, std::size_t offset = 0);

  /*!
   * @brief 実際に確保したバッファサイズ
   */
  std::size_t bufferSize() const { return bufferSize_; }

  /*!
   * @brief 作成したバッファのリソース
   */
  ID3D12Resource1* resource() { return resource_.Get(); }

 private:
  //! ヒーププロパティ
  CD3DX12_HEAP_PROPERTIES heapProp_;
  //! バッファのデスクリプタ
  CD3DX12_RESOURCE_DESC resourceDesc_;
  //! 確保したバッファのサイズ
  std::size_t bufferSize_;
  //! バッファのリソース
  Microsoft::WRL::ComPtr<ID3D12Resource1> resource_;
};
}  // namespace dxapp
