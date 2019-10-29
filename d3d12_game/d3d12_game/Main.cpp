#include "Application.hpp"

namespace {
//! ウィンドウタイトルに表示する文字列
constexpr wchar_t kWindowTitle[] = L"D3DTutor";

//! ウィンドウクラスネーム
constexpr wchar_t kClassName[] = L"D3DTutorClassName";

//! デフォルトのウィンドウ幅(クライアント領域)
constexpr std::uint32_t kClientWidth = 1024;

//! デフォルトのウィンドウ高さ(クライアント領域), 16:9になるように計算してるだけ
constexpr std::uint32_t kClientHeight = kClientWidth / 16 * 9;

/*!
 * @brief ウィンドウプロシージャ。OSからのメッセージを処理するコールバック関数
 * @details OSからアプリにメッセージが届くので、メッセージごとに処理する。
 * @param[in] hWnd メッセージの処理対象になるウィンドウハンドル
 * @param[in] message メッセージ。これをswitchで処理するのが一般的
 * @param[in] wParam メッセージに応じて中身が変わる引数みたいなもの。その1
 * @param[in] lParam メッセージに応じて中身が変わる引数みたいなもの。その2
 * @return メッセージの処理結果を返す。値はメッセージによって変わる
 */
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
  // アプリで必要なmessaageについて対応していく
  switch (message) {
    case WM_CLOSE:  // ウィンドウを閉じる(xボタン押したときとか)
      // ウィンドウを閉じるときは大体アプリケーションが終わるとき。
      // 実際のゲームだと確認ダイアログ出したりしますが。
      // DestroyWindowでウィンドウの破棄を送信
      DestroyWindow(hWnd);
      break;

    case WM_DESTROY:  // アプリケーション破棄
      // DestroyWindowしないでPostQuitMessageを呼ぶとアプリがハングアップする
      // ちゃんと手順を踏んで終了するのです
      // WM_QUITを送信してメインループを抜ける
      PostQuitMessage(0);
      break;

    default:
      // 自分で処理しないときはDefWindowProcに必ず処理させる
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

/*!
 * @brief HWND（ウィンドウハンドル）の生成
 * @details 早い話がウィンドウそのものを作る
 * @param[in] hInstance メッセージの処理対象になるウィンドウハンドル
 * @param[in] windowStyle ウィンドウに必要な機能を設定
 * @param[in] windowWidth メッセージ。これをswitchで処理するのが一般的
 * @param[in] windowHeight メッセージに応じて中身が変わる引数みたいなもの。その1
 * @return 生成したウィンドウハンドルを返す
 */
HWND CreateHWND(HINSTANCE hInstance, DWORD windowStyle, uint32_t windowWidth,
                uint32_t windowHeight) {
  if (hInstance == nullptr) {
    throw std::invalid_argument("HINSTANCE is nullptr");
  }

  // ウィンドウ作成のための基礎情報をつくる
  // ウィンドウ作成に必要な変数。詳しくは自分で調べてね
  WNDCLASSEX wcex = {};
  wcex.hInstance = hInstance;  // このアプリケーションのインスタンスハンドル
  wcex.lpfnWndProc = WndProc;  // ウィンドウプロシージャのポインタ
  wcex.lpszClassName = kClassName;  // ウィンドウクラスネーム
  // ここからの設定の説明は省略。
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszMenuName = nullptr;
  wcex.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);

  // RegisterClassExW
  // WNDCLASSEXオブジェクトでウィンドウクラスを登録する
  if (!RegisterClassExW(&wcex)) {
    throw std::runtime_error("RegisterClassExW Failed");
  }

  // CreateWindowExW
  // ウィンドウの作成
  HWND hWnd = CreateWindowExW(
      0,           // ゲームなら0でOK
      kClassName,  // WNDCLASSEXに設定したClassNameと同じものを使う
      kWindowTitle,  // ウィンドウタイトル文字列(あとから変えられる)
      windowStyle,       // ウィンドウスタイル
      CW_USEDEFAULT,     // ウィンドウのX軸表示位置
      CW_USEDEFAULT,     // ウィンドウのY軸表示位置
      windowWidth,       // クライアントの幅。
      windowHeight,      // クライアントの高さ
      nullptr, nullptr,  // nullでOK
      hInstance,  // このアプリケーションのインスタンスハンドル
      nullptr);  // nullでOK

  if (hWnd == nullptr) {
    throw std::runtime_error("CreateWindowExW Failed");
  }
  return hWnd;
}
}  // namespace

/*!
 *   @brief デスクトップアプリでのメイン関数
 *   @param[in] hInstance インスタンスハンドル。OSがくれる管理番号みたいなの
 *   @param[in] hPrevInstance 使わない。完全に無視してよい
 *   @param[in] lpCmdLine アプリの引数
 *   @param[in] nCmdShow ウィンドウの表示状態を指定
 */
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
  // UNREFERENCED_PARAMETERは不使用な引数をコンパイラに伝えてWarningを抑制する
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

#if _DEBUG
  // メモリリークの検出
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

  // CPUがDirectXMathの計算に使えるかチェック
  if (!DirectX::XMVerifyCPUSupport()) {
    return 1;
  }

  // Windows10 CreatorsUpdateで入ったモニタ単位でDPIが違う環境への対応
  SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  // COMの初期化処理
  Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(
      RO_INIT_MULTITHREADED);
  if (FAILED(initialize)) {
    return 1;  // 失敗したら終了
  }

  // RECTは矩形の左右上下の座標を指定するだけの単純な構造体
  RECT rc = {0, 0, static_cast<LONG>(kClientWidth),
             static_cast<LONG>(kClientHeight)};

  // ウィンドウスタイル（タイトルバーや最小化ボタンなど）を決めるフラグ
  // ウィンドウの端っこをつかんでのサイズ変更を禁止しています。
  DWORD windowStyle = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME;
  // AdjustWindowRectで実際に欲しいクライアント領域を求める
  AdjustWindowRect(&rc, windowStyle, FALSE);
  HWND hWnd = CreateHWND(hInstance, windowStyle, rc.right - rc.left,
                         rc.bottom - rc.top);

  // 作成したウィンドウからクライアント領域を取得
  GetClientRect(hWnd, &rc);

  // アプリケーションの生成、初期化
  auto app = std::make_unique<dxapp::Application>();
  app->Initialize(hWnd, rc.right - rc.left, rc.bottom - rc.top);

  // 準備できたのでウィンドウを表示
  ShowWindow(hWnd, nCmdShow);

  // メインループ
  MSG msg = {};
  while (WM_QUIT != msg.message) {
    // アプリケーションにOSからのイベント(メッセージ)を処理する
    // PeekMessageでイベントをアプリ側で確認しに行く
    // 非ゲームアプリではGetMessage関数を使うけど詳細はカット！
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      // ウィンドウプロシージャにメッセージを送る
      DispatchMessage(&msg);
    } else {
      // メッセージがないときにゲームアプリの処理をする
      app->Run();
    }
  }
  // スマートポインタだけど、解放処理を呼んでみます
  app.reset();

  // COMの解放
  CoUninitialize();

  return static_cast<int>(msg.wParam);
}
