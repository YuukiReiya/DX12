#include "pch.h"
#include "DeviceResources.h"

#include <array>
#include <unordered_map>

#include "TextRenderer.h"

#pragma warning(once : 26812)

using namespace DX;

template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma region Impl
/*!
 * @brief TextRenderer�̎���
 */
class TextRenderer::Impl {
 public:
   Impl(std::size_t frameCount);
  ~Impl(){}

  void Initialize(HWND hWnd, DeviceResources* device);
  void CreateD3D11Device();
  void CreateDWriteObject();
  void CreateRenderResource(HWND hWnd);

  void CreateColorBrush(const std::string& brushName, D2D1::ColorF::Enum color);

  void CreateTextFormat(
    const std::string& formatName, const std::wstring& fontName,
    float fontSize = 20.0f,
    DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL,
    DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL,
    DWRITE_FONT_STRETCH fontStretch = DWRITE_FONT_STRETCH_NORMAL);
  
  void CreateTextLayout(const std::string& layoutName, const std::wstring& text,
    float maxWidth, float maxHeight,
    const std::string& formatName);
  
  void AddDrawText(const std::wstring& text, float x, float y,
    const std::string& formatName, const std::string& brushName);

  void AddDrawTextLayout(const std::string& layoutName, float x,
    float y, const std::string& brushName);

  void Clear();
  void Render();

  //-------------

  //! D3D12�I�u�W�F�N�g�ւ̃L���b�V��
  DeviceResources* devResource_{};

  // D3D11 / D2D / DirectWrite�I�u�W�F�N�g
  ComPtr<ID3D11DeviceContext> d3d11DeviceContext_;  //!< D3D11�̕`�施�߂𔭍s����
  ComPtr<ID3D11On12Device> d3d11On12Device_;        //!< D3D12��11�̋��n��������f�o�C�X
  ComPtr<ID2D1Factory3> d2dFactory_;                //!< D2D�t�@�N�g���[
  ComPtr<ID2D1Device2> d2dDevice_;                  //!< D2D�f�o�C�X
  ComPtr<ID2D1DeviceContext2> d2dDeviceContext_;    //!< D2D�`�施�߂𔭍s����
  ComPtr<IDWriteFactory> dWriteFactory_;            //!< DW�n�̃I�u�W�F�N�g�t�@�N�g���[

  // �����_�[�^�[�Q�b�g
  std::vector<ComPtr<ID3D11Resource>> wrappedBackBuffers_;  //! D3D11�̃����_�[�^�[�Q�b�g
  std::vector<ComPtr<ID2D1Bitmap1>> d2dRenderTargets_;      //! D2D�ň����郌���_�[�^�[�Q�b�g

  //! �����������̍ő�l�i�l�ɈӖ��͂Ȃ��ł��A�����Ƒ傫���Ă��悢�j
  static constexpr std::uint32_t ArraySize_ = 2048;

  // �`�悷��e�L�X�g�͂��̃o�b�t�@�Ɍ��ԂȂ��l�ߍ��܂��
  // ���TextData�\���̂ɂ̓e�L�X�g�擪�̃|�C���^��o�^����
  // �Ȃ܂�std::string��ۑ������string�̃������m�ہE����Ō���������
  // ���������r�ꂿ�Ⴂ�����Ȃ̂ŁE�E�E
  std::array<wchar_t, ArraySize_> stringsBuffer_{}; //! �`�悷�镶����f�[�^

  //! 1�t���[�����ɓo�^����������
  std::uint32_t characterCount_ = 0;

  //! DrawText�ŕ`�悷��e�L�X�g�̏����W�߂��\����
  struct TextData {
    wchar_t* string{nullptr};            //!< strings_�ւ̃|�C���^
    std::uint32_t length{};              //!< �`�悷�镶����
    float x{};                           //!< �`�� x���W
    float y{};                           //!< �`�� y���W
    IDWriteTextFormat* format{nullptr};  //!< �g�p����t�H�[�}�b�g�̃|�C���^
    ID2D1SolidColorBrush* brush{nullptr};  //!< �g�p����u���V�̃|�C���^
  };

  //! DrawText�f�[�^�z��
  std::array<TextData, ArraySize_> textData_{};
  //! AddDrawText���ꂽ��
  int textDrawCount_{};

  struct LayoutTextData {
    D2D1_POINT_2F point{};                 //!< �`����W
    IDWriteTextLayout* layout{ nullptr };  //!< �g�p���郌�C�A�E�g�|�C���^
    ID2D1SolidColorBrush* brush{ nullptr };  //!< �g�p����u���V�̃|�C���^
  };

  //! DrawTextLayout�f�[�^�z��
  std::array<LayoutTextData, ArraySize_> layoutTextData_{};
  //! AddDrawTextLayout���ꂽ��
  int textDrawLayoutCount_{};

   //! �t�H�[�}�b�g�̘A�z�z��
   std::unordered_map<std::string, ComPtr<IDWriteTextFormat>> formats_{};

   //! ���C�A�E�g�̘A�z�z��
   std::unordered_map<std::string, ComPtr<IDWriteTextLayout>> layouts_{};

   //! �u���V�̘A�z�z��
   std::unordered_map<std::string, ComPtr<ID2D1SolidColorBrush>> brushs_{};
};

/*!
 * @brief �R���X�g���N�^
 * @param frameCount �o�b�N�o�b�t�@��
 */
TextRenderer::Impl::Impl(std::size_t frameCount)
  : wrappedBackBuffers_(frameCount)
  , d2dRenderTargets_(frameCount)
{}

/*!
 * @brief ������
 * @param hWnd �E�B���h�E�n���h��
 * @param device �f�o�C�X�I�u�W�F�N�g
 */
void TextRenderer::Impl::Initialize(HWND hWnd, DeviceResources* device) {
  devResource_ = device;

  CreateD3D11Device();
  CreateDWriteObject();
  CreateRenderResource(hWnd);
}

/*!
 * @brief D3D11�f�o�C�X�쐬
 */
void TextRenderer::Impl::CreateD3D11Device() {
  // �킴�킴D3D11�f�o�C�X�����̂� Direct2D��D3D11��ɍ\�z����Ă��邩��

  // ���̃R�}���h�L���[��11on12�f�o�C�X�ɓn��
  // 11on12�͓����I�ɂ�D3D12�ŕ`�悵�Ă�
  ID3D12CommandQueue* commandQueue[] = { devResource_->GetCommandQueue() };

  // D3D11On12CreateDevice��D3D12�f�o�C�X�Ń��b�v����D3D11���쐬�����
  // D3D11�̕`�施�߂��A���b�v����D3D12�ɗ���D3D12�̃R�}���h�L���[���g����
  // ��肢���ƕ`�悵�Ă����悤�ɂȂ��Ă�
  ComPtr<ID3D11Device> d3d11Device;
  D3D11On12CreateDevice(devResource_->GetD3DDevice(),
                        D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0,
                        reinterpret_cast<IUnknown**>(commandQueue), 1, 0,
                        &d3d11Device, &d3d11DeviceContext_, nullptr);
  d3d11Device.As(&d3d11On12Device_);
}

/*!
 * @brief DirectWrite�I�u�W�F�N�g�쐬
 */
void TextRenderer::Impl::CreateDWriteObject() {
    // �܂���D2D�f�o�C�X�R���e�L�X�g����邽�߂̃t�@�N�g���[���쐬
    D2D1_FACTORY_OPTIONS d2dFactoryOptions = {};
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
      __uuidof(ID2D1Factory3), &d2dFactoryOptions,
      &d2dFactory_);

    // D2D�f�o�C�X�쐬
    ComPtr<IDXGIDevice> dxgiDevice;
    d3d11On12Device_.As(&dxgiDevice);
    d2dFactory_->CreateDevice(dxgiDevice.Get(), &d2dDevice_);

    // �R���e�L�X�g��D3D12�ł����ƃL���[�Ƃ��R�}���h���X�g����̏�����S�����Ă����
    d2dDevice_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                                    &d2dDeviceContext_);

    // �Ō��DirectWrite���������߂̃t�@�N�g���[���쐬
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
      &dWriteFactory_);
}

/*!
 * @brief �`��ɕK�v�ȃ����_�[�^�[�Q�b�g�n�̃��\�[�X�쐬
 */
void TextRenderer::Impl::CreateRenderResource(HWND hWnd) {
  // �E�B���h�E��DPI(Dot Per Inch)���擾
  // �ŋ߂�Windows���ƃ��j�^���Ƃ�DPI���Ⴄ�̂ŃE�B���h�E�P�ʂŒ��ׂ�񂾂�
  auto dpi = static_cast<float>(GetDpiForWindow(hWnd));

  // �r�b�g�}�b�v�T�[�t�F�C�X�ݒ�
  D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
    D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
    D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
    dpi,
    dpi
  );

  // DirectWrite���g�������_�[�^�[�Q�b�g���쐬����
  auto backBufferCount = devResource_->GetBackBufferCount();
  for (UINT i = 0; i < backBufferCount; i++) {

    // D3D12�̃����_�[�^�[�Q�b�g��11on12�ł��g����悤�ɂ��܂�
    D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };

    // CreateWrappedResource�� 11on12�f�o�C�X���g���郊�\�[�X������
    d3d11On12Device_->CreateWrappedResource(
        devResource_->GetRenderTarget(i),   // ���̃����_�[�^�[�Q�b�g��
        &d3d11Flags,  // 11�̃����_�[�^�[�Q�b�g�Ƃ��Ă��g����悤�ɂ���
        D3D12_RESOURCE_STATE_RENDER_TARGET,    // �����X�e�[�g
        D3D12_RESOURCE_STATE_PRESENT,          // �J�ڂ���X�e�[�g
        IID_PPV_ARGS(&wrappedBackBuffers_[i])  // �󂯎��l
    );

    // surface�̓}�C�N���\�t�g�̏o���Ă�PC�B�ł͂Ȃ���
    // �\�ʂƂ��ʂ̈Ӗ��B�]���ĕ`��ʂ̈Ӗ�
    ComPtr<IDXGISurface> surface;
    wrappedBackBuffers_[i].As(&surface);

    // IDXGISurface���o�R����D2D�̃����_�[�^�[�Q�b�g�ɂ���
    d2dDeviceContext_->CreateBitmapFromDxgiSurface(
      surface.Get(),
      &bitmapProperties,
      &d2dRenderTargets_[i]
    );
  }
}

/*!
 * @brief �u���V�쐬
 */
void TextRenderer::Impl::CreateColorBrush(const std::string& brushName,
                                          D2D1::ColorF::Enum color) {
  // �쐬�ς݂Ȃ�Ȃɂ����Ȃ�
  if (brushs_.find(brushName) != brushs_.end()) {
    return;
  }

  // D2D�ŐF���w�肷��Ƃ���Brush�i�u���V�E�͂��j�I�u�W�F�N�g������
  ComPtr<ID2D1SolidColorBrush> brush;
  d2dDeviceContext_->CreateSolidColorBrush(D2D1::ColorF(color), &brush);
  brushs_.emplace(brushName, brush);
}

/*!
 * @brief �e�L�X�g�t�H�[�}�b�g�쐬
 */
void TextRenderer::Impl::CreateTextFormat(const std::string& name,
                                          const std::wstring& fontName,
                                          float fontSize,
                                          DWRITE_FONT_WEIGHT fontWeight,
                                          DWRITE_FONT_STYLE fontStyle,
                                          DWRITE_FONT_STRETCH fontStretch) {
  // �쐬�ς݂Ȃ�Ȃɂ����Ȃ�
  if (formats_.find(name) != formats_.end()) {
    return;
  }

  // IDWriteTextFormat�̓t�H���g�A�t�H���g�T�C�Y�Ȃǂ̏����ݒ�I�u�W�F�N�g
  ComPtr<IDWriteTextFormat> format;
  dWriteFactory_->CreateTextFormat(fontName.c_str(), NULL, fontWeight,
                                   fontStyle, fontStretch, fontSize, L"ja-jp",
                                   &format);
  formats_.emplace(name, format);
}

/*!
 * @brief �e�L�X�g���C�A�E�g�쐬
 */
void TextRenderer::Impl::CreateTextLayout(const std::string& layoutName,
                                          const std::wstring& text,
                                          float maxWidth, float maxHeight,
                                          const std::string& formatName) {
  if (layouts_.find(layoutName) != layouts_.end()) {
    return;
  }

  // �t�H�[�}�b�g���Ȃ��������O�Ȃ��Ă��炨��
  auto format = formats_.at(formatName);

  // IDWriteTextLayout�����C�A�E�g�I�u�W�F�N�g
  // �t�H�[�}�b�g���o����
  // �܂��e�L�X�g�ɑ΂��ĉ�����ŏ�������������܂�
  ComPtr<IDWriteTextLayout> layout;
  dWriteFactory_->CreateTextLayout(
      text.c_str(),                        // ������̃|�C���^
      static_cast<UINT32>(text.length()),  // ������̒���
      format.Get(),                        // �t�H�[�}�b�g
      maxWidth, maxHeight,                 // ���C�A�E�g�̋�`�T�C�Y
      &layout);

  layouts_.emplace(layoutName, layout);
}

/*!
 * @brief DrawText�f�[�^�\�z
 */
void TextRenderer::Impl::AddDrawText(const std::wstring& text, float x, float y,
                                     const std::string& formatName,
                                     const std::string& brushName) {
  auto length = static_cast<std::uint32_t>(text.length());
  // �ǉ������Ƃ��̕��������o�b�t�@�𒴂��Ă���Ƃ肠��������ł���
  assert(characterCount_ + length < ArraySize_);

  // ������̃R�s�[�J�n�ʒu�̃|�C���^�Ⴄ
  auto head = &stringsBuffer_[characterCount_];

  // TextData���\�z����...
  Impl::TextData data{
      head,
      length,
      x,
      y,
      formats_[formatName].Get(),  // �t�H�[�}�b�g�̐��|�C���^
      brushs_[brushName].Get(),    // �u���V�̐��|�C���^
  };
  // TextData�ǉ�
  textData_[textDrawCount_] = data;

  // ������f�[�^���o�b�t�@�ɃR�s�[
  text.copy(head, length);

  // ��������TextData�̐����X�V
  characterCount_ += length;
  textDrawCount_++;
}

/*!
 * @brief DrawTextLayout�f�[�^�\�z
 */
void TextRenderer::Impl::AddDrawTextLayout(const std::string& layoutName,
                                           float x, float y,
                                           const std::string& brushName) {
  // LayoutTextData���\�z����...
  LayoutTextData data{
      {x, y},
      layouts_[layoutName].Get(),  // ���C�A�E�g�̐��|�C���^
      brushs_[brushName].Get(),    // �u���V�̐��|�C���^
  };
  // LayoutTextData�ǉ�
  layoutTextData_[textDrawLayoutCount_] = data;

  // ��������TextData�̐����X�V
  textDrawLayoutCount_++;
}

/*!
 * @brief �����f�[�^���N���A
 */
void TextRenderer::Impl::Clear() {
  // ����͂��ڂ��Ă��邪�A�e�L�X�g�t�H�[�}�b�g�A�u���V�̍폜�͂��̂������
  // �܂Ƃ߂Ď��s�����ق����e�؂��낤
  // �폜�֐��Ȃǂ��Ăяo�����ƂɎ��s����ƁA�폜�����t���[���ł͂܂��g���Ă����I
  // �Ƃ����󋵂�����̂ō폜�������f�[�^���o���Ă����Ď��̃t���[���̐擪�ŏ����̂��悢
  characterCount_ = 0;
  textDrawCount_ = 0;
  textDrawLayoutCount_ = 0;
}

/*!
 * @brief �\�z�����f�[�^�ŕ`��
 */
void TextRenderer::Impl::Render() {
  const auto frameIndex = devResource_->GetCurrentFrameIndex();

  // �o�b�N�o�b�t�@���擾
  auto wrappedBackBuffer = wrappedBackBuffers_[frameIndex];
  d3d11On12Device_->AcquireWrappedResources(wrappedBackBuffer.GetAddressOf(),
                                            1);
  // �����_�[�^�[�Q�b�g�Z�b�g
  d2dDeviceContext_->SetTarget(d2dRenderTargets_[frameIndex].Get());

  // �`��J�n
  d2dDeviceContext_->BeginDraw();
  {
    d2dDeviceContext_->SetTransform(D2D1::Matrix3x2F::Identity());

    // �����_�[�^�[�Q�b�g�̃T�C�Y���擾
    D2D1_SIZE_F rtSize = d2dRenderTargets_[frameIndex]->GetSize();
    D2D1_RECT_F rect = D2D1::RectF(0, 0, rtSize.width, rtSize.height);

    // DrawText
    for (int i = 0; i < textDrawCount_; i++) {
      auto& data = textData_[i];

      // �`��G���A�ݒ�B������W�������������Ďg��
      // rect�̒��ɔ[�܂�悤�ɕ`�悷��̂ŕ�������Ȃ�Ɛ܂�Ԃ����
      rect.left = data.x;
      rect.top = data.y;
      // �`��
      d2dDeviceContext_->DrawText(data.string, data.length, data.format, &rect,
                                  data.brush);
    }

    // DrawTextLayout
    for (int i = 0; i < textDrawLayoutCount_; i++) {
      auto& data = layoutTextData_[i];
      // �`��
      d2dDeviceContext_->DrawTextLayout(data.point, data.layout, data.brush);
    }
  }

  d2dDeviceContext_->EndDraw();
  // �o�b�N�o�b�t�@���������B���\�[�X�̏�ԑJ�ڂ��s����
  d3d11On12Device_->ReleaseWrappedResources(wrappedBackBuffer.GetAddressOf(),
                                            1);
  // D3D11�̕`��R�}���h�𔭍s����
  d3d11DeviceContext_->Flush();
}
#pragma endregion

#pragma region TextRenderer
TextRenderer::TextRenderer(std::size_t frameCount)
    : impl_(new Impl(frameCount)){};

TextRenderer::~TextRenderer(){};

void TextRenderer::Initialize(HWND hWnd, DX::DeviceResources* deviceResources) {
  impl_->Initialize(hWnd, deviceResources);
  // �f�t�H���g�Ŏg�p�����u���V�ƃt�H�[�}�b�g������Ă���
  CreateColorBrush("default", D2D1::ColorF::Black);
  CreateTextFormat("default", L"���C���I");
}

void TextRenderer::CreateColorBrush(const std::string& brushName, D2D1::ColorF::Enum color) {
  impl_->CreateColorBrush(brushName, color);
}

void TextRenderer::CreateTextFormat(const std::string& formatName,
                                    const std::wstring& fontName,
                                    float fontSize,
                                    DWRITE_FONT_WEIGHT fontWeight,
                                    DWRITE_FONT_STYLE fontStyle,
                                    DWRITE_FONT_STRETCH fontStretch) {
  impl_->CreateTextFormat(formatName, fontName, fontSize, fontWeight, fontStyle, fontStretch);
}

void TextRenderer::AddDrawText(const std::wstring& text, float x, float y,
                               const std::string& formatName,
                               const std::string& brushName) {
  impl_->AddDrawText(text, x, y, formatName, brushName);
}

void TextRenderer::AddDrawText(const std::wstring& text, float x, float y) {
  impl_->AddDrawText(text, x, y, "default", "default");
}

void TextRenderer::AddDrawTextWithBrush(const std::wstring& text, float x,
                                        float y, const std::string& brushName) {
  impl_->AddDrawText(text, x, y, "default", brushName);
}

void TextRenderer::AddDrawTextWithFormat(const std::wstring& text, float x,
                                         float y,
                                         const std::string& formatName) {
  impl_->AddDrawText(text, x, y, formatName, "default");
}

void TextRenderer::CreateTextLayout(const std::string& layoutName,
                                    const std::wstring& text, float maxWidth,
                                    float maxHeight,
                                    const std::string& formatName) {
  impl_->CreateTextLayout(layoutName, text, maxWidth, maxHeight, formatName);
}

void TextRenderer::CreateTextLayoutWithDefaultFormat(
    const std::string& layoutName, const std::wstring& text, float maxWidth,
    float maxHeight) {
  impl_->CreateTextLayout(layoutName, text, maxWidth, maxHeight, "default");
}

void TextRenderer::AddDrawTextLayout(const std::string& layoutName, float x,
                                     float y) {
  impl_->AddDrawTextLayout(layoutName, x, y, "default");
}

void TextRenderer::AddDrawTextLayoutWithBrush(const std::string& layoutName,
                                              float x, float y,
                                              const std::string& brushName) {
  impl_->AddDrawTextLayout(layoutName, x, y, brushName);
}

void TextRenderer::Clear() { impl_->Clear(); }

void TextRenderer::Render() { impl_->Render(); }
#pragma endregion
