// �}�e���A��(���b�V���̑f�ސݒ�)���̒萔�o�b�t�@
cbuffer MaterialParam : register(b2) {
  float4x4 MatTrans;     // UV�̃g�����X�t�H�[���p
  float4 DiffuseAlbedo;  // �f�ނ̐F�Ǝv���Ă悢
  float3 Fresnel;        // ���ˌ��̐F
  float Roughness;       // �\�ʂ̑e���i���˂̋���ς��j
  int useTexture;  // �e�N�X�`���g�p�t���O�iint�Ȃ̂Œ��Ӂj
};
