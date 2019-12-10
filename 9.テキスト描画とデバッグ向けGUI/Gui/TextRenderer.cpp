#include "pch.h"
#include "DeviceResources.h"

#include <array>
#include <unordered_map>

#include "TextRenderer.h"

#pragma warning(once : 26812)

using namespace DX;

template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma region Impl
/*!
 * @brief TextRendererの実装
 */
class TextRenderer::Impl {
 public:
   Impl(std::size_t frameCount);
  ~Impl(){}

  void Initialize(HWND hWnd, DeviceResources* device);
  void CreateD3D11Device();
  void CreateDWriteObject();
  void CreateRenderResource(HWND hWnd);

  void CreateColorBrush(const std::string& brushName, D2D1::ColorF::Enum color);

  void CreateTextFormat(
    const std::string& formatName, const std::wstring& fontName,
    float fontSize = 20.0f,
    DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL,
    DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL,
    DWRITE_FONT_STRETCH fontStretch = DWRITE_FONT_STRETCH_NORMAL);
  
  void CreateTextLayout(const std::string& layoutName, const std::wstring& text,
    float maxWidth, float maxHeight,
    const std::string& formatName);
  
  void AddDrawText(const std::wstring& text, float x, float y,
    const std::string& formatName, const std::string& brushName);

  void AddDrawTextLayout(const std::string& layoutName, float x,
    float y, const std::string& brushName);

  void Clear();
  void Render();

  //-------------

  //! D3D12オブジェクトへのキャッシュ
  DeviceResources* devResource_{};

  // D3D11 / D2D / DirectWriteオブジェクト
  ComPtr<ID3D11DeviceContext> d3d11DeviceContext_;  //!< D3D11の描画命令を発行する
  ComPtr<ID3D11On12Device> d3d11On12Device_;        //!< D3D12と11の橋渡しをするデバイス
  ComPtr<ID2D1Factory3> d2dFactory_;                //!< D2Dファクトリー
  ComPtr<ID2D1Device2> d2dDevice_;                  //!< D2Dデバイス
  ComPtr<ID2D1DeviceContext2> d2dDeviceContext_;    //!< D2D描画命令を発行する
  ComPtr<IDWriteFactory> dWriteFactory_;            //!< DW系のオブジェクトファクトリー

  // レンダーターゲット
  std::vector<ComPtr<ID3D11Resource>> wrappedBackBuffers_;  //! D3D11のレンダーターゲット
  std::vector<ComPtr<ID2D1Bitmap1>> d2dRenderTargets_;      //! D2Dで扱えるレンダーターゲット

  //! 扱う文字数の最大値（値に意味はないです、もっと大きくてもよい）
  static constexpr std::uint32_t ArraySize_ = 2048;

  // 描画するテキストはこのバッファに隙間なく詰め込まれる
  // んでTextData構造体にはテキスト先頭のポインタを登録する
  // なまのstd::stringを保存するとstringのメモリ確保・解放で激しすぎて
  // メモリが荒れちゃいそうなので・・・
  std::array<wchar_t, ArraySize_> stringsBuffer_{}; //! 描画する文字列データ

  //! 1フレーム中に登録した文字数
  std::uint32_t characterCount_ = 0;

  //! DrawTextで描画するテキストの情報を集めた構造体
  struct TextData {
    wchar_t* string{nullptr};            //!< strings_へのポインタ
    std::uint32_t length{};              //!< 描画する文字列長
    float x{};                           //!< 描画 x座標
    float y{};                           //!< 描画 y座標
    IDWriteTextFormat* format{nullptr};  //!< 使用するフォーマットのポインタ
    ID2D1SolidColorBrush* brush{nullptr};  //!< 使用するブラシのポインタ
  };

  //! DrawTextデータ配列
  std::array<TextData, ArraySize_> textData_{};
  //! AddDrawTextされた回数
  int textDrawCount_{};

  struct LayoutTextData {
    D2D1_POINT_2F point{};                 //!< 描画座標
    IDWriteTextLayout* layout{ nullptr };  //!< 使用するレイアウトポインタ
    ID2D1SolidColorBrush* brush{ nullptr };  //!< 使用するブラシのポインタ
  };

  //! DrawTextLayoutデータ配列
  std::array<LayoutTextData, ArraySize_> layoutTextData_{};
  //! AddDrawTextLayoutされた回数
  int textDrawLayoutCount_{};

   //! フォーマットの連想配列
   std::unordered_map<std::string, ComPtr<IDWriteTextFormat>> formats_{};

   //! レイアウトの連想配列
   std::unordered_map<std::string, ComPtr<IDWriteTextLayout>> layouts_{};

   //! ブラシの連想配列
   std::unordered_map<std::string, ComPtr<ID2D1SolidColorBrush>> brushs_{};
};

/*!
 * @brief コンストラクタ
 * @param frameCount バックバッファ数
 */
TextRenderer::Impl::Impl(std::size_t frameCount)
  : wrappedBackBuffers_(frameCount)
  , d2dRenderTargets_(frameCount)
{}

/*!
 * @brief 初期化
 * @param hWnd ウィンドウハンドル
 * @param device デバイスオブジェクト
 */
void TextRenderer::Impl::Initialize(HWND hWnd, DeviceResources* device) {
  devResource_ = device;

  CreateD3D11Device();
  CreateDWriteObject();
  CreateRenderResource(hWnd);
}

/*!
 * @brief D3D11デバイス作成
 */
void TextRenderer::Impl::CreateD3D11Device() {
  // わざわざD3D11デバイスを作るのは Direct2DがD3D11上に構築されているから

  // このコマンドキューを11on12デバイスに渡す
  // 11on12は内部的にはD3D12で描画してる
  ID3D12CommandQueue* commandQueue[] = { devResource_->GetCommandQueue() };

  // D3D11On12CreateDeviceはD3D12デバイスでラップしたD3D11が作成される
  // D3D11の描画命令を、ラップしたD3D12に流しD3D12のコマンドキューを使って
  // 上手いこと描画してくれるようになってる
  ComPtr<ID3D11Device> d3d11Device;
  D3D11On12CreateDevice(devResource_->GetD3DDevice(),
                        D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0,
                        reinterpret_cast<IUnknown**>(commandQueue), 1, 0,
                        &d3d11Device, &d3d11DeviceContext_, nullptr);
  d3d11Device.As(&d3d11On12Device_);
}

/*!
 * @brief DirectWriteオブジェクト作成
 */
void TextRenderer::Impl::CreateDWriteObject() {
    // まずはD2Dデバイスコンテキストを作るためのファクトリーを作成
    D2D1_FACTORY_OPTIONS d2dFactoryOptions = {};
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
      __uuidof(ID2D1Factory3), &d2dFactoryOptions,
      &d2dFactory_);

    // D2Dデバイス作成
    ComPtr<IDXGIDevice> dxgiDevice;
    d3d11On12Device_.As(&dxgiDevice);
    d2dFactory_->CreateDevice(dxgiDevice.Get(), &d2dDevice_);

    // コンテキストはD3D12でいうとキューとかコマンドリスト周りの処理を担当してくれる
    d2dDevice_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                                    &d2dDeviceContext_);

    // 最後にDirectWriteを扱うためのファクトリーを作成
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
      &dWriteFactory_);
}

/*!
 * @brief 描画に必要なレンダーターゲット系のリソース作成
 */
void TextRenderer::Impl::CreateRenderResource(HWND hWnd) {
  // ウィンドウのDPI(Dot Per Inch)を取得
  // 最近のWindowsだとモニタごとにDPIが違うのでウィンドウ単位で調べるんだな
  auto dpi = static_cast<float>(GetDpiForWindow(hWnd));

  // ビットマップサーフェイス設定
  D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
    D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
    D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
    dpi,
    dpi
  );

  // DirectWriteが使うレンダーターゲットを作成する
  auto backBufferCount = devResource_->GetBackBufferCount();
  for (UINT i = 0; i < backBufferCount; i++) {

    // D3D12のレンダーターゲットを11on12でも使えるようにします
    D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };

    // CreateWrappedResourceは 11on12デバイスが使えるリソースを作るよ
    d3d11On12Device_->CreateWrappedResource(
        devResource_->GetRenderTarget(i),   // このレンダーターゲットを
        &d3d11Flags,  // 11のレンダーターゲットとしても使えるようにする
        D3D12_RESOURCE_STATE_RENDER_TARGET,    // 初期ステート
        D3D12_RESOURCE_STATE_PRESENT,          // 遷移するステート
        IID_PPV_ARGS(&wrappedBackBuffers_[i])  // 受け取る人
    );

    // surfaceはマイクロソフトの出してるPC。ではなくて
    // 表面とか面の意味。転じて描画面の意味
    ComPtr<IDXGISurface> surface;
    wrappedBackBuffers_[i].As(&surface);

    // IDXGISurfaceを経由してD2Dのレンダーターゲットにする
    d2dDeviceContext_->CreateBitmapFromDxgiSurface(
      surface.Get(),
      &bitmapProperties,
      &d2dRenderTargets_[i]
    );
  }
}

/*!
 * @brief ブラシ作成
 */
void TextRenderer::Impl::CreateColorBrush(const std::string& brushName,
                                          D2D1::ColorF::Enum color) {
  // 作成済みならなにもしない
  if (brushs_.find(brushName) != brushs_.end()) {
    return;
  }

  // D2Dで色を指定するときはBrush（ブラシ・はけ）オブジェクトをつかう
  ComPtr<ID2D1SolidColorBrush> brush;
  d2dDeviceContext_->CreateSolidColorBrush(D2D1::ColorF(color), &brush);
  brushs_.emplace(brushName, brush);
}

/*!
 * @brief テキストフォーマット作成
 */
void TextRenderer::Impl::CreateTextFormat(const std::string& name,
                                          const std::wstring& fontName,
                                          float fontSize,
                                          DWRITE_FONT_WEIGHT fontWeight,
                                          DWRITE_FONT_STYLE fontStyle,
                                          DWRITE_FONT_STRETCH fontStretch) {
  // 作成済みならなにもしない
  if (formats_.find(name) != formats_.end()) {
    return;
  }

  // IDWriteTextFormatはフォント、フォントサイズなどの書式設定オブジェクト
  ComPtr<IDWriteTextFormat> format;
  dWriteFactory_->CreateTextFormat(fontName.c_str(), NULL, fontWeight,
                                   fontStyle, fontStretch, fontSize, L"ja-jp",
                                   &format);
  formats_.emplace(name, format);
}

/*!
 * @brief テキストレイアウト作成
 */
void TextRenderer::Impl::CreateTextLayout(const std::string& layoutName,
                                          const std::wstring& text,
                                          float maxWidth, float maxHeight,
                                          const std::string& formatName) {
  if (layouts_.find(layoutName) != layouts_.end()) {
    return;
  }

  // フォーマットがなかったら例外なげてもらおう
  auto format = formats_.at(formatName);

  // IDWriteTextLayoutがレイアウトオブジェクト
  // フォーマットも覚える
  // またテキストに対して下線や打消し線も入れられます
  ComPtr<IDWriteTextLayout> layout;
  dWriteFactory_->CreateTextLayout(
      text.c_str(),                        // 文字列のポインタ
      static_cast<UINT32>(text.length()),  // 文字列の長さ
      format.Get(),                        // フォーマット
      maxWidth, maxHeight,                 // レイアウトの矩形サイズ
      &layout);

  layouts_.emplace(layoutName, layout);
}

/*!
 * @brief DrawTextデータ構築
 */
void TextRenderer::Impl::AddDrawText(const std::wstring& text, float x, float y,
                                     const std::string& formatName,
                                     const std::string& brushName) {
  auto length = static_cast<std::uint32_t>(text.length());
  // 追加したときの文字数がバッファを超えてたらとりあえず死んでおく
  assert(characterCount_ + length < ArraySize_);

  // 文字列のコピー開始位置のポインタ貰う
  auto head = &stringsBuffer_[characterCount_];

  // TextDataを構築して...
  Impl::TextData data{
      head,
      length,
      x,
      y,
      formats_[formatName].Get(),  // フォーマットの生ポインタ
      brushs_[brushName].Get(),    // ブラシの生ポインタ
  };
  // TextData追加
  textData_[textDrawCount_] = data;

  // 文字列データをバッファにコピー
  text.copy(head, length);

  // 文字数とTextDataの数を更新
  characterCount_ += length;
  textDrawCount_++;
}

/*!
 * @brief DrawTextLayoutデータ構築
 */
void TextRenderer::Impl::AddDrawTextLayout(const std::string& layoutName,
                                           float x, float y,
                                           const std::string& brushName) {
  // LayoutTextDataを構築して...
  LayoutTextData data{
      {x, y},
      layouts_[layoutName].Get(),  // レイアウトの生ポインタ
      brushs_[brushName].Get(),    // ブラシの生ポインタ
  };
  // LayoutTextData追加
  layoutTextData_[textDrawLayoutCount_] = data;

  // 文字数とTextDataの数を更新
  textDrawLayoutCount_++;
}

/*!
 * @brief 内部データをクリア
 */
void TextRenderer::Impl::Clear() {
  // 今回はさぼっているが、テキストフォーマット、ブラシの削除はこのあたりで
  // まとめて実行したほうが親切だろう
  // 削除関数などを呼び出すごとに実行すると、削除したフレームではまだ使っていた！
  // という状況もあるので削除したいデータを覚えておいて次のフレームの先頭で消すのがよい
  characterCount_ = 0;
  textDrawCount_ = 0;
  textDrawLayoutCount_ = 0;
}

/*!
 * @brief 構築したデータで描画
 */
void TextRenderer::Impl::Render() {
  const auto frameIndex = devResource_->GetCurrentFrameIndex();

  // バックバッファを取得
  auto wrappedBackBuffer = wrappedBackBuffers_[frameIndex];
  d3d11On12Device_->AcquireWrappedResources(wrappedBackBuffer.GetAddressOf(),
                                            1);
  // レンダーターゲットセット
  d2dDeviceContext_->SetTarget(d2dRenderTargets_[frameIndex].Get());

  // 描画開始
  d2dDeviceContext_->BeginDraw();
  {
    d2dDeviceContext_->SetTransform(D2D1::Matrix3x2F::Identity());

    // レンダーターゲットのサイズを取得
    D2D1_SIZE_F rtSize = d2dRenderTargets_[frameIndex]->GetSize();
    D2D1_RECT_F rect = D2D1::RectF(0, 0, rtSize.width, rtSize.height);

    // DrawText
    for (int i = 0; i < textDrawCount_; i++) {
      auto& data = textData_[i];

      // 描画エリア設定。左上座標だけ書き換えて使う
      // rectの中に納まるように描画するので幅が足りなりと折り返される
      rect.left = data.x;
      rect.top = data.y;
      // 描画
      d2dDeviceContext_->DrawText(data.string, data.length, data.format, &rect,
                                  data.brush);
    }

    // DrawTextLayout
    for (int i = 0; i < textDrawLayoutCount_; i++) {
      auto& data = layoutTextData_[i];
      // 描画
      d2dDeviceContext_->DrawTextLayout(data.point, data.layout, data.brush);
    }
  }

  d2dDeviceContext_->EndDraw();
  // バックバッファを解放する。リソースの状態遷移が行われる
  d3d11On12Device_->ReleaseWrappedResources(wrappedBackBuffer.GetAddressOf(),
                                            1);
  // D3D11の描画コマンドを発行する
  d3d11DeviceContext_->Flush();
}
#pragma endregion

#pragma region TextRenderer
TextRenderer::TextRenderer(std::size_t frameCount)
    : impl_(new Impl(frameCount)){};

TextRenderer::~TextRenderer(){};

void TextRenderer::Initialize(HWND hWnd, DX::DeviceResources* deviceResources) {
  impl_->Initialize(hWnd, deviceResources);
  // デフォルトで使用されるブラシとフォーマットを作っておく
  CreateColorBrush("default", D2D1::ColorF::Black);
  CreateTextFormat("default", L"メイリオ");
}

void TextRenderer::CreateColorBrush(const std::string& brushName, D2D1::ColorF::Enum color) {
  impl_->CreateColorBrush(brushName, color);
}

void TextRenderer::CreateTextFormat(const std::string& formatName,
                                    const std::wstring& fontName,
                                    float fontSize,
                                    DWRITE_FONT_WEIGHT fontWeight,
                                    DWRITE_FONT_STYLE fontStyle,
                                    DWRITE_FONT_STRETCH fontStretch) {
  impl_->CreateTextFormat(formatName, fontName, fontSize, fontWeight, fontStyle, fontStretch);
}

void TextRenderer::AddDrawText(const std::wstring& text, float x, float y,
                               const std::string& formatName,
                               const std::string& brushName) {
  impl_->AddDrawText(text, x, y, formatName, brushName);
}

void TextRenderer::AddDrawText(const std::wstring& text, float x, float y) {
  impl_->AddDrawText(text, x, y, "default", "default");
}

void TextRenderer::AddDrawTextWithBrush(const std::wstring& text, float x,
                                        float y, const std::string& brushName) {
  impl_->AddDrawText(text, x, y, "default", brushName);
}

void TextRenderer::AddDrawTextWithFormat(const std::wstring& text, float x,
                                         float y,
                                         const std::string& formatName) {
  impl_->AddDrawText(text, x, y, formatName, "default");
}

void TextRenderer::CreateTextLayout(const std::string& layoutName,
                                    const std::wstring& text, float maxWidth,
                                    float maxHeight,
                                    const std::string& formatName) {
  impl_->CreateTextLayout(layoutName, text, maxWidth, maxHeight, formatName);
}

void TextRenderer::CreateTextLayoutWithDefaultFormat(
    const std::string& layoutName, const std::wstring& text, float maxWidth,
    float maxHeight) {
  impl_->CreateTextLayout(layoutName, text, maxWidth, maxHeight, "default");
}

void TextRenderer::AddDrawTextLayout(const std::string& layoutName, float x,
                                     float y) {
  impl_->AddDrawTextLayout(layoutName, x, y, "default");
}

void TextRenderer::AddDrawTextLayoutWithBrush(const std::string& layoutName,
                                              float x, float y,
                                              const std::string& brushName) {
  impl_->AddDrawTextLayout(layoutName, x, y, brushName);
}

void TextRenderer::Clear() { impl_->Clear(); }

void TextRenderer::Render() { impl_->Render(); }
#pragma endregion
