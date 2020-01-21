// í∏ì_ì¸óÕ
struct VSInput {
  float3 pos : POSITION;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD;
  uint2 boneID : BONEINDICES;
  float2 boneWeights : BONEWEIGHTS;
};

// í∏ì_èoóÕ
struct VSOutput {
  float4 pos : SV_POSITION;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD;
};
