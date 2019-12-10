//
// pch.h
// Header for standard system include files.
//

#pragma once

#include <winsdkver.h>
#define _WIN32_WINNT 0x0A00
#include <sdkddkver.h>

// Use the C++ standard templated min/max
#define NOMINMAX

//// DirectX apps don't need GDI
//#define NODRAWTEXT
//#define NOGDI
//#define NOBITMAP
//
// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl/client.h>
#include <wrl/event.h>

#include <d3d12.h>

#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif

#include <DirectXColors.h>
#include <DirectXMath.h>

#include "d3dx12.h"

#include <algorithm>
#include <exception>
#include <memory>
#include <stdexcept>

#include <stdio.h>

// To use graphics and CPU markup events with the latest version of PIX, change
// this to include <pix3.h> then add the NuGet package WinPixEventRuntime to the
// project.
#include <pix.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

namespace DX {
// Helper class for COM exceptions
class com_exception : public std::exception {
 public:
  com_exception(HRESULT hr) : result(hr) {}

  virtual const char* what() const override {
    static char s_str[64] = {};
    sprintf_s(s_str, "Failure with HRESULT of %08X",
              static_cast<unsigned int>(result));
    return s_str;
  }

 private:
  HRESULT result;
};

// Helper utility converts D3D API failures into exceptions.
inline void ThrowIfFailed(HRESULT hr) {
  if (FAILED(hr)) {
    throw com_exception(hr);
  }
}
}  // namespace DX

#pragma region DirectWrite
//	lib
#pragma comment(lib,"d2d1.lib")
#pragma comment(lib,"dwrite.lib")
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")// DirectWrite内部で使用

//	header
#include <d2d1_3.h>			/*!< D2D */
#include <d3d11on12.h>	/*!< D3D12から11のデバイスを作るUtil */
#include <dwrite.h>			/*!< DirectWrite */
#pragma endregion

#pragma region imgui
//	ImGui 本体
#include "External/imgui/imgui.h"

// アプリケーションに組み込みヘッダ
#include "External/imgui/examples/imgui_impl_dx12.h"
#include "External/imgui/examples/imgui_impl_win32.h"
#pragma endregion
