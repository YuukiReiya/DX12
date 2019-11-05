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
	infile.seekg(0, infile.beg).read(shaderSrc.data(), shaderSrc.size());//�t�@�C���ǂݍ���
	//! �o�C�g�f�[�^�̉���V�F�[�_�ɕϊ�����I�u�W�F�N�g
	ComPtr<IDxcLibrary>lib;
	ret = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&lib));// IDxcLibrary�쐬
	//! �o�C�g�f�[�^����Ԋ҂��ꂽ�A�V�F�[�_�f�[�^���󂯎��I�u�W�F�N�g
	ComPtr<IDxcBlobEncoding> src;
	ret = lib->CreateBlobWithEncodingFromPinned(shaderSrc.data(), UINT(shaderSrc.size()), CP_ACP, &src);
	//�R���p�C���t���O
	LPCWSTR compileFlags[]{
#if _DEBUG||DEBUG
		L"/Z",L"/O0",
#else
		L"/O2"//�œK���ݒ�
#endif
	};

	//�R���p�C��
	ComPtr<IDxcCompiler>compiler;
	ret = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));  // �R���p�C���쐬
	//! �V�F�[�_�[�ł�include���������Ă���炵��
	ComPtr<IDxcIncludeHandler>includeHeader;
	ret = lib->CreateIncludeHandler(includeHeader.ReleaseAndGetAddressOf());
	ComPtr<IDxcOperationResult>dxcResult;
	//�V�F�[�_�[�f�[�^�����s�\�ȏ�ԂɃR���p�C��
	ret = compiler->Compile(src.Get(), filePath.wstring().c_str(), L"main", profile.c_str(), compileFlags, _countof(compileFlags), nullptr, 0, includeHeader.Get(), &dxcResult);

	HRESULT hr{ S_FALSE };
	ret = dxcResult->GetStatus(&hr);
	if (SUCCEEDED(hr)) {
		// �R���p�C��������
		// shaderBlob�ɃR���p�C����̃V�F�[�_�f�[�^�������
		dxcResult->GetResult(
			reinterpret_cast<IDxcBlob**>(shaderBlob.GetAddressOf()));
	}
	else {
		// �R���p�C�������s
   // errorBlob�ɃG���[��n��
		dxcResult->GetErrorBuffer(
			reinterpret_cast<IDxcBlobEncoding**>(errorBlob.GetAddressOf()));
	}
	return hr;
}
