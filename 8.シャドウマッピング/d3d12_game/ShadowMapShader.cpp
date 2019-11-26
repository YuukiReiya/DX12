#include "ShadowMapShader.hpp"

#include "BufferObject.hpp"
#include "Device.hpp"
#include "Utility.hpp"
#include "VertexType.hpp"

namespace dxapp {
#pragma region ShadowMapShader::Impl
	/*!
	 * @brief ShadowMapShaderの実装
	 */
	class ShadowMapShader::Impl {
	public:
		/*!
		 * @brief コンストラクタ
		 */
		Impl() = default;
		/*!
		 * @brief デストラクタ
		 */
		~Impl() = default;

		/*!
		 * @brief シェーダー生成
		 */
		void CreateShader();

		/*!
		 * @brief ルートシグネチャ生成
		 */
		void CreatRootSignature(ID3D12Device* device);

		/*!
		 * @brief パイプライン生成
		 */
		void CreatePipelineState(ID3D12Device* device);

		//! 頂点シェーダ
		Microsoft::WRL::ComPtr<ID3DBlob> vs_{};
		//! ピクセルシェーダ
		Microsoft::WRL::ComPtr<ID3DBlob> ps_{};
		//! ルートシグネチャ
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_{};
		//! パイプラインステート
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_{};

		// Begin/Endで使うコマンドリスト
		ID3D12GraphicsCommandList* commandList{};

		// コマンドリストに積む定数バッファへのGPUハンドルのキャッシュ
		D3D12_GPU_VIRTUAL_ADDRESS objParam_, sceneParam_;
	};

	void ShadowMapShader::Impl::CreateShader() {
		Microsoft::WRL::ComPtr<ID3DBlob> error;
		utility::CompileShaderFromFile(L"Shaders/ShadowMapVS.hlsl", L"vs_6_0", vs_,
			error);
	}

	// ルートシグネチャ生成
	void ShadowMapShader::Impl::CreatRootSignature(ID3D12Device* device) {
		// b0, b1の2個のパラメータを使う
		CD3DX12_ROOT_PARAMETER rootParams[2];
		rootParams[0].InitAsConstantBufferView(0);  // b0:ObjectParam
		rootParams[1].InitAsConstantBufferView(1);  // b1:SceneParam

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

	// パイプラインステートオブジェクト生成
	void ShadowMapShader::Impl::CreatePipelineState(ID3D12Device* device) {
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		// シェーダーのセット
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vs_.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(0, 0);  // ピクセルシェーダーはなし
		// ルートシグネチャのセット
		psoDesc.pRootSignature = rootSignature_.Get();
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.InputLayout = { VertexPositionColorNormalTexturElement,
							   _countof(VertexPositionColorNormalTexturElement) };
		// ブレンドステート設定
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		// ラスタライザーステート
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		// シャドウのバイアス
		// シャドウマップにでるアーティファクトを防止する
		psoDesc.RasterizerState.DepthBias = 100000;
		psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
		psoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;

		// マルチサンプル設定
		psoDesc.SampleDesc = { 1, 0 };
		psoDesc.SampleMask = UINT_MAX;

		// レンダーターゲットない時の設定はこう！
		psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
		psoDesc.NumRenderTargets = 0;
		// デプスステンシルフォーマットを設定
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline_));
	}
#pragma endregion

	ShadowMapShader::ShadowMapShader() : impl_(new Impl) {}

	ShadowMapShader::~ShadowMapShader() {
		{ Terminate(); };
	}

	void ShadowMapShader::Initialize(Device* device) {
		auto dev = device->device();

		impl_->CreateShader();
		impl_->CreatRootSignature(dev);
		impl_->CreatePipelineState(dev);
	}

	void ShadowMapShader::Terminate() {}

	void ShadowMapShader::Begin(ID3D12GraphicsCommandList* commandList) {
		assert(impl_->commandList == nullptr);
		impl_->commandList = commandList;
		commandList->SetGraphicsRootSignature(impl_->rootSignature_.Get());
		commandList->SetPipelineState(impl_->pipeline_.Get());
	}

	void ShadowMapShader::End() {
		assert(impl_->commandList);
		impl_->commandList = nullptr;
	}

	void ShadowMapShader::Apply() {
		// b0: ObjectParam
		impl_->commandList->SetGraphicsRootConstantBufferView(
			0,                  // シェーダのレジスタ番号
			impl_->objParam_);  // GPU側のアドレス

		// b1: SceneParam
		impl_->commandList->SetGraphicsRootConstantBufferView(1, impl_->sceneParam_);
	}

	void ShadowMapShader::SetObjectParam(D3D12_GPU_VIRTUAL_ADDRESS addr) {
		impl_->objParam_ = addr;
	}

	void ShadowMapShader::SetSceneParam(D3D12_GPU_VIRTUAL_ADDRESS addr) {
		impl_->sceneParam_ = addr;
	}

}  // namespace dxapp