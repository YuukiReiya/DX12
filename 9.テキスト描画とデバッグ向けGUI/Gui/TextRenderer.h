#pragma once
#include <string>

namespace DX {
class DeviceResources;
};

/*!
 * @brief コンストラクタ
 * @param frameCount バックバッファ数
 */
class TextRenderer {
 public:
  /*!
   * @brief コンストラクタ
   * @param frameCount バックバッファ数
   */
  TextRenderer(std::size_t frameCount);

  /*!
   * @brief デストラクタ
   */
  ~TextRenderer();

  /*!
   * @brief 初期化
   * @param hWnd ウィンドウハンドル
   * @param device デバイスオブジェクト
   */
  void Initialize(HWND hWnd, DX::DeviceResources* deviceResources);
  void Initialize(HWND hWnd, ID3D12Device* device, int backBufferCount,
	  ID3D12CommandQueue* commandQueue);

  /*!
   * @brief ブラシオブジェクトの作成
   * @param name ブラシの名前
   * @param color ブラシカラー
   */
  void CreateColorBrush(const std::string& brushName, D2D1::ColorF::Enum color);

  /*!
   * @brief テキストフォーマットの作成
   * @param name フォーマットの名前
   * @param fontName フォント名
   * @param fontSize フォントサイズ
   * @param fontWeight フォントの太さ
   * @param fontName フォントスタイル
   * @param fontStretch フォントの引き延ばし度合い
   */
  void CreateTextFormat(
      const std::string& formatName, const std::wstring& fontName,
      float fontSize = 20.0f,
      DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL,
      DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH fontStretch = DWRITE_FONT_STRETCH_NORMAL);

  /*!
   * @brief 描画テキストの追加
   * @param text 文字列
   * @param x 描画x座標
   * @param y 描画するy座標
   * @param formatName 使用するフォーマットの名前
   * @param brushName 使用するブラシの名前
   */
  void AddDrawText(const std::wstring& text, float x, float y,
                   const std::string& formatName, const std::string& brushName);

  /*!
   * @brief 描画テキストの追加, 色・フォーマットはデフォルトを使う
   * @param text 文字列
   * @param x 描画x座標
   * @param y 描画するy座標
   */
  void AddDrawText(const std::wstring& text, float x, float y);

  /*!
   * @brief 描画テキストの追加, ブラシはデフォルトを使う
   * @param text 文字列
   * @param x 描画x座標
   * @param y 描画するy座標
   * @param formatName 使用するフォーマットの名前
   */
  void AddDrawTextWithFormat(const std::wstring& text, float x, float y,
                             const std::string& formatName);

  /*!
   * @brief 描画テキストの追加, フォーマットはデフォルトを使う
   * @param text 文字列
   * @param x 描画x座標
   * @param y 描画するy座標
   * @param brushName 使用するブラシの名前
   */
  void AddDrawTextWithBrush(const std::wstring& text, float x, float y,
                            const std::string& brushName);

  /*!
   * @brief テキストレイアウトオブジェクトの作成
   * @param layoutName テキストレイアウトの名前
   * @param text 文字列
   * @param maxWidth レイアウトの幅
   * @param maxHeight レイアウトの高さ
   * @param formatName レイアウトに適用するテキストフォーマット
   * @details TextLayoutは描画データを計算済みの状態で保存する
   *          そのため毎回同じテキストならレイアウトのほうが高速になる
   */
  void CreateTextLayout(const std::string& layoutName, const std::wstring& text,
                        float maxWidth, float maxHeight,
                        const std::string& formatName);

  /*!
   * @brief テキストレイアウトオブジェクトの作成
   *        デフォルトのテキストフォーマットを使う
   * @param layoutName テキストレイアウトの名前
   * @param text 文字列
   * @param maxWidth レイアウトの幅
   * @param maxHeight レイアウトの高さ
   */
  void CreateTextLayoutWithDefaultFormat(const std::string& layoutName,
                                         const std::wstring& text,
                                         float maxWidth, float maxHeight);

  /*!
   * @brief レイアウトテキストの描画リクエスト
   * @param text 文字列
   * @param x 描画x座標
   * @param y 描画するy座標
   * @param formatName 使用するフォーマット名
   * @param brushName 使用するブラシ名
   */
  void AddDrawTextLayoutWithBrush(const std::string& layoutName, float x,
                                  float y, const std::string& brushName);

  /*!
   * @brief レイアウトテキストの描画リクエスト, ブラシはデフォルトを使う
   * @param x 描画x座標
   * @param y 描画するy座標
   */
  void AddDrawTextLayout(const std::string& layoutName, float x, float y);

  /*!
   * @brief 内部のバッファのお掃除。フレームの開始時に呼び出す
   */
  void Clear();

  /*!
   * @brief 描画
   */
  void Render();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
