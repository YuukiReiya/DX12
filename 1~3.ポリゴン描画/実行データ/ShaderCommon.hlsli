// High Level Shader Language = HLSL
// �\�����̂�C���ꂪ�x�[�X�B�\���̂�֐������܂��B
// �f�[�^�^��֐��͐�p�̂��̂ɂȂ����肵�܂�

// hlsli��hlsl [i]nclude �̗��炵����
// �V�F�[�_�[�Ŏg�p�p�x�̍����\���̂�֐��������Ă����ƕ֗�

struct VSInput
{
	float4 pos:SV_POSITION;
	float4 color:COLOR;
};

// �ϐ����̌��ɂ���u: SV_POSITION | COLOR�v�̓Z�}���e�B�N�X�Ƃ����܂�
// �ϐ��̖������w���������������B
// �V�F�[�_�̊O����肷��ϐ��ɂ͎w�肷��K�v������܂��B
// �Ƃ肠�������́u�ց[�v�Ǝv���Ă����Ă悢
