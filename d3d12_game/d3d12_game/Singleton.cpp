#include "pch.h"
#include "Singleton.hpp"

using namespace API;

namespace {
	// �V���O���g���̍ő�o�^��
	constexpr int c_MaxFinalizersSize = 32;
	// �o�^���ꂽ�I������
	static int sFinalizerNum = 0;

	// �I���������o���Ă����z��
	// �V���O���g���̓C���X�^���X�����������Ȃ����A���̓�������
	// �����Ɖ�����ɕq���ɂȂ肪���B
	// �����Ő������ɉ��������z��ɓ˂�����ł����āA������͔z���
	// ���K���珈�����邱�ƂŁA������ɋC���g���K�v���Ȃ��Ȃ�
	static std::array<SingletonFinalizer::FinalizerFunc, c_MaxFinalizersSize>sFinalizers;
}//namespce

void SingletonFinalizer::AddFinalizer(FinalizerFunc func)
{
	assert(sFinalizerNum < c_MaxFinalizersSize);
	// �I���������o���Ă���
	sFinalizers[sFinalizerNum++] = func;
}

void SingletonFinalizer::Finalize()
{
	for (int i = sFinalizerNum - 1; i >= 0; --i)
	{
		(*sFinalizers[i])();
	}
	sFinalizerNum = 0;
}

