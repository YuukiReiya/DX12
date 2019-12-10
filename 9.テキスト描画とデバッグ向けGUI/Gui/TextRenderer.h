#pragma once
#include <string>

namespace DX {
class DeviceResources;
};

/*!
 * @brief �R���X�g���N�^
 * @param frameCount �o�b�N�o�b�t�@��
 */
class TextRenderer {
 public:
  /*!
   * @brief �R���X�g���N�^
   * @param frameCount �o�b�N�o�b�t�@��
   */
  TextRenderer(std::size_t frameCount);

  /*!
   * @brief �f�X�g���N�^
   */
  ~TextRenderer();

  /*!
   * @brief ������
   * @param hWnd �E�B���h�E�n���h��
   * @param device �f�o�C�X�I�u�W�F�N�g
   */
  void Initialize(HWND hWnd, DX::DeviceResources* deviceResources);
  void Initialize(HWND hWnd, ID3D12Device* device, int backBufferCount,
	  ID3D12CommandQueue* commandQueue);

  /*!
   * @brief �u���V�I�u�W�F�N�g�̍쐬
   * @param name �u���V�̖��O
   * @param color �u���V�J���[
   */
  void CreateColorBrush(const std::string& brushName, D2D1::ColorF::Enum color);

  /*!
   * @brief �e�L�X�g�t�H�[�}�b�g�̍쐬
   * @param name �t�H�[�}�b�g�̖��O
   * @param fontName �t�H���g��
   * @param fontSize �t�H���g�T�C�Y
   * @param fontWeight �t�H���g�̑���
   * @param fontName �t�H���g�X�^�C��
   * @param fontStretch �t�H���g�̈������΂��x����
   */
  void CreateTextFormat(
      const std::string& formatName, const std::wstring& fontName,
      float fontSize = 20.0f,
      DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL,
      DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH fontStretch = DWRITE_FONT_STRETCH_NORMAL);

  /*!
   * @brief �`��e�L�X�g�̒ǉ�
   * @param text ������
   * @param x �`��x���W
   * @param y �`�悷��y���W
   * @param formatName �g�p����t�H�[�}�b�g�̖��O
   * @param brushName �g�p����u���V�̖��O
   */
  void AddDrawText(const std::wstring& text, float x, float y,
                   const std::string& formatName, const std::string& brushName);

  /*!
   * @brief �`��e�L�X�g�̒ǉ�, �F�E�t�H�[�}�b�g�̓f�t�H���g���g��
   * @param text ������
   * @param x �`��x���W
   * @param y �`�悷��y���W
   */
  void AddDrawText(const std::wstring& text, float x, float y);

  /*!
   * @brief �`��e�L�X�g�̒ǉ�, �u���V�̓f�t�H���g���g��
   * @param text ������
   * @param x �`��x���W
   * @param y �`�悷��y���W
   * @param formatName �g�p����t�H�[�}�b�g�̖��O
   */
  void AddDrawTextWithFormat(const std::wstring& text, float x, float y,
                             const std::string& formatName);

  /*!
   * @brief �`��e�L�X�g�̒ǉ�, �t�H�[�}�b�g�̓f�t�H���g���g��
   * @param text ������
   * @param x �`��x���W
   * @param y �`�悷��y���W
   * @param brushName �g�p����u���V�̖��O
   */
  void AddDrawTextWithBrush(const std::wstring& text, float x, float y,
                            const std::string& brushName);

  /*!
   * @brief �e�L�X�g���C�A�E�g�I�u�W�F�N�g�̍쐬
   * @param layoutName �e�L�X�g���C�A�E�g�̖��O
   * @param text ������
   * @param maxWidth ���C�A�E�g�̕�
   * @param maxHeight ���C�A�E�g�̍���
   * @param formatName ���C�A�E�g�ɓK�p����e�L�X�g�t�H�[�}�b�g
   * @details TextLayout�͕`��f�[�^���v�Z�ς݂̏�Ԃŕۑ�����
   *          ���̂��ߖ��񓯂��e�L�X�g�Ȃ烌�C�A�E�g�̂ق��������ɂȂ�
   */
  void CreateTextLayout(const std::string& layoutName, const std::wstring& text,
                        float maxWidth, float maxHeight,
                        const std::string& formatName);

  /*!
   * @brief �e�L�X�g���C�A�E�g�I�u�W�F�N�g�̍쐬
   *        �f�t�H���g�̃e�L�X�g�t�H�[�}�b�g���g��
   * @param layoutName �e�L�X�g���C�A�E�g�̖��O
   * @param text ������
   * @param maxWidth ���C�A�E�g�̕�
   * @param maxHeight ���C�A�E�g�̍���
   */
  void CreateTextLayoutWithDefaultFormat(const std::string& layoutName,
                                         const std::wstring& text,
                                         float maxWidth, float maxHeight);

  /*!
   * @brief ���C�A�E�g�e�L�X�g�̕`�惊�N�G�X�g
   * @param text ������
   * @param x �`��x���W
   * @param y �`�悷��y���W
   * @param formatName �g�p����t�H�[�}�b�g��
   * @param brushName �g�p����u���V��
   */
  void AddDrawTextLayoutWithBrush(const std::string& layoutName, float x,
                                  float y, const std::string& brushName);

  /*!
   * @brief ���C�A�E�g�e�L�X�g�̕`�惊�N�G�X�g, �u���V�̓f�t�H���g���g��
   * @param x �`��x���W
   * @param y �`�悷��y���W
   */
  void AddDrawTextLayout(const std::string& layoutName, float x, float y);

  /*!
   * @brief �����̃o�b�t�@�̂��|���B�t���[���̊J�n���ɌĂяo��
   */
  void Clear();

  /*!
   * @brief �`��
   */
  void Render();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
