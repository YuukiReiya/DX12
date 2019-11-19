// High Level Shader Language = HLSL
// �\�����̂�C���ꂪ�x�[�X�B�\���̂�֐������܂��B
// �f�[�^�^��֐��͐�p�̂��̂ɂȂ����肵�܂�

// hlsli��hlsl [i]nclude �̗��炵����
// �V�F�[�_�[�Ŏg�p�p�x�̍����\���̂�֐��������Ă����ƕ֗�

// float�^�̍��W�ƐF������\����
struct VSInput {
  float4 pos : SV_POSITION;  // float4��float��4��(xyzw)����f�[�^�^
  float4 color : COLOR;
};

// �ϐ����̌��ɂ���u: SV_POSITION | COLOR�v�̓Z�}���e�B�N�X�Ƃ����܂�
// �ϐ��̖������w���������������B
// �V�F�[�_�̊O����肷��ϐ��ɂ͎w�肷��K�v������܂��B
// �Ƃ肠�������́u�ց[�v�Ǝv���Ă����Ă悢

// ���_�V�F�[�_�̓���
struct VSInputPCNT {
  float3 pos : POSITION;
  float4 color : COLOR;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD;
};

// ���_�V�F�[�_����o��
struct VSOutputPCNT {
  float4 pos : SV_POSITION;
  float4 color : COLOR;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD;
};

//-------------------------------------------------------------------
// 11/12 �ǉ���������
//-------------------------------------------------------------------
// ���C�e�B���O���_�V�F�[�_�̏o��
struct VSOutputLitTex {
  float4 posVPW : SV_POSITION;  // �ŏI�I�Ȓ��_���W
  float3 posW : POSITION;       // ���[���h�ϊ������������_
  float3 normal : NORMAL;       // ���[���h�ϊ������@��
  float2 uv : TEXCOORD;         // �e�N�X�`�����W
};

// �f�B���N�V���i�����C�g3�܂őΉ�
#define NUM_DIR_LIGHT 3

// ���C�g�\���́i�f�B���N�V���i�������Ή��j
// �萔�o�b�t�@��16byte�P�ʂŃ������������B
// float��4byte = float3�Ȃ�12byte�ɂȂ�B
// ���̂���float3�̂��Ƃ�float3����ׂ�ƁA16byte�������������̋��E��
// �܂����ł��܂��̂ŁA���̂悤�ȂƂ��͋l�ߕ�(padding)�ŕ��т𐮂��Ă���
struct Light {
  float3 strength;  // ���C�g�F�E���邳
  float pad1;
  float3 direction;  // �f�B���N�V���i�����C�g�̌���
  float pad2;
};

//-------------------------------------------------------------------
// 11/12 �ǉ������܂�
//-------------------------------------------------------------------
