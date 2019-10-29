#pragma once
#include <mutex>

namespace API
{
	//
	// �݂�ȑ�D���V���O���g���N���X
	// �N���X�̃C���X�^���X��������Ȃ����Ƃ�ۏ؂���݌v�̂���
	// Unity�ł��X�R�A�Ǘ��Ƃ��ł������肷����
	// �Q�[�����ƂȂ��Ȃ��������ǂ��Ă���肽���Ȃ��Ă��܂����̐݌v
	// ���ł��V���O���g���ɂ���ƕ|���l�Ɏh���ꂽ�肷��̂Ŏg�p�ɂ͗p�@�p�ʂ���낤
	//

	class SingletonFinalizer
	{
	public:
		//	�߂�l�����A���������̊֐��ւ̃|�C���^
		using FinalizerFunc = void(*)();

		static void AddFinalizer(FinalizerFunc func);

		static void Finalize();
	};

	template<typename T>
	class Singleton final
	{
	public:
		/*!
			@brief	�C���X�^���X����
			@detail	call_once��1�񂵂��Ăяo���Ȃ��ۏ؂Ƃ��Ă���
		*/
		static T& Create() {
			assert(instance_ == nullptr);
			std::call_once(onceFlag_, CreateInstance);
			return *instance_;
		}
		/*!
			@brief	�C���X�^���X�̎擾
		*/
		static T& instance() {
			assert(instance_);
			return *instance_;
		}

	private:
		/*!
			@brief	�����̎���
		*/
		static void CreateInstance() {
			struct Helper : T {
				Helper() :T() {}
			};
			instance_ = new Helper;
			//�I�u�W�F�N�g�I�����ɌĂяo��������o�^
			SingletonFinalizer::AddFinalizer(&Singleton<T>::Destroy);
		}
		static void Destroy()
		{
			delete instance_;
			instance_ = nullptr;
		}
		static std::once_flag onceFlag_;//�������̃t���O
		static T* instance_;//�C���X�^���X
	};

	// static�����o�ϐ��Ȃ̂Œ�`���K�v
	template<typename T>
	std::once_flag Singleton<T>::onceFlag_{};
	template<typename T>
	T* Singleton<T>::instance_{};
}//namespace API