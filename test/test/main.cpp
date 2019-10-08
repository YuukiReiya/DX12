#include <sdkddkver.h>
//MEMO:授業
#define WIN32_LEAN_AND_MEAN

//#define NO_MINMAX
//#define NODRAWTEXT
//#define NOGDI
//#define NOBITMAP
//#define NOMCX
//#define NOSERVICE
//#define NOHELP
//MEMO:自己定義マクロ

//ヘッダ
#include <Windows.h>
#if _DEBUG
//メモリリーク
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#pragma region DX
#include <wrl/client.h>
#include <wrl/event.h>
#pragma comment(lib,"runtimeobject.lib")
#pragma endregion

#pragma region C/C++
#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>
#pragma endregion

namespace Win
{
	constexpr wchar_t c_szWinsowTitle[] = L"00_HelloD3D12";
	constexpr wchar_t c_szClassName[] = L"D3DTutorClassName";
	constexpr std::uint32_t c_WindowWidth = 1024;
	constexpr std::uint32_t c_WindowHeight = c_WindowWidth / 16 * 9;

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}
	
	HWND CreateHWND(HINSTANCE hInstance, DWORD windowStyle, uint32_t windowWidth,
		uint32_t windowHeight)
	{
		if (hInstance == nullptr) {
			throw std::invalid_argument("HINSTANCE is nullptr");
		}

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
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName = nullptr;
		wc.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);

		if (!RegisterClassExW(&wc)) {
			throw std::runtime_error("RegisterClassExW Failed");
		}

		HWND hWnd = CreateWindowExW(
			0,								// ゲームなら0でOK
			c_szClassName,			// WNDCLASSEXに設定したClassNameと同じものを使う
			c_szWinsowTitle,		// ウィンドウタイトル文字列(あとから変えられる)
			windowStyle,				// ウィンドウスタイル
			CW_USEDEFAULT,     // ウィンドウのX軸表示位置
			CW_USEDEFAULT,     // ウィンドウのY軸表示位置
			windowWidth,			// クライアントの幅。
			windowHeight,			// クライアントの高さ
			nullptr, nullptr,			// nullでOK
			hInstance,					// このアプリケーションのインスタンスハンドル
			nullptr						// nullでOK
		);
		if (hWnd == nullptr) {
			throw std::runtime_error("CreateWindowExW Failed");
		}
		return hWnd;
	}

	//エントリーポイント
	int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevIns,
		_In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
	{
		//不使用な引数をコンパイラに伝えてWarningを抑制する
		UNREFERENCED_PARAMETER(hPrevIns);
		UNREFERENCED_PARAMETER(lpCmdLine);

#pragma region メモリリーク
#if _DEBUG
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
#pragma endregion

		SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		//COM
		Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
		//失敗
		if (FAILED(initialize)) { return -1; }
		RECT rc = { 0,0,static_cast<LONG>(Win::c_WindowWidth),static_cast<LONG>(Win::c_WindowWidth) };
		DWORD wStyle = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME;
		AdjustWindowRect(&rc, wStyle, FALSE);
		HWND hWnd = Win::CreateHWND(hInstance, wStyle, rc.right - rc.left,
			rc.bottom - rc.top);
		ShowWindow(hWnd, nCmdShow);

		//メインループ
		MSG msg = {};
		while (WM_QUIT != msg.message)
		{
			// アプリケーションにOSからのイベント(メッセージ)を処理する
			// PeekMessageでイベントをアプリ側で確認しに行く
			// 非ゲームアプリではGetMessage関数を使うけど詳細はカット！
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				// ウィンドウプロシージャにメッセージを送る
				DispatchMessage(&msg);
			}
			else {

			}
		}
		//COM開放
		CoUninitialize();
		return static_cast<int>(msg.wParam);
	}

}//Win namespace