#include "LightingShader.hpp"

#include "BufferObject.hpp"
#include "Device.hpp"
#include "Utility.hpp"
#include "VertexType.hpp"

namespace dxapp {
#pragma region LightingShader::Impl
	/*!
	 * @brief LightingShader�̎���
	 */
	class LightingShader::Impl {
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

		//! ���_�V�F�[�_
		Microsoft::WRL::ComPtr<ID3DBlob> vs_{};
		//! �s�N�Z���V�F�[�_
		Microsoft::WRL::ComPtr<ID3DBlob> ps_{};
		//! ���[�g�V�O�l�`��
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_{};
		//! �p�C�v���C���X�e�[�g
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_{};

		// �f�X�N���v�^�T�C�Y�̃L���b�V��
		UINT srvDescriptorSize_{}, samplerDescriptorSize_{};

		// Begin/End�Ŏg���R�}���h���X�g
		ID3D12GraphicsCommandList* commandList{};

		// �R�}���h���X�g�ɐςރf�X�N���v�^�EGPU�n���h���̃L���b�V��
		ID3D12DescriptorHeap* srvHeap_{}, * samplerHeap_{};
		CD3DX12_GPU_DESCRIPTOR_HANDLE srv_, sampler_;

		// �R�}���h���X�g�ɐςޒ萔�o�b�t�@�ւ�GPU�n���h���̃L���b�V��
		D3D12_GPU_VIRTUAL_ADDRESS objParam_, sceneParam_, matParam_;

		// �e�N�X�`���Ȃ��̎��ł��������B�����������ƃq�[�v��n���h���͓n���K�v������
		ID3D12DescriptorHeap* defaultSamplerHeap_{};
		CD3DX12_GPU_DESCRIPTOR_HANDLE defaultSampler_;

		// �e�N�X�`���Ȃ��̎��ł��������B�����������ƃq�[�v��n���h���͓n���K�v������
		ID3D12DescriptorHeap* dammySrvHeap_{};
		CD3DX12_GPU_DESCRIPTOR_HANDLE dammySrv_;
	};

	void LightingShader::Impl::CreateShader() {
		{
			Microsoft::WRL::ComPtr<ID3DBlob> error;
			utility::CompileShaderFromFile(L"Shaders/LightingVS.hlsl", L"vs_6_0", vs_,
				error);
			utility::CompileShaderFromFile(L"Shaders/LightingPS.hlsl", L"ps_6_0", ps_,
				error);
		}
	}

	// ���[�g�V�O�l�`������
	void LightingShader::Impl::CreatRootSignature(ID3D12Device* device) {
		// b0, b1, b2, t0, s1��5�̃p�����[�^���g��
		CD3DX12_ROOT_PARAMETER rootParams[5];

		//-----------------------------------------------------------------
		// BasicShader�Ƃ͈Ⴂ�萔�o�b�t�@������InitAsConstantBufferView�ō쐬�B
		// InitAsConstantBufferView��DescriptorTable���o�R������
		// ���ڃV�F�[�_�[�ɓn����̂������b�g�B
		// �������n����p�����[�^������������A�������x�I�ɂ͒x���Ȃǂ̃f�����b�g������B
		// ����͊w�K�p�Ȃ̂ŗǂ��ɂ���B
		//-----------------------------------------------------------------
		// �����̐����̓��W�X�^�ԍ�.
		rootParams[0].InitAsConstantBufferView(0);  // b0:ObjectParam
		rootParams[1].InitAsConstantBufferView(1);  // b1:SceneParam
		rootParams[2].InitAsConstantBufferView(2);  // b2:MaterialParam

		// �e�N�X�`���ƃT���v���[�͍��܂Œʂ�f�X�N���v�^�e�[�u���œn��
		CD3DX12_DESCRIPTOR_RANGE srv, sampler;
		srv.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		sampler.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

		rootParams[3].InitAsDescriptorTable(
			1, &srv,                         // t0
			D3D12_SHADER_VISIBILITY_PIXEL);  // �s�N�Z���V�F�[�_���猩����
		rootParams[4].InitAsDescriptorTable(
			1, &sampler,                     // s0
			D3D12_SHADER_VISIBILITY_PIXEL);  // �s�N�Z���V�F�[�_���猩����

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc{};
		rootSigDesc.Init(
			_countof(rootParams), rootParams, 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		Microsoft::WRL::ComPtr<ID3DBlob> signature;
		Microsoft::WRL::ComPtr<ID3DBlob> error;
		D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
			&signature, &error);

		device->CreateRootSignature(0, signature->GetBufferPointer(),
			signature->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature_));
	}

	// �p�C�v���C���X�e�[�g�I�u�W�F�N�g����
	void LightingShader::Impl::CreatePipelineState(ID3D12Device* device) {
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
		psoDesc.InputLayout = { VertexPositionColorNormalTexturElement,
							   _countof(VertexPositionColorNormalTexturElement) };

		// ���[�g�V�O�l�`���̃Z�b�g
		psoDesc.pRootSignature = rootSignature_.Get();
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		// �}���`�T���v���ݒ�
		psoDesc.SampleDesc = { 1, 0 };
		psoDesc.SampleMask = UINT_MAX;

		// �f�v�X�o�b�t�@�̃t�H�[�}�b�g��ݒ�
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

		device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline_));
	}
#pragma endregion

	//-------------------------------------------------------------------
	// LightingShader
	//-------------------------------------------------------------------
	LightingShader::LightingShader() : impl_(new Impl) {}

	LightingShader::~LightingShader() {}

	void LightingShader::Initialize(Device* device) {
		auto dev = device->device();

		impl_->CreateShader();
		impl_->CreatRootSignature(dev);
		impl_->CreatePipelineState(dev);

		impl_->srvDescriptorSize_ = dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		impl_->samplerDescriptorSize_ =
			dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	}

	void LightingShader::Terminate() {}

	void LightingShader::Begin(ID3D12GraphicsCommandList* commandList) {
		assert(impl_->commandList == nullptr);
		impl_->commandList = commandList;

		// �V�F�[�_�[�����ł̓e�N�X�`���Q�Ƃ̓t���O�Ő؂蕪���Ă���
		// ���������\�[�X�͑���K�v������A�����ŕ`��J�n���ɓK���ȃe�N�X�`����
		// �f�t�H���g�̃T���v���[��n���Ă�����B�Ƃ肠��������Ŏ~�܂邱�Ƃ͂Ȃ��Ȃ�B
		// ���ۂ̃Q�[���Ŏg���Ȃ炱�̕ӂ͂����ƍH�v�������

		// ����Ղ�[�Ƀf�t�H���g�l��ݒ�
		impl_->samplerHeap_ = impl_->defaultSamplerHeap_;
		impl_->sampler_ = impl_->defaultSampler_;

		// �Ƃ肠�����_�~�[��n���Ă���
		impl_->srvHeap_ = impl_->dammySrvHeap_;
		impl_->srv_ = impl_->dammySrv_;

		commandList->SetGraphicsRootSignature(impl_->rootSignature_.Get());
		commandList->SetPipelineState(impl_->pipeline_.Get());
	}

	void LightingShader::End() {
		assert(impl_->commandList);
		impl_->commandList = nullptr;
	}

	void LightingShader::Apply() {
		ID3D12DescriptorHeap* heaps[] = { impl_->srvHeap_, impl_->samplerHeap_ };

		// _countof�͐��z��̗v�f���𐔂��Ă����}�N��
		impl_->commandList->SetDescriptorHeaps(_countof(heaps), heaps);

		// �萔�o�b�t�@�̓f�X�N���v�^�e�[�u�����g���ĂȂ�
		// ���̏ꍇ��SetGraphicsRootConstantBufferView���g���Ē��ڃA�h���X�𓊂���

		// b0: ObjectParam
		impl_->commandList->SetGraphicsRootConstantBufferView(
			0,                  // �V�F�[�_�̃��W�X�^�ԍ�
			impl_->objParam_);  // GPU���̃A�h���X

		// b1: SceneParam
		impl_->commandList->SetGraphicsRootConstantBufferView(1, impl_->sceneParam_);

		// b2: MaterialParam
		impl_->commandList->SetGraphicsRootConstantBufferView(2, impl_->matParam_);

		// �e�N�X�`���ƃT���v����SetGraphicsRootDescriptorTable�Őݒ�
		impl_->commandList->SetGraphicsRootDescriptorTable(3, impl_->srv_);
		impl_->commandList->SetGraphicsRootDescriptorTable(4, impl_->sampler_);
	}

	void LightingShader::SetObjectParam(D3D12_GPU_VIRTUAL_ADDRESS addr) {
		impl_->objParam_ = addr;
	}

	void LightingShader::SetSceneParam(D3D12_GPU_VIRTUAL_ADDRESS addr) {
		impl_->sceneParam_ = addr;
	}

	void LightingShader::SetMaterialParam(D3D12_GPU_VIRTUAL_ADDRESS addr) {
		impl_->matParam_ = addr;
	}

	void LightingShader::SetSrvDescriptorHeap(ID3D12DescriptorHeap* heap,
		const int offset) {
		impl_->srvHeap_ = heap;
		impl_->srv_ =
			CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->GetGPUDescriptorHandleForHeapStart(),
				offset, impl_->srvDescriptorSize_);
	}

	void LightingShader::SetSamplerDescriptorHeap(ID3D12DescriptorHeap* heap,
		const int offset) {
		impl_->samplerHeap_ = heap;
		impl_->sampler_ =
			CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->GetGPUDescriptorHandleForHeapStart(),
				offset, impl_->srvDescriptorSize_);
	}
	void LightingShader::SetDammySrvDescriptorHeap(ID3D12DescriptorHeap* heap,
		const int offset) {
		// �����Ŏ󂯎�������̂͂����Ǝg���܂킷
		impl_->dammySrvHeap_ = heap;
		impl_->dammySrv_ =
			CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->GetGPUDescriptorHandleForHeapStart(),
				offset, impl_->srvDescriptorSize_);
	}

	void LightingShader::SetDefaultSamplerDescriptorHeap(ID3D12DescriptorHeap* heap,
		const int offset) {
		impl_->defaultSamplerHeap_ = heap;
		impl_->defaultSampler_ =
			CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->GetGPUDescriptorHandleForHeapStart(),
				offset, impl_->srvDescriptorSize_);
	}

}  // namespace dxapp