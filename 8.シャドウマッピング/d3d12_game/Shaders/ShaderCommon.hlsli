// High Level Shader Language = HLSL
// �\�����̂�C���ꂪ�x�[�X�B�\���̂�֐������܂��B
// �f�[�^�^��֐��͐�p�̂��̂ɂȂ����肵�܂�

// hlsli��hlsl [i]nclude �̗��炵����
// �V�F�[�_�[�Ŏg�p�p�x�̍����\���̂�֐��������Ă����ƕ֗�

// �ϐ����̌��ɂ���u: SV_POSITION | COLOR�v�̓Z�}���e�B�N�X�Ƃ����܂�
// �ϐ��̖������w���������������B
// �V�F�[�_�̊O����肷��ϐ��ɂ͎w�肷��K�v������܂��B
// �Ƃ肠�������́u�ց[�v�Ǝv���Ă����Ă悢

//-------------------------------------------------------------------
// ���_�V�F�[�_�ւ̓��̓f�[�^�^
//-------------------------------------------------------------------
// float�^�̍��W�ƐF������\����
struct VSInput {
  float4 pos : SV_POSITION;  // float4��float��4��(xyzw)����f�[�^�^
  float4 color : COLOR;
};

// ���_�V�F�[�_�̓���
struct VSInputPCNT {
  float3 pos : POSITION;
  float4 color : COLOR;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD;
};

//-------------------------------------------------------------------
// ���_�V�F�[�_����̏o�̓f�[�^�^
//-------------------------------------------------------------------
struct VSOutputPT {
  float4 pos : SV_POSITION;  // �ˉe�ϊ��ςݒ��_
  float2 uv : TEXCOORD;      // UV
};

struct VSOutputPCNT {
  float4 pos : SV_POSITION;  // �ˉe�ϊ��ςݒ��_
  float4 color : COLOR;      // ���_�J���[
  float3 normal : NORMAL;    // ���[���h�ϊ������@��
  float2 uv : TEXCOORD;      // UV
};

struct VSOutputLitTex {
  float4 posVPW : SV_POSITION;  // �ŏI�I�Ȓ��_���W
  float3 posW : POSITION;       // ���[���h�ϊ������������_
  float3 normal : NORMAL;       // ���[���h�ϊ������@��
  float2 uv : TEXCOORD;         // UV
};
