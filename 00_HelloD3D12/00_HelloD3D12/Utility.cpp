#include "Utility.hpp"

using namespace Microsoft::WRL;

HRESULT dxapp::Util::CompileShaderFromFile(const std::wstring& fileName, const std::wstring& profile, Microsoft::WRL::ComPtr<ID3DBlob>& shaderBlob, Microsoft::WRL::ComPtr<ID3DBlob>& errorBlob)
{
	HRESULT ret = { E_FAIL };

	std::filesystem::path filePath(fileName);
	std::ifstream infile(filePath);
	if (!infile) { throw std::runtime_error("shader not found."); }

	std::vector<char>shaderSrc;
	shaderSrc.resize(uint32_t(infile.seekg(0, infile.end).tellg()));
	infile.seekg(0, infile.beg).read(shaderSrc.data(), shaderSrc.size());//ファイル読み込み
	//! バイトデータの塊をシェーダに変換するオブジェクト
	ComPtr<IDxcLibrary>lib;
	ret = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&lib));// IDxcLibrary作成
	//! バイトデータから返還された、シェーダデータを受け取るオブジェクト
	ComPtr<IDxcBlobEncoding> src;
	ret = lib->CreateBlobWithEncodingFromPinned(shaderSrc.data(), UINT(shaderSrc.size()), CP_ACP, &src);
	//コンパイルフラグ
	LPCWSTR compileFlags[]{
#if _DEBUG||DEBUG
		L"/Z",L"/O0",
#else
		L"/O2"//最適化設定
#endif
	};

	//コンパイラ
	ComPtr<IDxcCompiler>compiler;
	ret = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));  // コンパイラ作成
	//! シェーダーでのincludeを解決しているらしい
	ComPtr<IDxcIncludeHandler>includeHeader;
	ret = lib->CreateIncludeHandler(includeHeader.ReleaseAndGetAddressOf());
	ComPtr<IDxcOperationResult>dxcResult;
	//シェーダーデータを実行可能な状態にコンパイル
	ret = compiler->Compile(src.Get(), filePath.wstring().c_str(), L"main", profile.c_str(), compileFlags, _countof(compileFlags), nullptr, 0, includeHeader.Get(), &dxcResult);

	HRESULT hr{ S_FALSE };
	ret = dxcResult->GetStatus(&hr);
	if (SUCCEEDED(hr)) {
		// コンパイルが成功
		// shaderBlobにコンパイル後のシェーダデータをいれる
		dxcResult->GetResult(
			reinterpret_cast<IDxcBlob**>(shaderBlob.GetAddressOf()));
	}
	else {
		// コンパイルが失敗
   // errorBlobにエラーを渡す
		dxcResult->GetErrorBuffer(
			reinterpret_cast<IDxcBlobEncoding**>(errorBlob.GetAddressOf()));
	}
	return hr;
}
