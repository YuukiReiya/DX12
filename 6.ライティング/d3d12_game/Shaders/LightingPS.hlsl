#include "ShaderCommon.hlsli"

// シーン単位の定数バッファ
cbuffer SceneParam : register(b1) {
	float4x4 View;      // ビュー行列
	float4x4 Proj;      // 射影行列
	float4x4 ViewProj;  // ビュー射影
	float3 EyePos;      // カメラの位置
	float pad;          // メモリの並びを整える用
	// 環境光
	// 光が当たらい所は真っ黒になり不自然になる
	// アンビエントで空間の明るさを底上げする
	float4 AmbientLight;
	Light Lights[NUM_DIR_LIGHT];  // 平行光源3つ
};

// マテリアル(メッシュの素材設定)毎の定数バッファ
cbuffer MaterialParam : register(b2) {
	float4 DiffuseAlbedo;  // 素材の色と思ってよい
	float3 Fresnel;        // 反射光の色
	float Roughness;       // 表面の粗さ（反射の具合が変わる）
	int useTexture;  // テクスチャ使用フラグ（intなので注意）
};

// テクスチャ
Texture2D<float4> Texture : register(t0);
// サンプラ
sampler Sampler : register(s0);

//-------------------------------------------------------------------
// シェーダ内で使うマテリアル（モデルの表面の特性）用構造体
struct Material {
	float4 diffuseAlbedo;  // わかりやすく言うと表面色
	float3 fresnel;        // 反射光
	float shininess;       // 輝度
};

// フレネル計算
// 物体表面の光の反射・屈折の式。フレネルの式で検索
float3 ComputeFresnel(float3 fresnel, float3 normal, float3 lightVec) {
	float incidentAngle = saturate(dot(normal, lightVec));

	float f = 1.0f - incidentAngle;
	float3 result = fresnel + (1.0f - fresnel) * (f * f * f * f * f);

	return result;
}

// Blinn-Phong 鏡面反射
// 物体表面の光の反射を計算する
float3 BlinnPhongReflection(float3 lightStrength, float3 lightVec,
	float3 normal, float3 eyeVec, Material mat) {
	float shininess = mat.shininess * 256.0f;
	float3 halfVec = normalize(eyeVec + lightVec);

	// 面の粗さの係数
	float roughnessFactor = (shininess + 8.0f) *
		pow(max(dot(halfVec, normal), 0.0f), shininess) /
		8.0f;
	// 面の反射係数
	float3 fresnelFactor = ComputeFresnel(mat.fresnel, halfVec, lightVec);

	// 鏡面反射値になる
	float3 specular = fresnelFactor * roughnessFactor;
	specular = specular / (specular + 1.0f);

	// マテリアル拡散反射とスペキュラー値、ライト強度から反射光ができる
	return (mat.diffuseAlbedo.rgb + specular) * lightStrength;
}

// 平行光源ライト計算
float3 ComputeDirectionalLight(Light light, float3 normal, float3 eyeVec,
	Material mat) {
	// ライトの逆ベクトル
	float3 lightVec = -light.direction;

	// ライトと面の向きから光の当たり具合を計算
	// 法線とライトのベクトルの内積が1に近いほど光が強く当たっていることになる
	float d = max(dot(lightVec, normal), 0.0f);
	// ライト強度と乗算
	float3 lightStrength = light.strength * d;

	// 反射光の計算
	return BlinnPhongReflection(lightStrength, lightVec, normal, eyeVec, mat);
}

// ライトの数だけ計算を繰り返します
float4 ComputeLighting(Light lights[NUM_DIR_LIGHT], Material mat, float3 pos,
	float3 normal, float3 eye, float3 shadowFactor) {
	float3 result = 0.0f;

	int i = 0;
	for (i = 0; i < NUM_DIR_LIGHT; i++) {
		result +=
			shadowFactor[i] * ComputeDirectionalLight(lights[i], normal, eye, mat);
	}
	return float4(result, 0.0f);
}

// ライティングピクセルシェーダー
float4 main(VSOutputLitTex pIn) : SV_TARGET{
	// 拡散反射色(基本となる色)
	float4 diffuse = DiffuseAlbedo;
	// シェーダでもif使えるよ。
	// ただしあんまり使うと露骨に遅くなるよ
	if (useTexture != 0) {
		// テクスチャ色とマテリアルカラーを混ぜる
		diffuse = Texture.Sample(Sampler, pIn.uv) * DiffuseAlbedo;
	  }

	// 環境光を乗算
	float4 ambient = AmbientLight * diffuse;

	// 法線を正規化
	pIn.normal = normalize(pIn.normal);
	// カメラからみた頂点へのベクトル作る
	float3 eyeVec = normalize(EyePos - pIn.posW);

	// 輝度
	float shininess = 1.0f - Roughness;
	// マテリアル
	Material mat = {diffuse, Fresnel, shininess};

	// ライティング
	float3 shadowFactor = 1.0f;
	float4 result =
		ComputeLighting(Lights, mat, pIn.posW, pIn.normal, eyeVec, shadowFactor);

	// 環境光の値を加えて最終的な色の出来上がり
	float4 color = ambient + result;
	// アルファはディフューズの値をもらう
	color.a = diffuse.a;
	return color;
}
