//MEMO:����

//�w�b�_
#include"Application.hpp"

//���O���
namespace {
	constexpr wchar_t c_szWinsowTitle[] = L"00_HelloD3D12";
	constexpr wchar_t c_szClassName[] = L"D3DTutorClassName";
	constexpr std::uint32_t c_WindowWidth = 1024;
	constexpr std::uint32_t c_WindowHeight = c_WindowWidth / 16 * 9;

#pragma region Windows CALLBACK
	//�E�B���h�E�v���V�[�W��
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
#pragma region �E�B���h�E�v����`
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
		//TODO:���̃L���X�g������CreateBRUSH
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName = nullptr;
		wc.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);
#pragma endregion
		if (!RegisterClassExW(&wc)) { throw std::runtime_error("RegisterClassExW Failed."); }
#pragma region �E�B���h�E�쐬
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

//�G���g���[�|�C���g
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
	//�s�g�p�Ȉ������R���p�C���ɓ`����Warning��}������
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
#pragma region ���������[�N
#if _DEBUG||DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
#pragma endregion
#pragma region �Z�b�g�A�b�v
	//DPI���Ⴄ���ւ̑Ή�
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	//COM
	Microsoft::WRL::Wrappers::RoInitializeWrapper initializer(RO_INIT_MULTITHREADED);
	if (FAILED(initializer)) { return -1; }
	//�E�B���h�E
	RECT rc = { 0,0,static_cast<LONG>(c_WindowWidth),static_cast<LONG>(c_WindowWidth) };	DWORD wStyle = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME;	AdjustWindowRect(&rc, wStyle, FALSE);
	auto hWnd = CreateHWND(hInstance, wStyle, (rc.right - rc.left), (rc.bottom - rc.top));
	//�N���C�A���g�̈�擾
	GetClientRect(hWnd, &rc);
	//�A�v���P�[�V����
	auto app = std::make_unique<dxapp::Application>();
	app->Setup(hWnd, (rc.right - rc.left), (rc.bottom - rc.top));
#pragma endregion
	//�E�B���h�E�̕\��
	ShowWindow(hWnd, nCmdShow);
#pragma region ���C�����[�v
	MSG msg{};
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			//�E�B���h�E�v���V�[�W���Ƀ��b�Z�[�W���M
			DispatchMessage(&msg);
		}
		else {
			//�A�v���P�[�V��������
			app->Execute();
		}
	}
#pragma endregion
#pragma region �������̖����I�J��
	//APP
	app.reset();
	//COM
	CoUninitialize();
#pragma endregion
	return static_cast<int>(msg.wParam);
}
