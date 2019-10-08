#pragma once
#include "pch.h"
#include <iostream>
namespace dxapp {

	class Application
	{
	public:
		Application();
		virtual ~Application();
		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

		void Setup(HWND hWnd, std::uint32_t Width,
			std::uint32_t Height);

		void Run();

		void Theadown();
	private:

		void Update();
		void Render();
		HWND _hWnd{ nullptr };
	};
}