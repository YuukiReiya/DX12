#pragma once
namespace dxapp {
	class Device;
	/*!
	 * @brief LightingVS / PS�𓮂������߂̃R�[�h
	 */
	class LightingShader {
	public:
		/*!
		 * @brief �I�u�W�F�N�g�P�ʂŗp�ӂ���ׂ��\����
		 *       �i�萔�o�b�t�@��ObjectParam�Ɠ����j
		 */
		struct ObjectParam {
			DirectX::XMFLOAT4X4 world;
			DirectX::XMFLOAT4X4 texTrans;
		};

		//! �V�F�[�_���C�g�Ή����ƍ��킹��
		static constexpr int MaxDirLightNum{ 3 };
		/*!
		 * @brief ���s�����p���C�g�\����.���s�����Ȃ̂ňʒu�͎����Ȃ�
		 *        (�萔�o�b�t�@�Ɠ������e)
		 */
		struct Light {
			DirectX::XMFLOAT3 strength{ 0.5f, 0.5f, 0.5f };  //!< ���C�g�̐F�Ƌ���
			float pad1;  //!< �������̕��т𐮂���p
			DirectX::XMFLOAT3 direction{ 0, -1, 0 };  //!< �����̌���
			float pad2;  //!< �������̕��т𐮂���p
		};

		/*!
		 * @brief �V�[�����ʂ̃p�����[�^�������W�߂��\����
		 *        (�萔�o�b�t�@�Ɠ������e)
		 */
		struct SceneParam {
			DirectX::XMFLOAT4X4 view;      //!< �r���[�s��
			DirectX::XMFLOAT4X4 proj;      //!< �ˉe�s��
			DirectX::XMFLOAT4X4 viewProj;  //!< �r���[�ˉe
			DirectX::XMFLOAT3 eyePos;      //!< ���_
			float pad;                     //!< �������̕��т𐮂���p
			DirectX::XMFLOAT4 ambientLight;  //!< ����(��Ԃ𖞂����Ă�����̖��邳)
			Light lights[MaxDirLightNum];  //!< ���C�g�̔z��
		};

	public:
		/*!
		 * @brief �R���X�g���N�^
		 */
		LightingShader();

		/*!
		 * @brief �f�X�g���N�^
		 */
		~LightingShader();

		/*!
		 * @brief ������
		 */
		void Initialize(Device* device);

		/*!
		 * @brief �I������
		 */
		void Terminate();

		/*!
		 * @brief �t���[���ŃV�F�[�_���g�p����Ƃ��ɍŏ��ɌĂяo���֐�
		 * @details End���ĂԂ܂ł͈����̃R�}���h���X�g���g��
		 * @param[in,ont] commandList �R�}���h��ςނ��߂̃R�}���h���X�g
		 */
		void Begin(ID3D12GraphicsCommandList* commandList);

		/*!
		 * @brief �t���[���ŃV�F�[�_���g���ǂ������K���ĂԊ֐�
		 */
		void End();

		/*!
		 * @brief �V�F�[�_�������Ă���p�����[�^�ŃR�}���h�𔭍s
		 */
		void Apply();

		/*!
		 * @brief �萔�o�b�t�@b0��ݒ肷��
		 */
		void SetObjectParam(D3D12_GPU_VIRTUAL_ADDRESS addr);

		/*!
		 * @brief �萔�o�b�t�@b1��ݒ肷��
		 */
		void SetSceneParam(D3D12_GPU_VIRTUAL_ADDRESS addr);

		/*!
		 * @brief �萔�o�b�t�@b2��ݒ肷��
		 */
		void SetMaterialParam(D3D12_GPU_VIRTUAL_ADDRESS addr);

		/*!
		 * @brief �Q�Ƃ���SRV�̃f�X�N���v�^�q�[�v
		 * @param[in] heaps �f�X�N���v�^�q�[�v
		 * @param[in] offset �q�[�v���ł̃I�t�Z�b�g�ʒu
		 */
		void SetSrvDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset);

		/*!
		 * @brief �Q�Ƃ���T���v���̃f�X�N���v�^�q�[�v
		 * @param[in] heaps �f�X�N���v�^�q�[�v
		 * @param[in] offset �q�[�v���ł̃I�t�Z�b�g�ʒu
		 */
		void SetSamplerDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset);

		/*!
		 * @brief �_�~�[�e�N�X�`���̐ݒ�
		 * @param[in] heaps �f�X�N���v�^�q�[�v
		 * @param[in] offset �q�[�v���ł̃I�t�Z�b�g�ʒu
		 */
		void SetDammySrvDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset);

		/*!
		 * @brief �f�t�H���g�Ƃ��Ďg���T���v���̃f�X�N���v�^�q�[�v
		 * @param[in] heaps �f�X�N���v�^�q�[�v
		 * @param[in] offset �q�[�v���ł̃I�t�Z�b�g�ʒu
		 */
		void SetDefaultSamplerDescriptorHeap(ID3D12DescriptorHeap* heap,
			const int offset);

	private:
		//! ���������N���X
		class Impl;
		std::unique_ptr<Impl> impl_;
	};
}  // namespace dxapp