#include "ShaderCommon.hlsli"

// ����͒萔�o�b�t�@���R�g����

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
  float3 EyePos;      // ���_
  float padding[1];   // �������A���C�����g�p
  float4
      AmbientLight;  // �����i����������Ȃ��ꏊ�ł��Œ���A���̃��C�g���͖��邭�Ȃ�j
  Light Lights[NUM_DIR_LIGHT];  // ���C�g�i3���j
};

// �}�e���A��(���b�V���̑f�ސݒ�)���̒萔�o�b�t�@
cbuffer MaterialParam : register(b2) {
  float4x4 MatTrans;     // UV�̃g�����X�t�H�[���p
  float4 DiffuseAlbedo;  // �f�ނ̐F�Ǝv���Ă悢
  float3 Fresnel;        // ���ˌ��̐F
  float Roughness;       // �\�ʂ̑e���i���˂̋���ς��j

  int useTexture;  // �e�N�X�`���g�p�t���O�iint�Ȃ̂Œ��Ӂj
};

VSOutputLitTex main(VSInputPCNT vIn) {
  VSOutputLitTex vOut = (VSOutputLitTex)0;

  // ���_�����[�J���i���f�����O�j���W���烏�[���h���W�ɕϊ�
  float4 posW = mul(float4(vIn.pos, 1.0f), World);
  vOut.posW = posW.xyz;
  // VPW���v�Z
  vOut.posVPW = mul(posW, ViewProj);

  // �@�������[�J�����烏�[���h�ɕϊ�
  vOut.normal = mul(vIn.normal, (float3x3)World);

  // �e�N�X�`���̃g�����X�t�H�[��
  // UV�̃X�N���[����e�N�X�`���̉�]�Ȃǂ��g���ĕ֗�
  float4 uv = mul(float4(vIn.uv, 0.0f, 1.0f), TexTrans);
  uv = mul(uv, MatTrans);
  vOut.uv = uv.xy;  // .xy��xyzw��4��������xy�������n����

  return vOut;
}
