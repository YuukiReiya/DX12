
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

// �I�u�W�F�N�g�P�ʂ̒萔�o�b�t�@
cbuffer ObjectParam : register(b0) {
  float4x4 World;     // ���[���h�s��
  float4x4 TexTrans;  // UV�̃g�����X�t�H�[���p
};

// �V�[���P�ʂ̒萔�o�b�t�@
cbuffer SceneParam : register(b1) {
  float4x4 View;      // �r���[�s��
  float4x4 Proj;      // �ˉe�s��
  float4x4 ViewProj;  // �r���[�ˉe�s��
  float3 EyePos;      // �J�����̈ʒu
  float pad;          // �������̕��т𐮂���p
  float4
      AmbientLight;  // �����i����������Ȃ��ꏊ�ł��Œ���A���̃��C�g���͖��邭�Ȃ�j
  Light Lights[NUM_DIR_LIGHT];  // ���C�g�i3���j
};
