//MEMO:授業

//ヘッダ
#include"Application.hpp"

//名前空間
namespace {
	constexpr wchar_t c_szWinsowTitle[] = L"00_HelloD3D12";
	constexpr wchar_t c_szClassName[] = L"D3DTutorClassName";
	constexpr std::uint32_t c_WindowWidth = 1024;
	constexpr std::uint32_t c_WindowHeight = c_WindowWidth / 16 * 9;

#pragma region Windows CALLBACK
	//ウィンドウプロシージャ
	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{		case WM_CLOSE:			DestroyWindow(hWnd);			break;		case WM_DESTROY:			PostQuitMessage(0);			break;		default:
			return DefWindowProc(hWnd, message, wParam, lParam);		}
		return 0;
	}
#pragma endregion

#pragma region Function
	HWND CreateHWND(HINSTANCE hInstance, DWORD wStyle, uint32_t windowWidth, uint32_t windowHeight)
	{
		if (hInstance == nullptr) { throw std::invalid_argument("HINSTANCE is nullptr."); }
#pragma region ウィンドウ要件定義
		WNDCLASSEX wc = {};
		wc.hInstance = hInstance;
		wc.lpfnWndProc = WndProc;
		wc.lpszClassName = c_szClassName;
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		//TODO:このキャスト嫌い→CreateBRUSH
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName = nullptr;
		wc.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);
#pragma endregion
		if (!RegisterClassExW(&wc)) { throw std::runtime_error("RegisterClassExW Failed."); }
#pragma region ウィンドウ作成
		HWND hWnd = CreateWindowExW(
			0,
			c_szClassName,
			c_szWinsowTitle,
			wStyle,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			windowWidth,
			windowHeight,
			nullptr, nullptr,
			hInstance,
			nullptr
		);
#pragma endregion
		if (hWnd == nullptr) { throw std::runtime_error("CreateWindowExW Failed."); }
		return hWnd;
	}
#pragma endregion

}//namespace

//エントリーポイント
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
	//不使用な引数をコンパイラに伝えてWarningを抑制する
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
#pragma region メモリリーク
#if _DEBUG||DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
#pragma endregion
#pragma region セットアップ
	//DPIが違う環境への対応
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	//COM
	Microsoft::WRL::Wrappers::RoInitializeWrapper initializer(RO_INIT_MULTITHREADED);
	if (FAILED(initializer)) { return -1; }
	//ウィンドウ
	RECT rc = { 0,0,static_cast<LONG>(c_WindowWidth),static_cast<LONG>(c_WindowWidth) };	DWORD wStyle = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME;	AdjustWindowRect(&rc, wStyle, FALSE);
	auto hWnd = CreateHWND(hInstance, wStyle, (rc.right - rc.left), (rc.bottom - rc.top));
	//クライアント領域取得
	GetClientRect(hWnd, &rc);
	//アプリケーション
	auto app = std::make_unique<dxapp::Application>();
	app->Setup(hWnd, (rc.right - rc.left), (rc.bottom - rc.top));
#pragma endregion
	//ウィンドウの表示
	ShowWindow(hWnd, nCmdShow);
#pragma region メインループ
	MSG msg{};
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			//ウィンドウプロシージャにメッセージ送信
			DispatchMessage(&msg);
		}
		else {
			//アプリケーション処理
			app->Execute();
		}
	}
#pragma endregion
#pragma region メモリの明示的開放
	//APP
	app.reset();
	//COM
	CoUninitialize();
#pragma endregion
	return static_cast<int>(msg.wParam);
}
