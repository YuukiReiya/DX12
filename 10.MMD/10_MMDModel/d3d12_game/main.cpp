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
   // ImGui_ImplWin32_WndProcHandler��extern����ĂȂ��̂ł����ł��Ă��܂���...
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

  // �E�B���h�E�쐬�̂��߂̊�b��������
  // �E�B���h�E�쐬�ɕK�v�ȕϐ��B�ڂ����͎����Œ��ׂĂ�
  WNDCLASSEX wcex = {};
  wcex.hInstance = hInstance;  // ���̃A�v���P�[�V�����̃C���X�^���X�n���h��
  wcex.lpfnWndProc = WndProc;  // �E�B���h�E�v���V�[�W���̃|�C���^
  wcex.lpszClassName = kClassName;  // �E�B���h�E�N���X�l�[��
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszMenuName = nullptr;
  wcex.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);

  // WNDCLASSEX�I�u�W�F�N�g�ŃE�B���h�E�N���X��o�^����
  if (!RegisterClassExW(&wcex)) {
    throw std::runtime_error("RegisterClassExW Failed");
  }

  // �E�B���h�E�̍쐬
  HWND hWnd = CreateWindowExW(
      0,           // �Q�[���Ȃ�0��OK
      kClassName,  // WNDCLASSEX�ɐݒ肵��ClassName�Ɠ������̂��g��
      kWindowTitle,  // �E�B���h�E�^�C�g��������(���Ƃ���ς�����)
      windowStyle,       // �E�B���h�E�X�^�C��
      CW_USEDEFAULT,     // �E�B���h�E��X���\���ʒu
      CW_USEDEFAULT,     // �E�B���h�E��Y���\���ʒu
      windowWidth,       // �N���C�A���g�̕��B
      windowHeight,      // �N���C�A���g�̍���
      nullptr, nullptr,  // null��OK
      hInstance,  // ���̃A�v���P�[�V�����̃C���X�^���X�n���h��
      nullptr);  // null��OK

  if (hWnd == nullptr) {
    throw std::runtime_error("CreateWindowExW Failed");
  }
  return hWnd;
}
}  // namespace

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
  // UNREFERENCED_PARAMETER�͕s�g�p�Ȉ������R���p�C���ɓ`����Warning��}������
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

#if _DEBUG
  // ���������[�N�̌��o
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

  // CPU��DirectXMath�̌v�Z�Ɏg���邩�`�F�b�N
  if (!DirectX::XMVerifyCPUSupport()) {
    return 1;
  }

  // Windows10 CreatorsUpdate�œ��������j�^�P�ʂ�DPI���Ⴄ���ւ̑Ή�
  SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  // COM�̏���������
  Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(
      RO_INIT_MULTITHREADED);
  if (FAILED(initialize)) {
    return 1;  // ���s������I��
  }

  // RECT�͋�`�̍��E�㉺�̍��W���w�肷�邾���̒P���ȍ\����
  RECT rc = {0, 0, static_cast<LONG>(kClientWidth),
             static_cast<LONG>(kClientHeight)};

  // �E�B���h�E�X�^�C���i�^�C�g���o�[��ŏ����{�^���Ȃǁj�����߂�t���O
  // �E�B���h�E�̒[����������ł̃T�C�Y�ύX���֎~���Ă��܂��B
  DWORD windowStyle = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME;
  // AdjustWindowRect�Ŏ��ۂɗ~�����N���C�A���g�̈�����߂�
  AdjustWindowRect(&rc, windowStyle, FALSE);
  HWND hWnd = CreateHWND(hInstance, windowStyle, rc.right - rc.left,
                         rc.bottom - rc.top);

  // �쐬�����E�B���h�E����N���C�A���g�̈���擾
  GetClientRect(hWnd, &rc);

  // �A�v���P�[�V�����̐����A������
  auto app = std::make_unique<dxapp::Application>();
  app->Initialize(hWnd, rc.right - rc.left, rc.bottom - rc.top);

  // �����ł����̂ŃE�B���h�E��\��
  ShowWindow(hWnd, nCmdShow);

  // ���C�����[�v
  MSG msg = {};
  while (WM_QUIT != msg.message) {
    // �A�v���P�[�V������OS����̃C�x���g(���b�Z�[�W)����������
    // PeekMessage�ŃC�x���g���A�v�����Ŋm�F���ɍs��
    // ��Q�[���A�v���ł�GetMessage�֐����g�����Ǐڍׂ̓J�b�g�I
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      // �E�B���h�E�v���V�[�W���Ƀ��b�Z�[�W�𑗂�
      DispatchMessage(&msg);
    } else {
      // ���b�Z�[�W���Ȃ��Ƃ��ɃQ�[���A�v���̏���������
      app->Run();
    }
  }
  // �X�}�[�g�|�C���^�����ǁA����������Ă�ł݂܂�
  app.reset();

  // COM�̉��
  CoUninitialize();

  return static_cast<int>(msg.wParam);
}
