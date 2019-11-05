#include "ShaderCommon.hlsli"

// 最低限文明的なバーテックスシェーダ

// 頂点シェーダの仕事の流れのイメージ
// 1. GPUに描画実行要求(CommandQueue->Execute)がきて頂点シェーダを起動
// 2. GPUがVRAMにある頂点バッファからデータを1頂点ずつ引数としてくれる
// 3. シェーダーで頂点に関係するデータを計算
// 4. その結果を次のデータに渡すためにリターンする

VSInput main(VSInput vIn)
{
	//初期化
	VSInput vOut = (VSInput)0;

	// いろいろ計算
	 // このサンプルでは入ってきた情報そのまま渡します
	vOut = vIn;

	// リターンした値は自動的に次のシェーダに流れる
	return vOut;
}
