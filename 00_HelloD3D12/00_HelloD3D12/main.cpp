﻿#include <sdkddkver.h>
//MEMO:授業
#define WIN32_LEAN_AND_MEAN

//#define NO_MINMAX
//#define NODRAWTEXT
//#define NOGDI
//#define NOBITMAP
//#define NOMCX
//#define NOSERVICE
#include <wrl/client.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>
#pragma endregion

	constexpr wchar_t c_szWinsowTitle[] = L"00_HelloD3D12";
		uint32_t windowHeight)
		wc.cbWndExtra = 0;
		wc.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName = nullptr;
		wc.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);
			c_szClassName,			// WNDCLASSEXに設定したClassNameと同じものを使う
			c_szWinsowTitle,		// ウィンドウタイトル文字列(あとから変えられる)
			windowStyle,				// ウィンドウスタイル
			CW_USEDEFAULT,     // ウィンドウのX軸表示位置
			CW_USEDEFAULT,     // ウィンドウのY軸表示位置
			windowWidth,			// クライアントの幅。
			windowHeight,			// クライアントの高さ
			nullptr, nullptr,			// nullでOK
			hInstance,					// このアプリケーションのインスタンスハンドル
			throw std::runtime_error("CreateWindowExW Failed");
		}
		return hWnd;
		//不使用な引数をコンパイラに伝えてWarningを抑制する
#if _DEBUG

		HWND hWnd = Win::CreateHWND(hInstance, wStyle, rc.right - rc.left,
			rc.bottom - rc.top);
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