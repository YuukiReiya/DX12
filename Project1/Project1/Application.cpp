#include "pch.h"
#include "Application.h"

namespace dxapp {
	Application::Application() {}
	Application::~Application(){ Theadown(); }
	void Application::Setup(HWND hWnd, std::uint32_t Width, std::uint32_t Height)
	{
		_hWnd = hWnd;
	}
	void Application::Run()
	{
		Update();
		Render();
	}
	void Application::Theadown()
	{
	}
	void Application::Update()
	{
	}
	void Application::Render()
	{
	}
}