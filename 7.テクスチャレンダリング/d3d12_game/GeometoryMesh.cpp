#include "GeometoryMesh.hpp"
#include "BufferObject.hpp"

namespace {
// DirectXTK12 から移植
using namespace DirectX;
using IndexCollection = std::vector<std::uint32_t>;
using VertexCollection = std::vector<dxapp::VertexPositionColorNormalTexture>;

#include "External/TeapotData.inc"
#include "External/Bezier.h"

inline void CheckIndexOverflow(size_t value) {
  if (value >= USHRT_MAX)
    throw std::exception(
        "Index value out of range: cannot tesselate primitive so finely");
}

inline void index_push_back(IndexCollection& indices, size_t value) {
  CheckIndexOverflow(value);
  indices.push_back(static_cast<uint16_t>(value));
}

// 左手座標系の時に逆巻きする
inline void ReverseWinding(IndexCollection& indices,
                           VertexCollection& vertices) {
  assert((indices.size() % 3) == 0);
  for (auto it = indices.begin(); it != indices.end(); it += 3) {
    std::swap(*it, *(it + 2));
  }

  for (auto it = vertices.begin(); it != vertices.end(); ++it) {
    it->uv.x = (1.f - it->uv.x);
  }
}

inline void InvertNormals(VertexCollection& vertices) {
  for (auto it = vertices.begin(); it != vertices.end(); ++it) {
    it->normal.x = -it->normal.x;
    it->normal.y = -it->normal.y;
    it->normal.z = -it->normal.z;
  }
}

// テセレーション数に応じたメッシュを構築
void 
TessellatePatch(VertexCollection& vertices,
                                 std::vector<std::uint32_t>& indices,
                                 TeapotPatch const& patch, size_t tessellation,
                     FXMVECTOR scale, const XMFLOAT4& color, bool isMirrored) {
  // Look up the 16 control points for this patch.
  XMVECTOR controlPoints[16];

  // ベジエ曲線を使って頂点を計算で求める
  // コントロールポイントはベジエ曲線の制御に使う座標のこと
  for (int i = 0; i < 16; i++) {
    controlPoints[i] =
        XMVectorMultiply(TeapotControlPoints[patch.indices[i]], scale);
  }

  // Create the index data.
  size_t vbase = vertices.size();
  Bezier::CreatePatchIndices(tessellation, isMirrored, [&](size_t index) {
    index_push_back(indices, vbase + index);
  });

  // Create the vertex data.
  Bezier::CreatePatchVertices(
      controlPoints, tessellation, isMirrored,
      [&](FXMVECTOR position, FXMVECTOR normal, FXMVECTOR textureCoordinate) {
        XMFLOAT3 pos{}, nml{};
        XMFLOAT2 uv{};
        XMStoreFloat3(&pos, position);
        XMStoreFloat3(&nml, normal);
        XMStoreFloat2(&uv, textureCoordinate);
        vertices.push_back(dxapp::VertexPositionColorNormalTexture{pos, color, nml, uv});
      });
}
}  // namespace

// Creates a teapot primitive.
void ComputeTeapot(VertexCollection& vertices, IndexCollection& indices,
                   float size, size_t tessellation,
                   XMFLOAT4 color, bool rhcoords) {
  vertices.clear();
  indices.clear();

  if (tessellation < 1)
    throw std::out_of_range("tesselation parameter out of range");

  XMVECTOR scaleVector = XMVectorReplicate(size);

  XMVECTOR scaleNegateX = XMVectorMultiply(scaleVector, g_XMNegateX);
  XMVECTOR scaleNegateZ = XMVectorMultiply(scaleVector, g_XMNegateZ);
  XMVECTOR scaleNegateXZ =
      XMVectorMultiply(scaleVector, XMVectorMultiply(g_XMNegateX, g_XMNegateZ));

  for (size_t i = 0; i < _countof(TeapotPatches); i++) {
    TeapotPatch const& patch = TeapotPatches[i];

    // Because the teapot is symmetrical from left to right, we only store
    // data for one side, then tessellate each patch twice, mirroring in X.
    TessellatePatch(vertices, indices, patch, tessellation, scaleVector, color, false);
    TessellatePatch(vertices, indices, patch, tessellation, scaleNegateX, color, true);

    if (patch.mirrorZ) {
      // Some parts of the teapot (the body, lid, and rim, but not the
      // handle or spout) are also symmetrical from front to back, so
      // we tessellate them four times, mirroring in Z as well as X.
      TessellatePatch(vertices, indices, patch, tessellation, scaleNegateZ,
                      color, true);
      TessellatePatch(vertices, indices, patch, tessellation, scaleNegateXZ,
                      color, false);
    }
  }

  // Built RH above
  if (!rhcoords) ReverseWinding(indices, vertices);
}

namespace dxapp {
// 頂点の名前が長い...
using Vpcnt = VertexPositionColorNormalTexture;

// 内部クラスの実装
class GeometoryMesh::Impl {
  // GeometoryMeshからしか見えないので全部public
 public:
  // 頂点・インデックスバッファ
  // BufferObjectをつかう
  BufferObject vb_{};
  BufferObject ib_{};
  UINT indexCount_{};  // インデクス数

  // 頂点バッファ・インデックスバッファビュー
  D3D12_VERTEX_BUFFER_VIEW vbView_{};
  D3D12_INDEX_BUFFER_VIEW ibView_{};

  // 頂点タイプは固定なのでサイズも固定してしまった
  static constexpr std::size_t vertexStride_{sizeof(Vpcnt)};
  static constexpr std::size_t indexStride_{sizeof(std::uint32_t)};

  /*!
   * @brief 初期化
   * @param[in] device d3d12デバイス
   * @param[in] vertices 頂点配列
   * @param[in] indices インデックス配列
   */
  void Initialize(ID3D12Device* device, const std::vector<Vpcnt>& vertices,
                  const std::vector<std::uint32_t>& indices);
};

void GeometoryMesh::Impl::Initialize(
    ID3D12Device* device, const std::vector<Vpcnt>& vertices,
    const std::vector<std::uint32_t>& indices) {
  // 頂点バッファとビューの作成
  auto size = vertexStride_ * vertices.size();
  vb_.Initialize(device, BufferObjectType::VertexBuffer, size);
  vb_.Update(vertices.data(), size);

  vbView_.BufferLocation = vb_.resource()->GetGPUVirtualAddress();
  vbView_.SizeInBytes = static_cast<UINT>(size);
  vbView_.StrideInBytes = vertexStride_;

  // インデックスバッファとビューの作成
  indexCount_ = static_cast<UINT>(indices.size());
  size = indexStride_ * indices.size();
  ib_.Initialize(device, BufferObjectType::IndexBuffer,
                 indexStride_ * indices.size());
  ib_.Update(indices.data(), size);

  ibView_.BufferLocation = ib_.resource()->GetGPUVirtualAddress();
  ibView_.SizeInBytes = static_cast<UINT>(size);
  ibView_.Format = DXGI_FORMAT_R32_UINT;
}

//-------------------------------------------------------------------
// GeometoryMesh
//-------------------------------------------------------------------
GeometoryMesh::GeometoryMesh() : impl_(std::make_unique<Impl>()) {}

GeometoryMesh::~GeometoryMesh() {
  { Terminate(); }
}

void GeometoryMesh::Draw(ID3D12GraphicsCommandList* commandList) {
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->IASetVertexBuffers(0, 1, &impl_->vbView_);
  commandList->IASetIndexBuffer(&impl_->ibView_);
  commandList->DrawIndexedInstanced(impl_->indexCount_, 1, 0, 0, 0);
}

void GeometoryMesh::Terminate() {}

std::unique_ptr<GeometoryMesh> GeometoryMesh::CreateCube(
    ID3D12Device* device, float size, DirectX::XMFLOAT4 color) {
  // 辺の長さが同一のBoxを作る
  return CreateBox(device, size, size, size, color);
};

// 長くてめんどい、間違えるとメッシュが描画できないのでコピペ推奨
std::unique_ptr<GeometoryMesh> GeometoryMesh::CreateBox(
    ID3D12Device* device, float width, float height, float depth,
    DirectX::XMFLOAT4 color) {
  auto w = width * 0.5f;
  auto h = height * 0.5f;
  auto d = depth * 0.5f;

  std::vector<Vpcnt> v(24);
  // front
  v[0] = Vpcnt{{-w, -h, -d}, color, {0, 0, -1}, {0, 1}};
  v[1] = Vpcnt{{-w, +h, -d}, color, {0, 0, -1}, {0, 0}};
  v[2] = Vpcnt{{+w, +h, -d}, color, {0, 0, -1}, {1, 0}};
  v[3] = Vpcnt{{+w, -h, -d}, color, {0, 0, -1}, {1, 1}};
  // back
  v[0 + 4] = Vpcnt{{-w, -h, +d}, color, {0, 0, 1}, {1, 1}};
  v[1 + 4] = Vpcnt{{+w, -h, +d}, color, {0, 0, 1}, {0, 1}};
  v[2 + 4] = Vpcnt{{+w, +h, +d}, color, {0, 0, 1}, {0, 0}};
  v[3 + 4] = Vpcnt{{-w, +h, +d}, color, {0, 0, 1}, {1, 0}};
  // top
  v[0 + 8] = Vpcnt{{-w, +h, -d}, color, {0, 1, 0}, {0, 1}};
  v[1 + 8] = Vpcnt{{-w, +h, +d}, color, {0, 1, 0}, {0, 0}};
  v[2 + 8] = Vpcnt{{+w, +h, +d}, color, {0, 1, 0}, {1, 0}};
  v[3 + 8] = Vpcnt{{+w, +h, -d}, color, {0, 1, 0}, {1, 1}};
  // bottom
  v[0 + 12] = Vpcnt{{-w, -h, -d}, color, {0, -1, 0}, {1, 1}};
  v[1 + 12] = Vpcnt{{+w, -h, -d}, color, {0, -1, 0}, {0, 1}};
  v[2 + 12] = Vpcnt{{+w, -h, +d}, color, {0, -1, 0}, {0, 0}};
  v[3 + 12] = Vpcnt{{-w, -h, +d}, color, {0, -1, 0}, {1, 0}};
  // left
  v[0 + 16] = Vpcnt{{-w, -h, +d}, color, {-1, 0, 0}, {0, 1}};
  v[1 + 16] = Vpcnt{{-w, +h, +d}, color, {-1, 0, 0}, {0, 0}};
  v[2 + 16] = Vpcnt{{-w, +h, -d}, color, {-1, 0, 0}, {1, 0}};
  v[3 + 16] = Vpcnt{{-w, -h, -d}, color, {-1, 0, 0}, {1, 1}};
  // right
  v[0 + 20] = Vpcnt{{+w, -h, -d}, color, {1, 0, 0}, {0, 1}};
  v[1 + 20] = Vpcnt{{+w, +h, -d}, color, {1, 0, 0}, {0, 0}};
  v[2 + 20] = Vpcnt{{+w, +h, +d}, color, {1, 0, 0}, {1, 0}};
  v[3 + 20] = Vpcnt{{+w, -h, +d}, color, {1, 0, 0}, {1, 1}};

  // インデクス
  std::vector<std::uint32_t> i(36);
  // front
  i[0] = 0;
  i[1] = 1;
  i[2] = 2;
  i[3] = 0;
  i[4] = 2;
  i[5] = 3;

  // back
  i[6] = 4;
  i[7] = 5;
  i[8] = 6;
  i[9] = 4;
  i[10] = 6;
  i[11] = 7;

  // top
  i[12] = 8;
  i[13] = 9;
  i[14] = 10;
  i[15] = 8;
  i[16] = 10;
  i[17] = 11;

  // bottom
  i[18] = 12;
  i[19] = 13;
  i[20] = 14;
  i[21] = 12;
  i[22] = 14;
  i[23] = 15;

  // left
  i[24] = 16;
  i[25] = 17;
  i[26] = 18;
  i[27] = 16;
  i[28] = 18;
  i[29] = 19;

  // right
  i[30] = 20;
  i[31] = 21;
  i[32] = 22;
  i[33] = 20;
  i[34] = 22;
  i[35] = 23;

  // GeometoryMeshがGeometoryMeshをnewする
  // オブジェクト構築にはこういうやり方もあります
  std::unique_ptr<GeometoryMesh> mesh(new GeometoryMesh());
  // 頂点・インデクスでメッシュを作る
  mesh->impl_->Initialize(device, v, i);

  return mesh;
};

std::unique_ptr<GeometoryMesh> GeometoryMesh::CreateSphere(
    ID3D12Device* device, float radius, std::uint32_t sliceCount,
    std::uint32_t stackCount, DirectX::XMFLOAT4 color) {
  // スフィアの極(上下の端)の頂点
  auto topVertex = Vpcnt{{0, +radius, 0}, color, {0, 0, -1}, {0, 1}};
  auto bottomVertex = Vpcnt{{0, -radius, 0}, color, {0, 0, -1}, {0, 1}};

  // πを分割して頂点分割に使う
  float phiStep = DirectX::XM_PI / stackCount;
  float thetaStep = 2.0f * DirectX::XM_PI / sliceCount;

  std::vector<Vpcnt> vertices;
  std::vector<std::uint32_t> indices;

  vertices.emplace_back(topVertex);
  for (std::uint32_t i = 1; i <= stackCount - 1; ++i) {
    auto phi = i * phiStep;
    for (std::uint32_t j = 0; j <= sliceCount; ++j) {
      auto theta = j * thetaStep;

      Vpcnt v;
      v.position.x = radius * sinf(phi) * cosf(theta);
      v.position.y = radius * cosf(phi);
      v.position.z = radius * sinf(phi) * sinf(theta);

      v.color = color;

      DirectX::XMFLOAT3 tangent;
      tangent.x = -radius * sinf(phi) * sinf(theta);
      tangent.y = 0.0f;
      tangent.z = +radius * sinf(phi) * cosf(theta);
      DirectX::XMVECTOR tan = XMLoadFloat3(&tangent);
      DirectX::XMStoreFloat3(&tangent, DirectX::XMVector3Normalize(tan));

      auto p = DirectX::XMLoadFloat3(&v.position);
      DirectX::XMStoreFloat3(&v.normal, DirectX::XMVector3Normalize(p));

      v.uv.x = theta / DirectX::XM_2PI;
      v.uv.y = phi / DirectX::XM_PI;

      vertices.emplace_back(v);
    }
  }
  vertices.emplace_back(bottomVertex);

  // 最上段のインデックス構築
  for (std::uint32_t i = 1; i <= sliceCount; ++i) {
    indices.emplace_back(0);
    indices.emplace_back(i + 1);
    indices.emplace_back(i);
  }

  // 最上段のインデックスは作成済み。その次の段から始める
  std::uint32_t baseIndex = 1;
  std::uint32_t ringVertexCount = sliceCount + 1;
  for (std::uint32_t i = 0; i < stackCount - 2; ++i) {
    for (std::uint32_t j = 0; j < sliceCount; ++j) {
      indices.emplace_back(baseIndex + i * ringVertexCount + j);
      indices.emplace_back(baseIndex + i * ringVertexCount + j + 1);
      indices.emplace_back(baseIndex + (i + 1) * ringVertexCount + j);

      indices.emplace_back(baseIndex + (i + 1) * ringVertexCount + j);
      indices.emplace_back(baseIndex + i * ringVertexCount + j + 1);
      indices.emplace_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
    }
  }

  // 最下段のインデックス構築
  std::uint32_t southPoleIndex =
      static_cast<std::uint32_t>(vertices.size()) - 1;
  baseIndex = southPoleIndex - ringVertexCount;
  for (std::uint32_t i = 0; i < sliceCount; ++i) {
    indices.emplace_back(southPoleIndex);
    indices.emplace_back(baseIndex + i);
    indices.emplace_back(baseIndex + i + 1);
  }

  std::unique_ptr<GeometoryMesh> mesh(new GeometoryMesh());
  mesh->impl_->Initialize(device, vertices, indices);
  return mesh;
};

//
// DirectXTK12 から移植
// ティーポットはCGのチェックによく用いられるモデルのこと
// ユタ・ティーポットでググろう
//
std::unique_ptr<GeometoryMesh> GeometoryMesh::CreateTeapot(
    ID3D12Device* device, float size, std::size_t tessellation,
    DirectX::XMFLOAT4 color) {

  std::vector<Vpcnt> vertices{};
  std::vector<std::uint32_t> indices{};
  ComputeTeapot(vertices, indices, size, tessellation, color, false);

  std::unique_ptr<GeometoryMesh> mesh(new GeometoryMesh());
  mesh->impl_->Initialize(device, vertices, indices);
  return mesh;
}
}  // namespace dxapp
