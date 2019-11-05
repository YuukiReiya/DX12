#pragma once
// namespaceはすごく大雑把に説明するとフォルダみたいなもの
// 1つのフォルダに同じファイル名は1つしか存在できないがフォルダが違えばOKですね？
// プログラムでも同じ名前の関数や型は許されません（名前の衝突とかいう）
// そこでnamespaceです。namespaceの中にコードをくくることで名前の衝突を回避できます

// namespaceのなかでさらに別のnamespaceを定義できる

namespace dxapp
{
	namespace Util
	{
		HRESULT CompileShaderFromFile(const std::wstring& fileName, const std::wstring& profile, Microsoft::WRL::ComPtr<ID3DBlob>& shaderBlob, Microsoft::WRL::ComPtr<ID3DBlob>& errorBlob);
	}
}
