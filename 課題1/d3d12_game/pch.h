#pragma once

// プラットフォーム系
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#include <Windows.h>

// DEBUGビルド時のメモリリークチェック
#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

// Windows Runtime Library
#include <wrl/client.h>
#include <wrl/event.h>
#pragma comment(lib, "runtimeobject.lib")

// C/C++
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>  // C++17
#include <fstream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

// DirectX SDK
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <d3d12.h>
#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif

#ifdef _DEBUG
#include <dxgidebug.h>
#endif
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

// dxc シェーダコンパイル必要なヘッダとライブラリ
#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")

// 外部のソース
#pragma warning(push)
#pragma warning(once : 26812)  // 消すのは忍びないので１度だけ出してもらおう
#include "External/d3dx12.h"
#include "External/Keyboard.h"
#include "External/Mouse.h"
#pragma warning(pop)
