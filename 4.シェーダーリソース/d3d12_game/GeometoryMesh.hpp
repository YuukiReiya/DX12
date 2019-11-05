#pragma once
#include "VertexType.hpp"
namespace dxapp {
	class GeometoryMesh
	{
	public:
		GeometoryMesh(const GeometoryMesh&) = delete;
		GeometoryMesh& operator=(const GeometoryMesh&) = delete;
		~GeometoryMesh();
		/*!
			@brief	�`��R�}���h
		*/
		void Draw(ID3D12GraphicsCommandList* commandList);
		void Teardown();
		static std::unique_ptr<GeometoryMesh>CreateCube(ID3D12Device* device, float size = 1.0f);
		static std::unique_ptr<GeometoryMesh>CreateBox(ID3D12Device* device, float width = 1.0f, float height = 1.0f, float depth = 1.0f);
	
	private:
		GeometoryMesh();

		// pImpl�p�^�[��
		// �����������B�؂�����
		class Impl;                   // �����N���X��錾
		std::unique_ptr<Impl> m_Impl;  // ������GeometoryMesh�̎��ԂɂȂ�B
	}; 

}//namespace dxapp