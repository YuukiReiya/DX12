#pragma once
/*
 * @brief FPS���̈ړ����ł���J����
 */
class FpsCamera {
 public:
  /*
   * @brief �R���X�g���N�^
   */
  FpsCamera();

  /*
   * @brief �f�X�g���N�^
   */
  ~FpsCamera() = default;

  /*
   * @brief �r���[�s��
   */
  void UpdateViewMatrix();

  /*
   * @brief �����Y(�p�[�X�y�N�e�B�u)�̐ݒ�
   */
  void SetLens(float aspect, float fov, float nz, float fz);
  
  /*
   * @brief �����x�N�g�������
   */
  void LookAt(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 target,
              DirectX::XMFLOAT3 up);

  /*
   * @brief ���������ɑ΂��Ă̍��E�ړ�
   */
  void Truck(float value);

  /*
   * @brief ���������ɑ΂��Ă̑O��ړ�
   */
  void Dolly(float value);

  /*
   * @brief �㉺�ړ�
   */
  void Boom(float value);

  /*
   * @brief X����]
   */
  void Tilt(float angle);

  /*
   * @brief Y����]
   */
  void Pan(float angle);

  /*
   * @brief �r���[�s��擾
   */
  DirectX::XMMATRIX view() const;

  /*
   * @brief �ˉe�s��擾
   */
  DirectX::XMMATRIX proj() const;

  /*
   * @brief �ˉe�s��擾
   */
  DirectX::XMFLOAT3 position() const;

 private:
  DirectX::XMFLOAT3 position_{0.0f, 0.0f, 0.0f};  //!< �ʒu
  DirectX::XMFLOAT3 look_{0.0f, 0.0f, 1.0f};      //!< ����
  DirectX::XMFLOAT3 right_{1.0f, 0.0f, 0.0f};  //!< �����x�N�g���̉E����
  DirectX::XMFLOAT3 up_{0.0f, 1.0f, 0.0f};  //!< �J�����̏�x�N�g��(�X��)

  float aspect_;  //!< ��ʏc����
  float fov_;     //!< ����p
  float nearZ_;  //!< Z�̃j�A�N���b�v(������߂��ƕ`�悵�Ȃ�)
  float farZ_;  //!< Z�̃t�@�[�N���b�v(�����艓���ƕ`�悵�Ȃ�)

  DirectX::XMMATRIX view_;  //!< �r���[�s��
  DirectX::XMMATRIX proj_;  //!< �ˉe�s��
  bool isDirty_{true};  //!< �Čv�Z�t���O,Dirty�͉��ꂽ�Ƃ����Ӗ�
};
