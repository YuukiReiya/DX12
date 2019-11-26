#include "ShaderCommon.hlsli"

VSOutputPT main(VSInputPCNT vIn) {
  VSOutputPT vOut = (VSOutputPT)0.0f;

  // w‚É1.0‚¾‚¯İ’è‚µ‚Ä‚¨‚í‚è
  vOut.pos = float4(vIn.pos, 1.0f);
  vOut.uv = vIn.uv;
  return vOut;
}
