#include "ShaderCommon.hlsli"

#include "BaseConstant.hlsli"

float4 main(VSInputPCNT vIn) : SV_POSITION{
  VSOutputPT vOut = (VSOutputPT)0;
// これは通常のワールド変換
float4 pos = mul(float4(vIn.pos, 1.0f), World);
// ライトから見た射影変換
vOut.pos = mul(pos, ViewProj);
vOut.uv = vIn.uv;

// これでデプスを記録してもらう
return vOut.pos;
}
