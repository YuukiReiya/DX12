#include "ShaderCommon.hlsli"

cbuffer ObjectPram : register(b0) {
  float4x4 world;
  float4x4 worldViewProj;
}

VSOutputPCNT main(VSInputPCNT vIn) {
  VSOutputPCNT vOut = (VSOutputPCNT)0;

  vOut.pos = mul(vIn.pos, worldViewProj);

  vOut.color = vIn.color;
  vOut.normal = vIn.normal;
  vOut.uv = vIn.uv;

  return vOut;
}
