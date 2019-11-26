#include "ShaderCommon.hlsli"

VSOutputPT main(VSInputPCNT vIn) {
  VSOutputPT vOut = (VSOutputPT)0.0f;

  // wに1.0だけ設定しておわり
  vOut.pos = float4(vIn.pos, 1.0f);
  vOut.uv = vIn.uv;
  return vOut;
}
