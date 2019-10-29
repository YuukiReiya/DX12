#include "ShaderCommon.hlsli"

// 描画に使うテクスチャ
// registerのt0はシェーダーリソースビューの格納位置を指定
// 複数枚つかえるのでt1 ,t2と指定する
Texture2D<float4> Texture : register(t0);

// テクスチャサンプラ(ピクセルデータの取り出し方)
// registerのs0はサンプラーリソースの格納位置
// こちらも複数つかえる
sampler Sampler : register(s0);

float4 main(VSOutputPCNT pIn) : SV_TARGET{
	// シェーダーリソースビュー.Sample(サンプラ, uv座標)で
	// テクスチャからサンプラーで指定されている方法で
	// 対象のuv座標のピクセルデータを取り出すことができる

	// テクスチャ色 * 頂点カラーをしてみる。今は頂点カラーが1なので変化ない
	float4 color = Texture.Sample(Sampler, pIn.uv) * pIn.color;

	return color;
}