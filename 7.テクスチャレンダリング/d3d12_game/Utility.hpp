#pragma once
// namespaceはすごく大雑把に説明するとフォルダみたいなもの
// 1つのフォルダに同じファイル名は1つしか存在できないがフォルダが違えばOKですね？
// プログラムでも同じ名前の関数や型は許されません（名前の衝突とかいう）
// そこでnamespaceです。namespaceの中にコードをくくることで名前の衝突を回避できます

// namespaceのなかでさらに別のnamespaceを定義できる

namespace dxapp {
namespace utility {

/*!
 * @brief シェーダファイルをコンパイル、実行可能な状態にして返す
 * @pram[in] fileName コンパイルするファイル
 * @pram[in] profile シェーダプロファイル(シェーダの種類、バージョン)
 * @pram[out] shaderBlob コンパイルされたシェーダーコードを返す
 * @pram[out] errorBlob コンパイル失敗時にエラー内容を返す
 * @return 実行の成否をHRESULTで戻します
 */
HRESULT CompileShaderFromFile(const std::wstring& fileName,
                              const std::wstring& profile,
                              Microsoft::WRL::ComPtr<ID3DBlob>& shaderBlob,
                              Microsoft::WRL::ComPtr<ID3DBlob>& errorBlob);
}  // namespace utility
}  // namespace dxapp
