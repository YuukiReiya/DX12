#include "ShaderCommon.hlsli"

// �Œ�������I�ȃo�[�e�b�N�X�V�F�[�_

// ���_�V�F�[�_�̎d���̗���̃C���[�W
// 1. GPU�ɕ`����s�v��(CommandQueue->Execute)�����Ē��_�V�F�[�_���N��
// 2. GPU��VRAM�ɂ��钸�_�o�b�t�@����f�[�^��1���_�������Ƃ��Ă����
// 3. �V�F�[�_�[�Œ��_�Ɋ֌W����f�[�^���v�Z
// 4. ���̌��ʂ����̃f�[�^�ɓn�����߂Ƀ��^�[������

VSInput main(VSInput vIn)
{
	//������
	VSInput vOut = (VSInput)0;

	// ���낢��v�Z
	 // ���̃T���v���ł͓����Ă�����񂻂̂܂ܓn���܂�
	vOut = vIn;

	// ���^�[�������l�͎����I�Ɏ��̃V�F�[�_�ɗ����
	return vOut;
}
