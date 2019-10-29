#include "pch.h"
#include "BasicShader.hpp"
#include "BufferObject.hpp"
#include "Utility.hpp"
#include "VertexType.hpp"

using namespace dxapp;

#pragma region Impl
class BasicShader::Impl {
public:
	/*!
	 * @brief �R���X�g���N�^
	 */
	Impl() = default;
	/*!
	 * @brief �f�X�g���N�^
	 */
	~Impl() = default;

	/*!
	 * @brief �V�F�[�_�[����
	 */
	void CreateShader();

	/*!
	 * @brief ���[�g�V�O�l�`������
	 */
	void CreatRootSignature(ID3D12Device* device);

	/*!
	 * @brief �p�C�v���C������
	 */
	void CreatePipelineState(ID3D12Device* device);

	Microsoft::WRL::ComPtr<ID3DBlob> vs_{};
	Microsoft::WRL::ComPtr<ID3DBlob> ps_{};
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_{};
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_{};

	// �f�X�N���v�^�T�C�Y�̃L���b�V��
	UINT srvDescriptorSize_{}, samplerDescriptorSize_{};

	// Begin/End�Ȃ��Ŏg���R�}���h���X�g
	ID3D12GraphicsCommandList* commandList{};

	// �R�}���h���X�g�ςݍ��݂Ɏg���f�X�N���v�^�̃L���b�V��
	// �Ȃ񂩂���������ƃX�}�[�g�ɂ�肽��
	ID3D12DescriptorHeap* cbvSrvHeap_{}, * samplerHeap_{};
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbv_, srv_, sampler_;
};

void BasicShader::Impl::CreateShader() {
	{
		Microsoft::WRL::ComPtr<ID3DBlob> error;
		utility::CompileShaderFromFile(L"Shaders/BasicVS.hlsl", L"vs_6_0", vs_,
			error);
		utility::CompileShaderFromFile(L"Shaders/BasicPS.hlsl", L"ps_6_0", ps_,
			error);
	}
}

// ���[�g�V�O�l�`������
// SimplePolygon�N���X���f�R���G�B�O��̓��\�[�X���S���Ȃ������̂ŁB
void BasicShader::Impl::CreatRootSignature(ID3D12Device* device) {
	// ���\�[�X�̃f�X�N���v�^�����W�����܂�
	// �V�F�[�_�̍쐬�ł݂��悤��b0/b1 ... bn�ƃ��W�X�^���蓖�Ă��ł���
	// �����ă��W�X�^���ƂɃf�X�N���v�^���g���̂ł��̏���
	// ����͒萔�ESRV�E�T���v��1���Ȃ̂ł��܂�Y�܂Ȃ��Ă悢
	CD3DX12_DESCRIPTOR_RANGE cbv, srv, sampler;
	// �R���X�^���g�o�b�t�@(b0)�̏���
	cbv.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,  // ���\�[�X��ށF�R���X�^���g�o�b�t�@
		1,                                // b0��1��
		0);                               // �Ƃ肠����0��OK
	// �e�N�X�`��(t0)�̏���
	srv.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	// �e�N�X�`���T���v��(s0)�̏���
	sampler.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

	// ���\�[�X������Ƃ���ROOT_PARAMETER���K�v
	// ��ŏ��������f�X�N���v�^���ǂ̂悤�ɔz�u����邩�B
	// �܂��ǂ̕����������邩�Ƃ������ݒ肪�ł���
	CD3DX12_ROOT_PARAMETER rootParams[3];
	rootParams[0].InitAsDescriptorTable(
		1, &cbv,  // b0�̐ݒ�
		// ���\�[�X��������V�F�[�_�͈̔́B����͒��_�V�F�[�_����
		D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[1].InitAsDescriptorTable(
		1, &srv,                         // t0
		D3D12_SHADER_VISIBILITY_PIXEL);  // �s�N�Z���V�F�[�_���猩����
	rootParams[2].InitAsDescriptorTable(
		1, &sampler,                     // s0
		D3D12_SHADER_VISIBILITY_PIXEL);  // �s�N�Z���V�F�[�_���猩����

	// ���[�g�p�����^�����ƂɃ��[�g�V�O�l�`����"�ݒ�"�����
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc{};
	rootSigDesc.Init(
		_countof(rootParams), rootParams,  // ROOT_PARAMETER���킽��
		0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// �V���A���C�Y
	Microsoft::WRL::ComPtr<ID3DBlob> signature;
	Microsoft::WRL::ComPtr<ID3DBlob> error;
	D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
		&signature, &error);

	// ����ƃ��[�g�V�O�l�`�����ł��܂����I
	device->CreateRootSignature(0, signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature_));
}

// �p�C�v���C���X�e�[�g�I�u�W�F�N�g����
// ������SimplePolygon�ƈꏏ
void BasicShader::Impl::CreatePipelineState(ID3D12Device* device) {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	// �V�F�[�_�[�̃Z�b�g
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vs_.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(ps_.Get());
	// �u�����h�X�e�[�g�ݒ�
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	// ���X�^���C�U�[�X�e�[�g
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	// �o�͐��1�^�[�Q�b�g
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.InputLayout = { simpleVertex::c_InputLayoutElement,
						   _countof(simpleVertex::c_InputLayoutElement) };
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	// ���[�g�V�O�l�`���̃Z�b�g
	psoDesc.pRootSignature = rootSignature_.Get();
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// �}���`�T���v���ݒ�
	psoDesc.SampleDesc = { 1, 0 };
	psoDesc.SampleMask = UINT_MAX;

	// �f�v�X�o�b�t�@�̃t�H�[�}�b�g��ݒ�
	auto dsvDesc = CD3DX12_DEPTH_STENCIL_DESC();
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.DepthStencilState = dsvDesc;
	dsvDesc.DepthEnable = FALSE;

	device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline_));
}

#pragma endregion


dxapp::BasicShader::BasicShader() :impl_(new Impl) {}

dxapp::BasicShader::~BasicShader() {}

void dxapp::BasicShader::Setup(ID3D12Device* device)
{
	impl_->CreateShader();
	impl_->CreatRootSignature(device);
	impl_->CreatePipelineState(device);

	impl_->srvDescriptorSize_ = device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	impl_->samplerDescriptorSize_ = device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

void dxapp::BasicShader::Teardown()
{
}

void dxapp::BasicShader::Begin(ID3D12GraphicsCommandList* commandList)
{
	assert(impl_->commandList == nullptr);
	impl_->commandList = commandList;

	commandList->SetGraphicsRootSignature(impl_->rootSignature_.Get());
	commandList->SetPipelineState(impl_->pipeline_.Get());
}

void dxapp::BasicShader::End()
{
	assert(impl_->commandList);
	impl_->commandList = nullptr;
}

void dxapp::BasicShader::Apply()
{
	ID3D12DescriptorHeap* heaps[] = { impl_->cbvSrvHeap_, impl_->samplerHeap_ };

	// _countof�͐��z��̗v�f���𐔂��Ă����}�N��
	impl_->commandList->SetDescriptorHeaps(_countof(heaps), heaps);

	// ���[�g�p�����[�^�ƃ��\�[�X�Ɠ������тɂ���
	impl_->commandList->SetGraphicsRootDescriptorTable(
		0,             // ���[�g�p�����[�^�ł̃C���f�b�N�X
		impl_->cbv_);  // GPU�n���h��
	impl_->commandList->SetGraphicsRootDescriptorTable(1, impl_->srv_);
	impl_->commandList->SetGraphicsRootDescriptorTable(2, impl_->sampler_);
}

void dxapp::BasicShader::SetCBufferDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset)
{
	impl_->cbvSrvHeap_ = heap;
	impl_->cbv_ =
		CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->GetGPUDescriptorHandleForHeapStart(),
			offset, impl_->srvDescriptorSize_);
}

void dxapp::BasicShader::SetSrvDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset)
{
	impl_->cbvSrvHeap_ = heap;
	impl_->srv_ =
		CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->GetGPUDescriptorHandleForHeapStart(),
			offset, impl_->srvDescriptorSize_);
}

void dxapp::BasicShader::SetSamplerDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset)
{
	impl_->samplerHeap_ = heap;
	impl_->sampler_ =
		CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->GetGPUDescriptorHandleForHeapStart(),
			offset, impl_->samplerDescriptorSize_);
}
