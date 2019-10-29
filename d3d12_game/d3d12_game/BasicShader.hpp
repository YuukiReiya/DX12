#pragma once
// ����͂Ƃ肠����1�V�F�[�_�ɑΉ������p�N���X�Ƃ��Ă���܂��B

namespace dxapp{

	/*!
		@brief	�x�[�V�b�N�V�F�[�_�[�Ŏg�p����萔�o�b�t�@

		MEMO:	�{���Ȃ�K��\����
	*/
	struct BasicShaderCB 
	{
		// XMFLOAT4X4�ƃV�F�[�_��float4x4�͓����`�ɂȂ�
		// Transform�f�[�^���s��(�}�g���N�X)�ɕϊ����Ċi�[
		DirectX::XMFLOAT4X4 world{};
		DirectX::XMFLOAT4X4 wvp{};
	};

	/*!
		@brief BasicShader�ł̃��\�[�X�A�f�X�N���v�^�̊J�n�C���f�N�X
	*/
	enum class BasicShaderResourceIndex {
		Constant = 0,
		Srv = 2
	};

	class BasicShader
	{
	public:
		BasicShader();
		~BasicShader();

		void Setup(ID3D12Device* device);
		void Teardown();

		/*!
			 @brief �t���[���ŃV�F�[�_���g�p����Ƃ��ɍŏ��ɌĂяo���֐�
			 @details End���ĂԂ܂ł͈����̃R�}���h���X�g���g��
			 @param[in,ont] commandList �R�}���h��ςނ��߂̃R�}���h���X�g
		*/
		void Begin(ID3D12GraphicsCommandList* commandList);
		/*!
			 @brief �t���[���ŃV�F�[�_���g���ǂ������K���ĂԊ֐�
		 */

		void End();
		/*!
			@brief �V�F�[�_�������Ă���p�����[�^�ŃR�}���h�𔭍s
		*/
		void Apply();

		/*!
			@brief �Q�Ƃ���萔�o�b�t�@�̃f�X�N���v�^
			@param[in] heaps �f�X�N���v�^�q�[�v
			@param[in] offset �q�[�v���ł̃I�t�Z�b�g�ʒu
		*/
		void SetCBufferDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset);
		/*!
			@brief �Q�Ƃ���SRV�̃f�X�N���v�^
			@param[in] heaps �f�X�N���v�^�q�[�v
			@param[in] offset �q�[�v���ł̃I�t�Z�b�g�ʒu
		*/
		void SetSrvDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset);
		/*!
			 @brief �Q�Ƃ���T���v���̃f�X�N���v�^
		  	 @param[in] heaps �f�X�N���v�^�q�[�v
		   	 @param[in] offset �q�[�v���ł̃I�t�Z�b�g�ʒu
		*/
		void SetSamplerDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset);
	private:
		class Impl;
		std::unique_ptr<Impl> impl_;
	};
}//namespace dxapp