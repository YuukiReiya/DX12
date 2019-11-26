#include "ShaderCommon.hlsli"

#include "BaseConstant.hlsli"
#include "MaterialConstant.hlsli"

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
