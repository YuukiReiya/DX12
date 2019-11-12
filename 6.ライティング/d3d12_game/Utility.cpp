#include "Utility.hpp"

namespace dxapp {
namespace utility {

// using namespaceしたもの、宣言以降は namespaceを省略できるぞ
// でも宣言以降すべてで適用されるのでヘッダでは絶対にしないこと。
using namespace Microsoft::WRL;

HRESULT CompileShaderFromFile(const std::wstring& fileName,
                              const std::wstring& profile,
                              Microsoft::WRL::ComPtr<ID3DBlob>& shaderBlob,
                              Microsoft::WRL::ComPtr<ID3DBlob>& errorBlob) {
  std::filesystem::path filePath(fileName);
  std::ifstream infile(filePath);
  if (!infile) {
    throw std::runtime_error("shader not found");
  }

  std::vector<char> shaderSrc;
  // ファイルサイズに合わせて、動的配列のサイズをなおす
  shaderSrc.resize(uint32_t(infile.seekg(0, infile.end).tellg()));
  infile.seekg(0, infile.beg)
      .read(shaderSrc.data(), shaderSrc.size());  // ファイルの読み込み

  //! バイトデータの塊をシェーダに変換するオブジェクト
  ComPtr<IDxcLibrary> library;
  DxcCreateInstance(CLSID_DxcLibrary,
                    IID_PPV_ARGS(&library));  // IDxcLibrary作成

  //! バイトデータから返還された、シェーダデータを受け取るオブジェクト
  ComPtr<IDxcBlobEncoding> source;
  // バイトデータからシェーダデータに変換
  library->CreateBlobWithEncodingFromPinned(
      shaderSrc.data(), UINT(shaderSrc.size()), CP_ACP, &source);

  // コンパイルフラグ
  LPCWSTR compilerFlags[] {
#if _DEBUG
    L"/Zi", L"/O0",
#else
    L"/O2"  // リリースビルドは最適化
#endif
  };

  //! 実行可能な状態にするコンパイラ
  ComPtr<IDxcCompiler> compiler;
  DxcCreateInstance(CLSID_DxcCompiler,
                    IID_PPV_ARGS(&compiler));  // コンパイラ作成

  //! シェーダーでのincludeを解決しているらしい
  ComPtr<IDxcIncludeHandler> includeHandler;
  library->CreateIncludeHandler(includeHandler.ReleaseAndGetAddressOf());

  //! コンパイルの結果を受け取るオブジェクト
  ComPtr<IDxcOperationResult> dxcResult;
  // シェーダデータを実行可能な状態にコンパイル
  compiler->Compile(source.Get(),                // シェーダーデータ
                    filePath.wstring().c_str(),  // ファイルパス
                    L"main",  // コンパイル対象のエントリーポイント
                    profile.c_str(),  // シェーダのプロファイル
                    compilerFlags,    // コンパイルフラグ
                    _countof(compilerFlags),  // フラグの要素数
                    nullptr,                  // 基本的にはnullでOK
                    0,                        // 0でOK
                    includeHandler.Get(),     // IDxcIncludeHandler
                    &dxcResult);              // IDxcOperationResult

  HRESULT hr{S_FALSE};
  dxcResult->GetStatus(&hr);
  if (SUCCEEDED(hr)) {
    // コンパイルが成功
    // shaderBlobにコンパイル後のシェーダデータをいれる
    dxcResult->GetResult(
        reinterpret_cast<IDxcBlob**>(shaderBlob.GetAddressOf()));
  } else {
    // コンパイルが失敗
    // errorBlobにエラーを渡す
    dxcResult->GetErrorBuffer(
        reinterpret_cast<IDxcBlobEncoding**>(errorBlob.GetAddressOf()));
  }
  return hr;
}

}  // namespace utility
}  // namespace dxapp
