#pragma once
#include <mutex>

namespace API
{
	//
	// みんな大好きシングルトンクラス
	// クラスのインスタンスが一つしかないことを保証する設計のこと
	// Unityでもスコア管理とかでつくったりするよね
	// ゲームだとなかなか相性が良くてつい作りたくなってしまう魔の設計
	// 何でもシングルトンにすると怖い人に刺されたりするので使用には用法用量を守ろう
	//

	class SingletonFinalizer
	{
	public:
		//	戻り値無し、引数無しの関数へのポインタ
		using FinalizerFunc = void(*)();

		static void AddFinalizer(FinalizerFunc func);

		static void Finalize();
	};

	template<typename T>
	class Singleton final
	{
	public:
		/*!
			@brief	インスタンス生成
			@detail	call_onceで1回しか呼び出せない保証としている
		*/
		static T& Create() {
			assert(instance_ == nullptr);
			std::call_once(onceFlag_, CreateInstance);
			return *instance_;
		}
		/*!
			@brief	インスタンスの取得
		*/
		static T& instance() {
			assert(instance_);
			return *instance_;
		}

	private:
		/*!
			@brief	生成の実装
		*/
		static void CreateInstance() {
			struct Helper : T {
				Helper() :T() {}
			};
			instance_ = new Helper;
			//オブジェクト終了時に呼び出す処理を登録
			SingletonFinalizer::AddFinalizer(&Singleton<T>::Destroy);
		}
		static void Destroy()
		{
			delete instance_;
			instance_ = nullptr;
		}
		static std::once_flag onceFlag_;//初期化のフラグ
		static T* instance_;//インスタンス
	};

	// staticメンバ変数なので定義が必要
	template<typename T>
	std::once_flag Singleton<T>::onceFlag_{};
	template<typename T>
	T* Singleton<T>::instance_{};
}//namespace API