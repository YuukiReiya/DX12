#pragma once
#include "Singleton.hpp"
#include "DescriptorAllocator.hpp"

namespace dxapp {
class Device;

// ビューとリソースをまとめたもの
// リソース破棄やデスクリプタの破棄対策はしてないぞ
struct TextureView {
  DescriptorHandle handle;
  ID3D12Resource* resource{nullptr};

  bool IsValid() const { return resource != nullptr; }
};


// Deviceクラスが持ってる viewDescriptorHeapを使ってSRVを生成する
class TextureViewManager {
public:
  TextureView CreateView(Device* device, ID3D12Resource* resource);
  void DestroyView(TextureView* view);

  TextureView textureView(ID3D12Resource* resource);

private:
  friend class Singleton<TextureViewManager>;

  TextureViewManager();
  ~TextureViewManager();

  TextureViewManager(const TextureViewManager&) = delete;
  TextureViewManager& operator=(const TextureViewManager&) = delete;
  TextureViewManager(TextureViewManager&&) = delete;
  TextureViewManager& operator=(TextureViewManager&&) = delete;

  class Impl;
  std::unique_ptr<Impl> impl_;  //!< TextureViewManagerの実装
};



class TextureManager {
 public:
  /*!
   * @brief WICフォーマットの画像をテクスチャとしてロード
   */
  bool LoadWICTextureFromFile(Device* device, const std::wstring& fileName,
                              const std::string& assetName);

  /*!
   * @brief テクスチャデータの取得
   */
  Microsoft::WRL::ComPtr<ID3D12Resource> texture(const std::string& assetName);

private:
   friend class Singleton<TextureManager>;
  /*!
   * @brief コンストラクタ
   */
  TextureManager();

  /*!
   * @brief デストラクタ
   */
  ~TextureManager();

  TextureManager(const TextureManager&) = delete;
  TextureManager& operator=(const TextureManager&) = delete;
  TextureManager(TextureManager&&) = delete;
  TextureManager& operator=(TextureManager&&) = delete;


  // これはpImplパターン
  // 詳細な実装は内部クラスImplになげる
  // これにより外部のプログラム対してがっつり隠蔽できる
  class Impl;
  std::unique_ptr<Impl> impl_;  //!< TextureManagerの実装
};
}  // namespace dxapp
