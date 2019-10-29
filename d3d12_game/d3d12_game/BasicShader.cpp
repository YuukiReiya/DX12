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

	Microsoft::WRL::ComPtr<ID3DBlob> vs_{};
	Microsoft::WRL::ComPtr<ID3DBlob> ps_{};
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_{};
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_{};

	// デスクリプタサイズのキャッシュ
	UINT srvDescriptorSize_{}, samplerDescriptorSize_{};

	// Begin/Endないで使うコマンドリスト
	ID3D12GraphicsCommandList* commandList{};

	// コマンドリスト積み込みに使うデスクリプタのキャッシュ
	// なんかもうちょっとスマートにやりたい
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

// ルートシグネチャ生成
// SimplePolygonクラスより断然複雑。前回はリソースが全くなかったので。
void BasicShader::Impl::CreatRootSignature(ID3D12Device* device) {
	// リソースのデスクリプタレンジを作ります
	// シェーダの作成でみたようにb0/b1 ... bnとレジスタ割り当てができる
	// そしてレジスタごとにデスクリプタを使うのでその準備
	// 今回は定数・SRV・サンプラ1個ずつなのであまり悩まなくてよい
	CD3DX12_DESCRIPTOR_RANGE cbv, srv, sampler;
	// コンスタントバッファ(b0)の準備
	cbv.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,  // リソース種類：コンスタントバッファ
		1,                                // b0の1個
		0);                               // とりあえず0でOK
	// テクスチャ(t0)の準備
	srv.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	// テクスチャサンプラ(s0)の準備
	sampler.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

	// リソースがあるときはROOT_PARAMETERが必要
	// 上で準備したデスクリプタがどのように配置されるか。
	// またどの部分を見せるかといった設定ができる
	CD3DX12_ROOT_PARAMETER rootParams[3];
	rootParams[0].InitAsDescriptorTable(
		1, &cbv,  // b0の設定
		// リソースが見えるシェーダの範囲。これは頂点シェーダだけ
		D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[1].InitAsDescriptorTable(
		1, &srv,                         // t0
		D3D12_SHADER_VISIBILITY_PIXEL);  // ピクセルシェーダから見える
	rootParams[2].InitAsDescriptorTable(
		1, &sampler,                     // s0
		D3D12_SHADER_VISIBILITY_PIXEL);  // ピクセルシェーダから見える

	// ルートパラメタをもとにルートシグネチャの"設定"を作る
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc{};
	rootSigDesc.Init(
		_countof(rootParams), rootParams,  // ROOT_PARAMETERをわたす
		0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// シリアライズ
	Microsoft::WRL::ComPtr<ID3DBlob> signature;
	Microsoft::WRL::ComPtr<ID3DBlob> error;
	D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
		&signature, &error);

	// やっとルートシグネチャができました！
	device->CreateRootSignature(0, signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature_));
}

// パイプラインステートオブジェクト生成
// ここはSimplePolygonと一緒
void BasicShader::Impl::CreatePipelineState(ID3D12Device* device) {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	// シェーダーのセット
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vs_.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(ps_.Get());
	// ブレンドステート設定
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	// ラスタライザーステート
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	// 出力先は1ターゲット
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.InputLayout = { simpleVertex::c_InputLayoutElement,
						   _countof(simpleVertex::c_InputLayoutElement) };
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	// ルートシグネチャのセット
	psoDesc.pRootSignature = rootSignature_.Get();
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// マルチサンプル設定
	psoDesc.SampleDesc = { 1, 0 };
	psoDesc.SampleMask = UINT_MAX;

	// デプスバッファのフォーマットを設定
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

	// _countofは生配列の要素数を数えてくれるマクロ
	impl_->commandList->SetDescriptorHeaps(_countof(heaps), heaps);

	// ルートパラメータとリソースと同じ並びにする
	impl_->commandList->SetGraphicsRootDescriptorTable(
		0,             // ルートパラメータでのインデックス
		impl_->cbv_);  // GPUハンドル
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
