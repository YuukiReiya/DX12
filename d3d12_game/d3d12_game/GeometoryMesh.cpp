#include "pch.h"
#include "GeometoryMesh.hpp"
#include "BufferObject.hpp"
using namespace dxapp;
using Vpcnt = dxapp::simpleVertex::Vertex;

#pragma region Impl
// �����N���X�̎���
class dxapp::GeometoryMesh::Impl {
	// GeometoryMesh���炵�������Ȃ��̂őS��public
public:
	// ���_�E�C���f�b�N�X�o�b�t�@
	// BufferObject������
	BufferObject vb_{};
	BufferObject ib_{};
	UINT indexCount_{};  // �C���f�N�X��

	// ���_�o�b�t�@�E�C���f�b�N�X�o�b�t�@�r���[
	D3D12_VERTEX_BUFFER_VIEW vbView_{};
	D3D12_INDEX_BUFFER_VIEW ibView_{};

	// ���_�^�C�v�͌Œ�Ȃ̂ŃT�C�Y���Œ肵�Ă��܂���
	static constexpr std::size_t vertexStride_{ sizeof(Vpcnt) };
	static constexpr std::size_t indexStride_{ sizeof(std::uint32_t) };

	/*!
	 * @brief ������
	 * @param[in] device d3d12�f�o�C�X
	 * @param[in] vertices ���_�z��
	 * @param[in] indices �C���f�b�N�X�z��
	 */
	void Initialize(ID3D12Device* device, const std::vector<Vpcnt>& vertices,
		const std::vector<std::uint32_t>& indices);
};
void GeometoryMesh::Impl::Initialize(
	ID3D12Device* device, const std::vector<Vpcnt>& vertices,
	const std::vector<std::uint32_t>& indices) {
	// ���_�o�b�t�@�ƃr���[�̍쐬
	auto size = vertexStride_ * vertices.size();
	vb_.Setup(device, BufferType::VertexBuffer, size);
	vb_.Update(vertices.data(), size);

	vbView_.BufferLocation = vb_.GetResouce()->GetGPUVirtualAddress();
	vbView_.SizeInBytes = static_cast<UINT>(size);
	vbView_.StrideInBytes = vertexStride_;

	// �C���f�b�N�X�o�b�t�@�ƃr���[�̍쐬
	indexCount_ = static_cast<UINT>(indices.size());
	size = indexStride_ * indices.size();
	ib_.Setup(device, BufferType::IndexBuffer,
		indexStride_ * indices.size());
	ib_.Update(indices.data(), size);

	ibView_.BufferLocation = ib_.GetResouce()->GetGPUVirtualAddress();
	ibView_.SizeInBytes = static_cast<UINT>(size);
	ibView_.Format = DXGI_FORMAT_R32_UINT;
}
#pragma endregion

dxapp::GeometoryMesh::~GeometoryMesh()
{
	Teardown();
}

void dxapp::GeometoryMesh::Draw(ID3D12GraphicsCommandList* commandList)
{
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &m_Impl->vbView_);
	commandList->IASetIndexBuffer(&m_Impl->ibView_);
	commandList->DrawIndexedInstanced(m_Impl->indexCount_, 1, 0, 0, 0);
}

void dxapp::GeometoryMesh::Teardown()
{
}

std::unique_ptr<dxapp::GeometoryMesh> dxapp::GeometoryMesh::CreateCube(ID3D12Device* device, float size)
{
	return CreateBox(device, size, size, size);
}

std::unique_ptr<dxapp::GeometoryMesh> dxapp::GeometoryMesh::CreateBox(ID3D12Device* device, float width, float height, float depth)
{
	auto w = width * 0.5f;
	auto h = height * 0.5f;
	auto d = depth * 0.5f;

	std::vector<Vpcnt>v(24);
	// front
	v[0] = Vpcnt{ {-w, -h, -d}, {1, 1, 1, 1}, {0, 0, -1}, {0, 1} };
	v[1] = Vpcnt{ {-w, +h, -d}, {1, 1, 1, 1}, {0, 0, -1}, {0, 0} };
	v[2] = Vpcnt{ {+w, +h, -d}, {1, 1, 1, 1}, {0, 0, -1}, {1, 0} };
	v[3] = Vpcnt{ {+w, -h, -d}, {1, 1, 1, 1}, {0, 0, -1}, {1, 1} };
	// back
	v[0 + 4] = Vpcnt{ {-w, -h, +d}, {1, 1, 1, 1}, {0, 0, 1}, {1, 1} };
	v[1 + 4] = Vpcnt{ {+w, -h, +d}, {1, 1, 1, 1}, {0, 0, 1}, {0, 1} };
	v[2 + 4] = Vpcnt{ {+w, +h, +d}, {1, 1, 1, 1}, {0, 0, 1}, {0, 0} };
	v[3 + 4] = Vpcnt{ {-w, +h, +d}, {1, 1, 1, 1}, {0, 0, 1}, {1, 0} };
	// top
	v[0 + 8] = Vpcnt{ {-w, +h, -d}, {1, 1, 1, 1}, {0, 1, 0}, {0, 1} };
	v[1 + 8] = Vpcnt{ {-w, +h, +d}, {1, 1, 1, 1}, {0, 1, 0}, {0, 0} };
	v[2 + 8] = Vpcnt{ {+w, +h, +d}, {1, 1, 1, 1}, {0, 1, 0}, {1, 0} };
	v[3 + 8] = Vpcnt{ {+w, +h, -d}, {1, 1, 1, 1}, {0, 1, 0}, {1, 1} };
	// bottom
	v[0 + 12] = Vpcnt{ {-w, -h, -d}, {1, 1, 1, 1}, {0, -1, 0}, {1, 1} };
	v[1 + 12] = Vpcnt{ {+w, -h, -d}, {1, 1, 1, 1}, {0, -1, 0}, {0, 1} };
	v[2 + 12] = Vpcnt{ {+w, -h, +d}, {1, 1, 1, 1}, {0, -1, 0}, {0, 0} };
	v[3 + 12] = Vpcnt{ {-w, -h, +d}, {1, 1, 1, 1}, {0, -1, 0}, {1, 0} };
	// left
	v[0 + 16] = Vpcnt{ {-w, -h, +d}, {1, 1, 1, 1}, {-1, 0, 0}, {0, 1} };
	v[1 + 16] = Vpcnt{ {-w, +h, +d}, {1, 1, 1, 1}, {-1, 0, 0}, {0, 0} };
	v[2 + 16] = Vpcnt{ {-w, +h, -d}, {1, 1, 1, 1}, {-1, 0, 0}, {1, 0} };
	v[3 + 16] = Vpcnt{ {-w, -h, -d}, {1, 1, 1, 1}, {-1, 0, 0}, {1, 1} };
	// right
	v[0 + 20] = Vpcnt{ {+w, -h, -d}, {1, 1, 1, 1}, {1, 0, 0}, {0, 1} };
	v[1 + 20] = Vpcnt{ {+w, +h, -d}, {1, 1, 1, 1}, {1, 0, 0}, {0, 0} };
	v[2 + 20] = Vpcnt{ {+w, +h, +d}, {1, 1, 1, 1}, {1, 0, 0}, {1, 0} };
	v[3 + 20] = Vpcnt{ {+w, -h, +d}, {1, 1, 1, 1}, {1, 0, 0}, {1, 1} };

	//�C���f�b�N�X
	std::vector<std::uint32_t>i(36);
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

	// GeometoryMesh��GeometoryMesh��new����
	// �I�u�W�F�N�g�\�z�ɂ͂�����������������܂�
	std::unique_ptr<GeometoryMesh> mesh(new GeometoryMesh());
	// ���_�E�C���f�N�X�Ń��b�V�������
	mesh->m_Impl->Initialize(device, v, i);
	return mesh;
}

dxapp::GeometoryMesh::GeometoryMesh() :m_Impl(std::make_unique<Impl>()) {}