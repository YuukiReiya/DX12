#pragma once
// namespace�͂�������G�c�ɐ�������ƃt�H���_�݂����Ȃ���
// 1�̃t�H���_�ɓ����t�@�C������1�������݂ł��Ȃ����t�H���_���Ⴆ��OK�ł��ˁH
// �v���O�����ł��������O�̊֐���^�͋�����܂���i���O�̏Փ˂Ƃ������j
// ������namespace�ł��Bnamespace�̒��ɃR�[�h�������邱�ƂŖ��O�̏Փ˂�����ł��܂�

// namespace�̂Ȃ��ł���ɕʂ�namespace���`�ł���

namespace dxapp
{
	namespace Util
	{
		HRESULT CompileShaderFromFile(const std::wstring& fileName, const std::wstring& profile, Microsoft::WRL::ComPtr<ID3DBlob>& shaderBlob, Microsoft::WRL::ComPtr<ID3DBlob>& errorBlob);
	}
}
