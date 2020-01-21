#include "model.hlsli"

// �I�u�W�F�N�g�萔
cbuffer ObjectParam : register(b0) {
  float4x4 World;
}

// �I�u�W�F�N�g�萔
cbuffer SceneParam : register(b1) {
  float4x4 View;
  float4x4 Proj;
  float4x4 ViewProj;
}

VSOutput main(VSInput vIn) {
  VSOutput vOut = (VSOutput)0;

  // ���_�����[�J�����W���烏�[���h���W�ɕϊ�
  float4 posW = mul(float4(vIn.pos, 1.0f), World);
  vOut.pos = mul(posW, ViewProj);

  // �@�������[�J�����烏�[���h�ɕϊ�
  vOut.normal = mul(vIn.normal, (float3x3)World);

  vOut.uv = vIn.uv; 
  return vOut;
}
