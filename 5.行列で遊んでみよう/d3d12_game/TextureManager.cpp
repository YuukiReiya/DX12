#include "TextureManager.hpp"
#include "Device.hpp"

// テクスチャの読み込みにはMicrosoftさんが配布されているコードを使いますね
#include "External/WICTextureLoader12.h"
#include "External/DDSTextureLoader12.h"

namespace dxapp {

/*!
 * @brief TextureManagerの実装
 */
class TextureManager::Impl {
  // 内部クラスは、クラス名::内部クラス名で定義できます。
 public:
  /*!
   * @brief コンストラクタ
   */
  Impl() = default;
  /*!
   * @brief デストラクタ
   */
  ~Impl() = default;

  /*!
   * @brief ID3D12Resourceの取得
   * @return アセットが存在すればID3D12Resource*を、なければnullptrを返す
   */
  Microsoft::WRL::ComPtr<ID3D12Resource> resource(
      const std::string& assetName) {
    const auto end = std::end(textures_);  // 連想配列の終わりを取得(つまり)
    const auto ret = textures_.find(assetName);  // アセットで探して...
    // 連想配列にデータがないと、std::end(textures_)が返ってくる
    if (ret == end) {
      return nullptr;  // なかった
    }
    return ret->second->resource;  // 見つかった
  }

  //! テクスチャ管理用の構造体
  struct Texture {
    std::wstring fileName;                            //!< ファイル名
    std::string assetName;                            //<! アセット名
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;  //!< テクスチャの実態
  };
  // テクスチャもロードすると頂点バッファなどと同じにID3D12Resourceになる
  // リソースはGPUからはメモリの塊にしか見えないってことだな

  //! テクスチャを格納しておくコンテナ
  std::unordered_map<std::string, std::unique_ptr<Texture>> textures_{};
  // unordered_mapはいわゆる連想配列
  // unordered_map<キーの型, 格納したい型>になる。
  // キーの値は重複できないので、同じテクスチャを二回以上ロードしない仕組みに使える
};

//-------------------------------------------------------------------
// TextureManager
//-------------------------------------------------------------------
TextureManager::TextureManager() : impl_(new Impl()) {}

TextureManager::~TextureManager() {}

bool TextureManager::LoadWICTextureFromFile(Device* device,
                                            const std::wstring& fileName,
                                            const std::string& assetName) {
  // テクスチャの存在チェック
  if (impl_->resource(assetName)) {
    return true;
  }

  // ロード時に必要な変数を確保
  ID3D12Resource* resource;
  std::unique_ptr<uint8_t[]> decodedData{};
  D3D12_SUBRESOURCE_DATA subresource{};

  // LoadWICTextureFromFileでWICをつかってbmp,png,jpgとかが読める
  // とりあえずファイルからメモリに読み込み
  auto hr = DirectX::LoadWICTextureFromFile(
      device->device(),  // デバイス
      fileName.c_str(),  // ファイル名
      &resource,     // D3Dで使用可能になったテクスチャデータ
      decodedData,   // ファイルから読み込まれたバイトデータ
      subresource);  // テクスチャデータのアドレスとデータの並びが返ってくる

  if (FAILED(hr)) {
    return false;
  }

  // VRAMに送るサイズを取得
  const auto uploadBufferSize = GetRequiredIntermediateSize(resource, 0, 1);

  auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto desc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
  Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeap;

  // アップロード先のメモリを確保
  device->device()->CreateCommittedResource(
      &prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr, IID_PPV_ARGS(uploadHeap.GetAddressOf()));

  // アップロードコマンドの生成
  // テクスチャの転送はコマンドリストを経由する必要がある
  auto cl = device->CreateNewGraphicsCommandList();
  {
    // VRAMへの転送コマンド発行
    // d3dx12のヘルパー関数を利用します。便利便利
    UpdateSubresources(
        cl.Get(),
        resource,          // テクスチャリソース
        uploadHeap.Get(),  // 送り先
        0, 0, 1,           // とりあえずこの設定でOK
        &subresource);  // ロード時に得たD3D12_SUBRESOURCE_DATAを設定

    // 転送可能からピクセルシェーダになるまで待つ
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource, D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cl->ResourceBarrier(1, &barrier);
  }

  // 転送開始
  // 当然だが1ファイルずつ実行するより、まとまった単位でExexしたほうがいいよ
  // いまは簡単のために1個ずつやっています。
  {
    cl->Close();
    ID3D12CommandList* lists[]{cl.Get()};
    device->commandQueue()->ExecuteCommandLists(1, lists);

    // コマンドリストで転送するのでGPUの処理待ちをします
    {
      // Device::WaitForGPUはDeviceクラスためのものなので自前で作る
      Microsoft::WRL::ComPtr<ID3D12Fence> fence;
      UINT64 value = 0;
      device->device()->CreateFence(
          value++, D3D12_FENCE_FLAG_NONE,
          IID_PPV_ARGS(fence.ReleaseAndGetAddressOf()));

      Microsoft::WRL::Wrappers::Event fenceEvent{};
      fenceEvent.Attach(
          CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
      if (SUCCEEDED(device->commandQueue()->Signal(fence.Get(), value))) {
        if (SUCCEEDED(fence->SetEventOnCompletion(value, fenceEvent.Get()))) {
          WaitForSingleObjectEx(fenceEvent.Get(), INFINITE, FALSE);
        }
      }
    }

    // 終わったのでデータを格納
    auto tex = std::make_unique<Impl::Texture>();
    tex->fileName = fileName;
    tex->assetName = assetName;
    tex->resource.Attach(resource);
    impl_->textures_.emplace(assetName, std::move(tex));
  }

  // ちなみにテクスチャの削除は用意してない
  return true;
}

Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::texture(
    const std::string& assetName) {
  return impl_->resource(assetName);
}
}  // namespace dxapp
