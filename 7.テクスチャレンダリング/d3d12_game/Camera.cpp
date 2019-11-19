#include "Camera.hpp"

using namespace DirectX;

FpsCamera::FpsCamera() { 
  SetLens(16.0f / 9.0f, DirectX::XM_PIDIV4, 0.01f, 1000.0f);
}

void FpsCamera::UpdateViewMatrix() {

  // �����x�N�g���𐳋K��
  auto l = XMLoadFloat3(&look_);
  l = XMVector3Normalize(l);

  auto r = XMLoadFloat3(&right_);
  auto u = XMLoadFloat3(&up_);
  // �����ƉE�x�N�g�������x�N�g�������
  u = XMVector3Normalize(XMVector3Cross(l, r));

  // ��Ǝ����x�N�g�����琳�K�����ꂽ�E�x�N�g�������
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

  // �v�Z�����̂ŉ��ꂪ�Ȃ��Ȃ���
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

  // �����x�N�g��
  XMVECTOR l = XMVector3Normalize(XMVectorSubtract(t, p));
  XMStoreFloat3(&look_, l);

  // �E�x�N�g��
  XMVECTOR r = XMVector3Normalize(XMVector3Cross(u, l));
  XMStoreFloat3(&right_, r);

  // ��x�N�g��
  // �����ƉE�x�N�g���̒��s�x�N�g������ɂȂ�
  u = XMVector3Cross(l, r);
  XMStoreFloat3(&up_, u);

  isDirty_ = true;
}

void FpsCamera::Truck(float value) {
  XMVECTOR v = XMVectorReplicate(value);
  XMVECTOR r = XMLoadFloat3(&right_);
  XMVECTOR p = XMLoadFloat3(&position_);

  // �E�x�N�g����(r)�Ɉړ���(v)���|���Z�A���ʂ��J�������W(p)������
  // �v�Z���ʂ��V�����J�������W�ƂȂ�
  XMStoreFloat3(&position_, XMVectorMultiplyAdd(v, r, p));

  isDirty_ = true;
}

void FpsCamera::Dolly(float value) {
  auto v = XMVectorReplicate(value);
  auto l = XMLoadFloat3(&look_);
  auto p = XMLoadFloat3(&position_);

  // ��������(l)�Ɉړ���(v)���|���Z�A���ʂ��J�������W(p)������
  // �v�Z���ʂ��V�����J�������W�ƂȂ�
  XMStoreFloat3(&position_, XMVectorMultiplyAdd(v, l, p));

  isDirty_ = true;
}

void FpsCamera::Boom(float value) {
  auto v = XMVectorReplicate(value);
  auto u = XMLoadFloat3(&up_);
  auto p = XMLoadFloat3(&position_);

  // �����(u)�Ɉړ���(v)���|���Z�A���ʂ��J�������W(p)������
  // �v�Z���ʂ��V�����J�������W�ƂȂ�
  XMStoreFloat3(&position_, XMVectorMultiplyAdd(v, u, p));

  isDirty_ = true;
}

void FpsCamera::Tilt(float angle) {
  // �E�x�N�g�������ɂ���angle�ŉ�]�s�������
  XMMATRIX r = XMMatrixRotationAxis(XMLoadFloat3(&right_), angle);

  // ��]�s������Ƃɏ�x�N�g�����v�Z
  XMStoreFloat3(&up_, XMVector3TransformNormal(XMLoadFloat3(&up_), r));
  // ��]�s������ƂɎ����x�N�g�����v�Z
  XMStoreFloat3(&look_, XMVector3TransformNormal(XMLoadFloat3(&look_), r));

  isDirty_ = true;
}

void FpsCamera::Pan(float angle) {
  // Y����]�s������
  XMMATRIX r = XMMatrixRotationY(angle);

   // ��]�s������ƂɉE�x�N�g�����v�Z
  XMStoreFloat3(&right_, XMVector3TransformNormal(XMLoadFloat3(&right_), r));
  // ��]�s������Ƃɏ�x�N�g�����v�Z
  XMStoreFloat3(&up_, XMVector3TransformNormal(XMLoadFloat3(&up_), r));
  // ��]�s������ƂɎ����x�N�g�����v�Z
  XMStoreFloat3(&look_, XMVector3TransformNormal(XMLoadFloat3(&look_), r));

  isDirty_ = true;
}

DirectX::XMMATRIX FpsCamera::view() const {
  // �Čv�Z�����ɌĂ΂ꂽ��~�߂��Ⴄ
  assert(isDirty_ == false);
  return view_;
}

DirectX::XMMATRIX FpsCamera::proj() const { return proj_; }

DirectX::XMFLOAT3 FpsCamera::position() const { return position_; }
