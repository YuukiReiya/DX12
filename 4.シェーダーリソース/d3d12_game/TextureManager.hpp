#pragma once
#include "Singleton.hpp"

namespace dxapp {
	class Device;

	class TextureManager {
	public:
		/*!
			@brief	�R���X�g���N�^
		*/
		TextureManager();
		/*!
			@brief	�f�X�g���N�^
		*/
		~TextureManager();

		TextureManager(const TextureManager&) = delete;
		TextureManager& operator=(const TextureManager&) = delete;
		TextureManager(TextureManager&&) = delete;
		TextureManager& operator=(TextureManager&&) = delete;

		/*!
			@brief �I�u�W�F�N�g�̔j���BSingletonFinalizer���Ăяo��
		*/
		bool LoadWICTextureFromFile(Device* device, const std::wstring& fileName, const std::string& assetName);

		/*!
			@brief	�I�u�W�F�N�g�̔j��
			@detail	SingletonFinalizer���Ăяo��
		*/
		Microsoft::WRL::ComPtr<ID3D12Resource> texture(const std::string& assetName);
	private:
		// �����pImpl�p�^�[��
		// �ڍׂȎ����͓����N���XImpl�ɂȂ���
		// ����ɂ��O���̃v���O�����΂��Ă�����B���ł���
		class Impl;
		std::unique_ptr<Impl> impl_;  //!< TextureManager�̎���
	};
}