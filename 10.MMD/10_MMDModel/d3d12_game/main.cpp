#include "Application.hpp"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg,
  WPARAM wParam, LPARAM lParam);

namespace {

constexpr wchar_t kWindowTitle[] = L"D3DTutor";
constexpr wchar_t kClassName[] = L"D3DTutorClassName";

constexpr std::uint32_t kClientWidth = 1024;
constexpr std::uint32_t kClientHeight = kClientWidth / 16 * 9;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
   // ImGui_ImplWin32_WndProcHandlerがexternされてないのでここでしてしまおう...
  if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) {
    return TRUE;
  }

  switch (message) {
    case WM_CLOSE:
      DestroyWindow(hWnd);
      break;

    case WM_DESTROY:
      PostQuitMessage(0);
      break;

    case WM_ACTIVATEAPP:
      DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
      DirectX::Mouse::ProcessMessage(message, wParam, lParam);
      break;

    case WM_INPUT:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_MOUSEHOVER:
      DirectX::Mouse::ProcessMessage(message, wParam, lParam);
      break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
      DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
      if (DirectX::Keyboard::Get().GetState().Escape) {
        DestroyWindow(hWnd);
      }
      break;
#pragma endregion

    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

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
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszMenuName = nullptr;
  wcex.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);

  // WNDCLASSEXオブジェクトでウィンドウクラスを登録する
  if (!RegisterClassExW(&wcex)) {
    throw std::runtime_error("RegisterClassExW Failed");
  }

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
