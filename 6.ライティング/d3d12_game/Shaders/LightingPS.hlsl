#include "ShaderCommon.hlsli"

// �V�[���P�ʂ̒萔�o�b�t�@
cbuffer SceneParam : register(b1) {
	float4x4 View;      // �r���[�s��
	float4x4 Proj;      // �ˉe�s��
	float4x4 ViewProj;  // �r���[�ˉe
	float3 EyePos;      // �J�����̈ʒu
	float pad;          // �������̕��т𐮂���p
	// ����
	// ���������炢���͐^�����ɂȂ�s���R�ɂȂ�
	// �A���r�G���g�ŋ�Ԃ̖��邳���グ����
	float4 AmbientLight;
	Light Lights[NUM_DIR_LIGHT];  // ���s����3��
};

// �}�e���A��(���b�V���̑f�ސݒ�)���̒萔�o�b�t�@
cbuffer MaterialParam : register(b2) {
	float4 DiffuseAlbedo;  // �f�ނ̐F�Ǝv���Ă悢
	float3 Fresnel;        // ���ˌ��̐F
	float Roughness;       // �\�ʂ̑e���i���˂̋���ς��j
	int useTexture;  // �e�N�X�`���g�p�t���O�iint�Ȃ̂Œ��Ӂj
};

// �e�N�X�`��
Texture2D<float4> Texture : register(t0);
// �T���v��
sampler Sampler : register(s0);

//-------------------------------------------------------------------
// �V�F�[�_���Ŏg���}�e���A���i���f���̕\�ʂ̓����j�p�\����
struct Material {
	float4 diffuseAlbedo;  // �킩��₷�������ƕ\�ʐF
	float3 fresnel;        // ���ˌ�
	float shininess;       // �P�x
};

// �t���l���v�Z
// ���̕\�ʂ̌��̔��ˁE���܂̎��B�t���l���̎��Ō���
float3 ComputeFresnel(float3 fresnel, float3 normal, float3 lightVec) {
	float incidentAngle = saturate(dot(normal, lightVec));

	float f = 1.0f - incidentAngle;
	float3 result = fresnel + (1.0f - fresnel) * (f * f * f * f * f);

	return result;
}

// Blinn-Phong ���ʔ���
// ���̕\�ʂ̌��̔��˂��v�Z����
float3 BlinnPhongReflection(float3 lightStrength, float3 lightVec,
	float3 normal, float3 eyeVec, Material mat) {
	float shininess = mat.shininess * 256.0f;
	float3 halfVec = normalize(eyeVec + lightVec);

	// �ʂ̑e���̌W��
	float roughnessFactor = (shininess + 8.0f) *
		pow(max(dot(halfVec, normal), 0.0f), shininess) /
		8.0f;
	// �ʂ̔��ˌW��
	float3 fresnelFactor = ComputeFresnel(mat.fresnel, halfVec, lightVec);

	// ���ʔ��˒l�ɂȂ�
	float3 specular = fresnelFactor * roughnessFactor;
	specular = specular / (specular + 1.0f);

	// �}�e���A���g�U���˂ƃX�y�L�����[�l�A���C�g���x���甽�ˌ����ł���
	return (mat.diffuseAlbedo.rgb + specular) * lightStrength;
}

// ���s�������C�g�v�Z
float3 ComputeDirectionalLight(Light light, float3 normal, float3 eyeVec,
	Material mat) {
	// ���C�g�̋t�x�N�g��
	float3 lightVec = -light.direction;

	// ���C�g�Ɩʂ̌���������̓��������v�Z
	// �@���ƃ��C�g�̃x�N�g���̓��ς�1�ɋ߂��قǌ��������������Ă��邱�ƂɂȂ�
	float d = max(dot(lightVec, normal), 0.0f);
	// ���C�g���x�Ə�Z
	float3 lightStrength = light.strength * d;

	// ���ˌ��̌v�Z
	return BlinnPhongReflection(lightStrength, lightVec, normal, eyeVec, mat);
}

// ���C�g�̐������v�Z���J��Ԃ��܂�
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

// ���C�e�B���O�s�N�Z���V�F�[�_�[
float4 main(VSOutputLitTex pIn) : SV_TARGET{
	// �g�U���ːF(��{�ƂȂ�F)
	float4 diffuse = DiffuseAlbedo;
	// �V�F�[�_�ł�if�g�����B
	// ����������܂�g���ƘI���ɒx���Ȃ��
	if (useTexture != 0) {
		// �e�N�X�`���F�ƃ}�e���A���J���[��������
		diffuse = Texture.Sample(Sampler, pIn.uv) * DiffuseAlbedo;
	  }

	// ��������Z
	float4 ambient = AmbientLight * diffuse;

	// �@���𐳋K��
	pIn.normal = normalize(pIn.normal);
	// �J��������݂����_�ւ̃x�N�g�����
	float3 eyeVec = normalize(EyePos - pIn.posW);

	// �P�x
	float shininess = 1.0f - Roughness;
	// �}�e���A��
	Material mat = {diffuse, Fresnel, shininess};

	// ���C�e�B���O
	float3 shadowFactor = 1.0f;
	float4 result =
		ComputeLighting(Lights, mat, pIn.posW, pIn.normal, eyeVec, shadowFactor);

	// �����̒l�������čŏI�I�ȐF�̏o���オ��
	float4 color = ambient + result;
	// �A���t�@�̓f�B�t���[�Y�̒l�����炤
	color.a = diffuse.a;
	return color;
}
