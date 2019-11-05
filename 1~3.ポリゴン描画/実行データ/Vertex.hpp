#pragma once

namespace dxapp
{
	/*!
			@brief	���_���
	*/
	struct VertexPositionColor
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
	};

	// D3D12_INPUT_ELEMENT_DESC�͒��_�f�[�^�̃��C�A�E�g(�����o�̈Ӗ��ƃT�C�Y�Ȃ�)���`
	// ���̃��C�A�E�g�ƒ��_�V�F�[�_�̓��͂������Ă��Ȃ��Ɛ������`��ł��Ȃ�
	// D3D12_INPUT_ELEMENT_DESC�̐錾
	//   SemanticName; - �Z�}���e�B�b�N��(�g�p�p�r)
	//   SemanticIndex; - �����Z�}���e�B�b�N������Ƃ��g��
	//   Format; - �g���Ă���f�[�^�t�H�[�}�b�g
	//   InputSlot; - �ʏ�0
	//   AlignedByteOffset; - �\���̂̐擪����̃o�C�g�I�t�Z�b�g
	//   InputSlotClass; - �ʏ��D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA
	//   InstanceDataStepRate; - �ʏ�0

	static constexpr D3D12_INPUT_ELEMENT_DESC c_VertexLayout[]
	{
		{"SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
		 D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
		 D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
}//namespace
