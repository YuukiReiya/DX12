#include "ShaderCommon.hlsli"

#include "BaseConstant.hlsli"

float4 main(VSInputPCNT vIn) : SV_POSITION{
  VSOutputPT vOut = (VSOutputPT)0;
// ����͒ʏ�̃��[���h�ϊ�
float4 pos = mul(float4(vIn.pos, 1.0f), World);
// ���C�g���猩���ˉe�ϊ�
vOut.pos = mul(pos, ViewProj);
vOut.uv = vIn.uv;

// ����Ńf�v�X���L�^���Ă��炤
return vOut.pos;
}
