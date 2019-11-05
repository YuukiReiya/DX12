#include "ShaderCommon.hlsli"

// �V�F�[�_���O��������炤�f�[�^���u���\�[�X�v�Ƃ����܂�
// ���L��4��ނ�����
//  register: b - �R���X�^���g(�萔)�o�b�t�@�[�r���[(CBV)
//  register: t - �V�F�[�_�[���\�[�X�r���[(SRV)�A�e�N�X�`������
//  register: s - �T���v���[�A�e�N�X�`���̎Q�ƕ��@
//  register: u - �����w�肳��Ă��Ȃ��A�N�Z�X �r���[(UAV)�B�Ƃ肠�����Y��Ă悢

// �v���O����������n����郊�\�[�X�ƁA�V�F�[�_�[�̃��\�[�X�͈�v���Ă���K�v������

// cbuffer ���O : register(b���W�X�^�ԍ�) {
//  �l1
//  �l2
//  ...
// } �Œ�`�ł���B

// register(b���W�X�^�ԍ�)�́A���\�[�X���i�[����Ă���ꏊ���w��
// �܂����\�[�X�͕ϐ��ł͂Ȃ��A�v���O����������炤���́B
// �f�[�^�̕��я��̓V�F�[�_�ƃv���O�������ō��킹��K�v������B
// ���_�V�F�[�_�̓��͂Ɠ����d�g�݂���
cbuffer ObjectPram : register(b0) {  // �萔�o�b�t�@��b0�Ɋi�[�����
  float4x4 world;                    // �I�u�W�F�N�g�̃��[���h�s��
  float4x4 worldViewProj;  // �I�u�W�F�N�g�̎ˉe�ϊ��ςݍs��
}

VSOutputPCNT main(VSInputPCNT vIn) {
  VSOutputPCNT vOut = (VSOutputPCNT)0;

  // mul�͊|���Z�Afloat4��float4x4�s������������Ɋ|���Z���Ă���܂�
  // ���_��worldViewProj���|���Z���邱�Ƃ�1���_���Ƃɍ��W�ϊ�
  vOut.pos = mul(vIn.pos, worldViewProj);

  vOut.color = vIn.color;
  vOut.normal = vIn.normal;
  vOut.uv = vIn.uv;

  return vOut;
}
