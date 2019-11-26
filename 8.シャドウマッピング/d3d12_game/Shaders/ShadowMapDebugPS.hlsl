#include "ShaderCommon.hlsli"

// �e�N�X�`��
Texture2D<float4> ShadowMap : register(t0);
// �T���v��
sampler Sampler : register(s0);

float4 main(VSOutputPT pIn) : SV_Target {
  return float4(ShadowMap.Sample(Sampler, pIn.uv).rrr, 1.0f);
}
