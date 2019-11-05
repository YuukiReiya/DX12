#include "ShaderCommon.hlsli"

float4 main(VSOutputPCNT pIn) : SV_TARGET {
  float4 color = pIn.color;
  return color;
}
