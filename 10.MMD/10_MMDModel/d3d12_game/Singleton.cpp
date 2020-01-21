#include "Singleton.hpp"

namespace dxapp {

namespace {
// シングルトンの最大登録数
constexpr int MaxFinalizersSize = 32;  // 32個もあったら十分

// 登録された終了処理
static int sFinalizerNum = 0;

// 終了処理を覚えておく配列
// シングルトンはインスタンスが一つしかつくれないが、その特性から
// 生成と解放順に敏感になりがち。
// そこで生成時に解放処理を配列に突っ込んでおいて、解放時は配列の
// お尻から処理することで、解放順に気を使う必要がなくなる
static std::array<SingletonFinalizer::FinalizerFunc, MaxFinalizersSize>
    sFinalizers;
}  // namespace

void SingletonFinalizer::AddFinalizer(FinalizerFunc func) {
  assert(sFinalizerNum < MaxFinalizersSize);
  // 終了処理を覚えておく
  sFinalizers[sFinalizerNum++] = func;
}

void SingletonFinalizer::Finalize() {
  // 登録した逆順で終了していくのでもし依存関係があっても安心
  for (int i = sFinalizerNum - 1; i >= 0; --i) {
    (*sFinalizers[i])();
  }
  sFinalizerNum = 0;
}
}  // namespace dxapp
