#pragma once
/*
 * @brief FPS風の移動ができるカメラ
 */
class FpsCamera {
 public:
  /*
   * @brief コンストラクタ
   */
  FpsCamera();

  /*
   * @brief デストラクタ
   */
  ~FpsCamera() = default;

  /*
   * @brief ビュー行列
   */
  void UpdateViewMatrix();

  /*
   * @brief レンズ(パースペクティブ)の設定
   */
  void SetLens(float aspect, float fov, float nz, float fz);
  
  /*
   * @brief 視線ベクトルを作る
   */
  void LookAt(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 target,
              DirectX::XMFLOAT3 up);

  /*
   * @brief 視線方向に対しての左右移動
   */
  void Truck(float value);

  /*
   * @brief 視線方向に対しての前後移動
   */
  void Dolly(float value);

  /*
   * @brief 上下移動
   */
  void Boom(float value);

  /*
   * @brief X軸回転
   */
  void Tilt(float angle);

  /*
   * @brief Y軸回転
   */
  void Pan(float angle);

  /*
   * @brief ビュー行列取得
   */
  DirectX::XMMATRIX view() const;

  /*
   * @brief 射影行列取得
   */
  DirectX::XMMATRIX proj() const;

  /*
   * @brief 射影行列取得
   */
  DirectX::XMFLOAT3 position() const;

 private:
  DirectX::XMFLOAT3 position_{0.0f, 0.0f, 0.0f};  //!< 位置
  DirectX::XMFLOAT3 look_{0.0f, 0.0f, 1.0f};      //!< 視線
  DirectX::XMFLOAT3 right_{1.0f, 0.0f, 0.0f};  //!< 視線ベクトルの右方向
  DirectX::XMFLOAT3 up_{0.0f, 1.0f, 0.0f};  //!< カメラの上ベクトル(傾き)

  float aspect_;  //!< 画面縦横比
  float fov_;     //!< 視野角
  float nearZ_;  //!< Zのニアクリップ(これより近いと描画しない)
  float farZ_;  //!< Zのファークリップ(これより遠いと描画しなし)

  DirectX::XMMATRIX view_;  //!< ビュー行列
  DirectX::XMMATRIX proj_;  //!< 射影行列
  bool isDirty_{true};  //!< 再計算フラグ,Dirtyは汚れたという意味
};
