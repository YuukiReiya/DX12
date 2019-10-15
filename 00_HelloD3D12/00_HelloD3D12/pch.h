#pragma once
//"P"re "C"ompiled "H"eader

// プラットフォーム系
#include <SDKDDKVer.h>

#pragma region マクロ
#define WIN32_LEAN_AND_MEAN
// Windows.hから必要ない機能を止めるマクロ
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma endregion

//MEMO:自己定義
#pragma region マクロ

#pragma endregion


//ヘッダ
#include <Windows.h>

#pragma region メモリリーク
#if _DEBUG||DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#pragma endregion

#pragma region DX
#include <wrl/client.h>
#include <wrl/event.h>
#pragma comment(lib,"runtimeobject.lib")
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <d3d12.h>
#ifdef NTDDI_WIN10_RS2
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif // (NTDDI_WIN1-_RS2)
#if _DEBUG||DEBUG
#include <dxgidebug.h>
#endif
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma endregion

#pragma region D3DX
#include "d3dx12.h"
#pragma endregion

#pragma region C/C++
#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>
#include <fstream>
#pragma endregion

#pragma region C++17
// C++17から使えるファイルシステム
// コンパイラをc++17にするとexperimentalをとっても使える
//#include <experimental/filesystem>
//あるとエラーになる
#include <filesystem>
#pragma endregion

#pragma region シェーダー
#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")
#pragma endregion
