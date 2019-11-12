#pragma once
#include <mutex>

namespace dxapp {
//
// みんな大好きシングルトンクラス
// クラスのインスタンスが一つしかないことを保証する設計のこと
// Unityでもスコア管理とかでつくったりするよね
// ゲームだとなかなか相性が良くてつい作りたくなってしまう魔の設計
// 何でもシングルトンにすると怖い人に刺されたりするので使用には用法用量を守ろう
//

/*!
 * @brief シングルトンを生成の逆順で解放するためのクラス
 */
class SingletonFinalizer {
 public:
  // 戻り値なし、引数なしの関数へのポインタ
  using FinalizerFunc = void (*)();

  /*!
   * @brief シングルトンの終了処理を登録
   */
  static void AddFinalizer(FinalizerFunc func);

  /*!
   * @brief シングルトンインスタンスをかたずける
   */
  static void Finalize();
};

/*!
 * @brief シングルトンテンプレート
 * @details シングルトンの生成と逆順で解放することが保証されています
 */
template <typename T>
class Singleton final {
 public:
  /*!
   * @brief インスタンスの生成、call_onceで1回しか呼び出せない保証
   */
  static T& Create() {
    assert(instance_ == nullptr);  // 二回呼び出したら死ぬ
    std::call_once(onceFlag_, CreateInstance);
    return *instance_;
  }

  /*!
   * @brief インスタンスの取得
   */
  static T& instance() {
    assert(instance_);
    return *instance_;
  }

 private:
  /*!
   * @brief 生成の実装
   */
  static void CreateInstance() {
    struct Helper : T {
      Helper() : T() {}
    };

    instance_ = new Helper;
    // オブジェクト終了時に呼び出す処理を登録
    SingletonFinalizer::AddFinalizer(&Singleton<T>::Destroy);
  }

  /*!
   * @brief オブジェクトの破棄。SingletonFinalizerが呼び出す
   */
  static void Destroy() {
    delete instance_;
    instance_ = nullptr;
  }

  static std::once_flag onceFlag_;  //!< 初期化時のフラグ
  static T* instance_;              //!< 唯一のインスタンス
};

// staticメンバ変数なので定義が必要
template <typename T>
std::once_flag Singleton<T>::onceFlag_{};
template <typename T>
T* Singleton<T>::instance_{};
}  // namespace dxapp
