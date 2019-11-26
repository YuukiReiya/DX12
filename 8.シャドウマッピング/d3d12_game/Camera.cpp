#include "Camera.hpp"

using namespace DirectX;

FpsCamera::FpsCamera() { 
  SetLens(16.0f / 9.0f, DirectX::XM_PIDIV4, 0.01f, 1000.0f);
}

void FpsCamera::UpdateViewMatrix() {

  // 視線ベクトルを正規化
  auto l = XMLoadFloat3(&look_);
  l = XMVector3Normalize(l);

  auto r = XMLoadFloat3(&right_);
  auto u = XMLoadFloat3(&up_);
  // 視線と右ベクトルから上ベクトルを作る
  u = XMVector3Normalize(XMVector3Cross(l, r));

  // 上と視線ベクトルから正規化された右ベクトルを作る
  r = XMVector3Cross(u, l);

  auto p = XMLoadFloat3(&position_);
  float x = -XMVectorGetX(XMVector3Dot(p, r));
  float y = -XMVectorGetX(XMVector3Dot(p, u));
  float z = -XMVectorGetX(XMVector3Dot(p, l));

  XMStoreFloat3(&right_, r);
  XMStoreFloat3(&up_, u);
  XMStoreFloat3(&look_, l);

  XMFLOAT4X4 mat{};
  mat.m[0][0] = right_.x;
  mat.m[1][0] = right_.y;
  mat.m[2][0] = right_.z;
  mat.m[3][0] = x;

  mat.m[0][1] = up_.x;
  mat.m[1][1] = up_.y;
  mat.m[2][1] = up_.z;
  mat.m[3][1] = y;

  mat.m[0][2] = look_.x;
  mat.m[1][2] = look_.y;
  mat.m[2][2] = look_.z;
  mat.m[3][2] = z;

  mat.m[0][3] = 0;
  mat.m[1][3] = 0;
  mat.m[2][3] = 0;
  mat.m[3][3] = 1;

  view_ = XMLoadFloat4x4(&mat);

  // 計算したので汚れがなくなった
  isDirty_ = false;
}

void FpsCamera::SetLens(float aspect, float fov, float nz, float fz) {
  aspect_ = aspect;
  fov_ = fov;
  nearZ_ = nz;
  farZ_ = fz;

  proj_ = XMMatrixPerspectiveFovLH(fov_, aspect_, nearZ_, farZ_);
}

void FpsCamera::LookAt(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 target,
                       DirectX::XMFLOAT3 up) {
  position_ = pos;

  XMVECTOR p = XMLoadFloat3(&pos);
  XMVECTOR t = XMLoadFloat3(&target);
  XMVECTOR u = XMLoadFloat3(&up);

  // 視線ベクトル
  XMVECTOR l = XMVector3Normalize(XMVectorSubtract(t, p));
  XMStoreFloat3(&look_, l);

  // 右ベクトル
  XMVECTOR r = XMVector3Normalize(XMVector3Cross(u, l));
  XMStoreFloat3(&right_, r);

  // 上ベクトル
  // 視線と右ベクトルの直行ベクトルが上になる
  u = XMVector3Cross(l, r);
  XMStoreFloat3(&up_, u);

  isDirty_ = true;
}

void FpsCamera::Truck(float value) {
  XMVECTOR v = XMVectorReplicate(value);
  XMVECTOR r = XMLoadFloat3(&right_);
  XMVECTOR p = XMLoadFloat3(&position_);

  // 右ベクトルに(r)に移動量(v)を掛け算、結果をカメラ座標(p)加える
  // 計算結果が新しいカメラ座標となる
  XMStoreFloat3(&position_, XMVectorMultiplyAdd(v, r, p));

  isDirty_ = true;
}

void FpsCamera::Dolly(float value) {
  auto v = XMVectorReplicate(value);
  auto l = XMLoadFloat3(&look_);
  auto p = XMLoadFloat3(&position_);

  // 視線方向(l)に移動量(v)を掛け算、結果をカメラ座標(p)加える
  // 計算結果が新しいカメラ座標となる
  XMStoreFloat3(&position_, XMVectorMultiplyAdd(v, l, p));

  isDirty_ = true;
}

void FpsCamera::Boom(float value) {
  auto v = XMVectorReplicate(value);
  auto u = XMLoadFloat3(&up_);
  auto p = XMLoadFloat3(&position_);

  // 上方向(u)に移動量(v)を掛け算、結果をカメラ座標(p)加える
  // 計算結果が新しいカメラ座標となる
  XMStoreFloat3(&position_, XMVectorMultiplyAdd(v, u, p));

  isDirty_ = true;
}

void FpsCamera::Tilt(float angle) {
  // 右ベクトルを軸にしてangleで回転行列をつくる
  XMMATRIX r = XMMatrixRotationAxis(XMLoadFloat3(&right_), angle);

  // 回転行列をもとに上ベクトルを計算
  XMStoreFloat3(&up_, XMVector3TransformNormal(XMLoadFloat3(&up_), r));
  // 回転行列をもとに視線ベクトルを計算
  XMStoreFloat3(&look_, XMVector3TransformNormal(XMLoadFloat3(&look_), r));

  isDirty_ = true;
}

void FpsCamera::Pan(float angle) {
  // Y軸回転行列を作る
  XMMATRIX r = XMMatrixRotationY(angle);

   // 回転行列をもとに右ベクトルを計算
  XMStoreFloat3(&right_, XMVector3TransformNormal(XMLoadFloat3(&right_), r));
  // 回転行列をもとに上ベクトルを計算
  XMStoreFloat3(&up_, XMVector3TransformNormal(XMLoadFloat3(&up_), r));
  // 回転行列をもとに視線ベクトルを計算
  XMStoreFloat3(&look_, XMVector3TransformNormal(XMLoadFloat3(&look_), r));

  isDirty_ = true;
}

DirectX::XMMATRIX FpsCamera::view() const {
  // 再計算せずに呼ばれたら止めちゃう
  assert(isDirty_ == false);
  return view_;
}

DirectX::XMMATRIX FpsCamera::proj() const { return proj_; }

DirectX::XMFLOAT3 FpsCamera::position() const { return position_; }
