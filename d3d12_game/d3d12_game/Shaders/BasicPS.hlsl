#include "ShaderCommon.hlsli"

// �`��Ɏg���e�N�X�`��
// register��t0�̓V�F�[�_�[���\�[�X�r���[�̊i�[�ʒu���w��
// ������������̂�t1 ,t2�Ǝw�肷��
Texture2D<float4> Texture : register(t0);

// �e�N�X�`���T���v��(�s�N�Z���f�[�^�̎��o����)
// register��s0�̓T���v���[���\�[�X�̊i�[�ʒu
// �����������������
sampler Sampler : register(s0);

float4 main(VSOutputPCNT pIn) : SV_TARGET{
	// �V�F�[�_�[���\�[�X�r���[.Sample(�T���v��, uv���W)��
	// �e�N�X�`������T���v���[�Ŏw�肳��Ă�����@��
	// �Ώۂ�uv���W�̃s�N�Z���f�[�^�����o�����Ƃ��ł���

	// �e�N�X�`���F * ���_�J���[�����Ă݂�B���͒��_�J���[��1�Ȃ̂ŕω��Ȃ�
	float4 color = Texture.Sample(Sampler, pIn.uv) * pIn.color;

	return color;
}