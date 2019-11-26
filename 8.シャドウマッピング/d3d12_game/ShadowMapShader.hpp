#pragma once

namespace dxapp {
	class Device;

	/*!
	 * @brief ShadowMapVS / PS�𓮂������߂̃R�[�h
	 *        �f�v�X�������L�^����̂łق��̃V�F�[�_�[�����V���v��
	 */

	class ShadowMapShader {
	public:
		/*!
		 * @brief �R���X�g���N�^
		 */
		ShadowMapShader();

		/*!
		 * @brief �f�X�g���N�^
		 */
		~ShadowMapShader();

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

	private:
		//! ���������N���X
		class Impl;
		std::unique_ptr<Impl> impl_;
	};
}  // namespace dxapp