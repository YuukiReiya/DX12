#include "ShaderCommon.hlsli"

VSOutputPT main(VSInputPCNT vIn) {
  VSOutputPT vOut = (VSOutputPT)0.0f;

  // w��1.0�����ݒ肵�Ă����
  vOut.pos = float4(vIn.pos, 1.0f);
  vOut.uv = vIn.uv;
  return vOut;
}
