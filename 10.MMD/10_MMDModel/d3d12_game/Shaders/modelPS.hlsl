#include "model.hlsli"

// �}�e���A��
cbuffer MaterialParam : register(b2) {
  float4 Diffuse;
  float4 Specular;  // ����͎g��Ȃ��ł�
  float3 Ambient;   // ����͎g��Ȃ��ł�
  int useTexture;   // �e�N�X�`���g�p�t���O
};

//---

// �e�N�X�`��, �T���v��
Texture2D<float4> Texture : register(t0);
SamplerState Sampler : register(s0);

//---

// �f�B�t���[�Y�ƃe�N�X�`���J���[�����̃s�N�Z���V�F�[�_
float4 main(VSOutput pIn) : SV_TARGET {
  float4 color = Diffuse;
  if (useTexture == 1) {
    // �e�N�X�`���F�ƃ}�e���A���J���[��������
    color *= Texture.Sample(Sampler, pIn.uv);
  } 

  return color;
}
