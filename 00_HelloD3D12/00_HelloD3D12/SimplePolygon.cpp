#include "SimplePolygon.hpp"
#include "Utility.hpp"

void dxapp::SimplePolygon::Setup(ID3D12Device* device)
{
	Teardown();
	m_pDevice = device;

	// プロファイル vs_6_0 で
  // "V"ertex shaderの6.0でコンパイル
	CreateShader(L"simpleVS.hlsl", L"vs_6_0", m_pVertexShader);

	// プロファイル ps_6_0 で
  // "P"ixel shaderの6.0でコンパイル
	CreateShader(L"simplePS.hlsl", L"ps_6_0", m_pPixelShader);

	//	三角ポリゴン生成
	MakeTriangle();

	CreateBufferObject();
	CreateRootSignature();
	CreatePipelineState();
}

void dxapp::SimplePolygon::Teardown()
{
	m_pDevice = nullptr;
	m_vPolygonVertices.clear();
	m_vPolygonIndices.clear();
	m_pVertexBuffer.Reset();
	m_pIndexBuffer.Reset();
	m_vbView = {};
	m_ibView = {};
}

void dxapp::SimplePolygon::MakeCommandList(ID3D12GraphicsCommandList* commandList)
{
	// パイプラインとルートシグネチャを設定
	commandList->SetPipelineState(m_pPipeLine.Get());
	commandList->SetGraphicsRootSignature(m_pRootSignature.Get());

	// IASetPrimitiveTopologyでのデータの並び
	// D3D_PRIMITIVE_TOPOLOGY_TRIANGLELISTで3角形の頂点が順番に並んでいるしてい
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 描画したいD3D12_VERTEX_BUFFER_VIEWを渡す
  // D3D12_VERTEX_BUFFER_VIEWはパイプラインとあって入れば複数渡せる
  //   複数渡したいときは下記のようにすればよい
  //   D3D12_VERTEX_BUFFER_VIEW = vbViews[] {
  //     頂点バッファビュー1, バッファビュー2, ... バッファビューn};
	commandList->IASetVertexBuffers(0, 1, &m_vbView);

	// インデクスバッファ
	commandList->IASetIndexBuffer(&m_ibView);

	// DrawIndexedInstancedは名前のとおり、インデクスを使って描画するコマンド
// Instancedは同じものを繰り返し描画するときに使える
// とりあえずここでは3角形しか設定しないので下記の値でよい
	commandList->DrawIndexedInstanced(
		3,  // 1回の描画に使うインデックスの数
		1,  // 繰り返し数
		0,  // インデクスバッファの何番目の要素から読み取り開始するか
		0,  // 頂点バッファの何番目の要素から読み取り開始するか
		0);  // 頂点バッファからデータを読み取るときに、インデックスに加算する値

}

void dxapp::SimplePolygon::MakeTriangle()
{
	// 頂点作成
	m_vPolygonVertices = {
		// 0番
		{{0.f, 0.5f, 0.f}, {1.f, 0.f, 0.f, 1.f}},
		// 1番
		{{0.5f, -0.5f, 0.f}, {0.f, 1.f, 0.f, 1.f}},
		// 2番
		{{-0.5f, -0.5f, 0.f}, {0.f, 0.f, 1.f, 1.f}},
	};

	// Gpuが頂点をつかう順番を指定。今回は下のように頂点をめぐって三角形にする
	//       0
	//     ／  ＼
	//   ／      ＼
	// 2ーーーーーー1
	m_vPolygonIndices = { 0, 1, 2 };
}

void dxapp::SimplePolygon::CreateShader(const std::wstring& fileName, const std::wstring& profile, Microsoft::WRL::ComPtr<ID3DBlob>& blob)
{
	// シェーダープログラムは事前にコンパイルしておく方法もあるがいまは都度コンパイルしまう
	Microsoft::WRL::ComPtr<ID3DBlob>error;
	auto hr = Util::CompileShaderFromFile(fileName, profile, blob, error);
	if (FAILED(hr)) {
		OutputDebugStringA((const char*)error->GetBufferPointer());
		throw std::runtime_error("ComplieShaderFromFile Failed.");
	}
}

void dxapp::SimplePolygon::CreateBufferObject()
{
	// polygonVertecies_ と polygonIndices_からバッファオブジェクトを作る

	//頂点
	{
		// 作成するバッファのメモリサイズを計算
		// sizeof(polygonVertecies_[0]で1個当たりのメモリ * size()で配列の使用数
		auto bufferSize = sizeof(m_vPolygonVertices[0]) * m_vPolygonVertices.size();

		// バッファの作成（ここではVRAMを確保するだけ）
		CreateCommittedResource(bufferSize, m_pVertexBuffer);

		//確保したメモリにデータコピー
		Mapping(m_pVertexBuffer, m_vPolygonVertices.data(), bufferSize);

		//GPUにあるバッファのメモリ情報うぃ設定
		m_vbView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();//GPU内のメモリアドレス
		m_vbView.SizeInBytes = static_cast<UINT>(bufferSize);//データサイズ
		m_vbView.StrideInBytes = sizeof(m_vPolygonVertices[0]);//1個あたりのデータサイズ
	}
	//インデックス
	// 頂点バッファの作成とほとんど一緒なんだけど微妙に違うところもあるので注意！
	{
		auto bufferSize = sizeof(m_vPolygonIndices[0]) * m_vPolygonIndices.size();
		CreateCommittedResource(bufferSize, m_pIndexBuffer);
		Mapping(m_pIndexBuffer, m_vPolygonIndices.data(), bufferSize);

		// D3D12_INDEX_BUFFER_VIEWは使う値がちょっと違う
		m_ibView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
		m_ibView.SizeInBytes = static_cast<UINT>(bufferSize);//データサイズ
		m_ibView.Format = DXGI_FORMAT_R32_UINT;// インデックスに使用している値のフォーマット
	}
}

void dxapp::SimplePolygon::CreateCommittedResource(std::size_t bufferSize, Microsoft::WRL::ComPtr<ID3D12Resource1>& resource)
{
	auto hr = m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),//バッファサイズ
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(resource.ReleaseAndGetAddressOf()));
}

void dxapp::SimplePolygon::Mapping(Microsoft::WRL::ComPtr<ID3D12Resource1>& resource, const void* src, std::size_t bufferSize)
{
	void* mapped{ nullptr };
	CD3DX12_RANGE range{ 0,0 };
	auto hr = resource->Map(0, &range, &mapped);
	memcpy(mapped, src, bufferSize);
	resource->Unmap(0, nullptr);
}

void dxapp::SimplePolygon::CreateRootSignature()
{
	// ルートシグネチャは名前から動作が想像しずらいが
	// シェーダのパラメータのレイアウト情報を記述するものと考えておく
	// そのため使うシェーダとルートシグネチャの記述があっていないと描画に失敗する
	CD3DX12_ROOT_SIGNATURE_DESC desc{};
	// CD3DX12_ROOT_SIGNATURE_DESCに作成したいルートシグネチャのデータを記述
	// ここで使うシェーダは渡すパラメータがないので下記のような0とnullばっかりの設定
	desc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	Microsoft::WRL::ComPtr<ID3DBlob>buffer{}, error{};
	//bufferにrootSignatureの内容でメモリ確保
	auto hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &buffer, &error);
	if (FAILED(hr)) { throw std::runtime_error("D3D12SerializeRootSignature Failed."); }
	// bufferからRootSignature の生成
	hr = m_pDevice->CreateRootSignature(
		0,
		buffer->GetBufferPointer(),//バッファの位置
		buffer->GetBufferSize(),//バッファサイズ	
		IID_PPV_ARGS(&m_pRootSignature)
	);
	if (FAILED(hr)) { throw std::runtime_error("CreateRootSignature Failed."); }
}

void dxapp::SimplePolygon::CreatePipelineState()
{
	// パイプラインステートオブジェクトは
  // 描画で使用するシェーダ、ルートシグネチャ、入力頂点データ、レンダーターゲット
  // などなどを設定するオブジェクト。
  // 描画コマンドに先んじてまずはキューにPSOを渡すことになる。

  // んで描画設定が少しでも違う場合（シェーダに渡すパラメータが1つでも違うとか）は
  // 別のPSOがいる！！これはかなり管理が面倒でDx12が辛い理由の１つでもある。

  // パイプラインステートオブジェクトの定義を作る

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	//使うシェーダー
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(m_pVertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(m_pPixelShader.Get());
	//ブレンドステート
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	//ラスタライザステート
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	//出力するレンダーターゲットの数と設定
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;  // ここはレンダーターゲットと合わせる

	 // 頂点シェーダに入力する頂点レイアウト
	psoDesc.InputLayout = { c_VertexLayout,_countof(c_VertexLayout) };
	// ルートシグネチャのセット
	psoDesc.pRootSignature = m_pRootSignature.Get();

	//トポロジー
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// マルチサンプル設定(いわゆるアンチエイリアスっぽいやつ)とりあえず下記の設定
	psoDesc.SampleDesc = { 1, 0 };
	psoDesc.SampleMask = UINT_MAX;

	// デプスバッファとステンシルの設定
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;  // デプスバッファのフォーマットを設定
	auto dsvDesc = CD3DX12_DEPTH_STENCIL_DESC();
	dsvDesc.DepthEnable = FALSE;
	psoDesc.DepthStencilState = dsvDesc;
	auto hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipeLine));
	if (FAILED(hr)) { throw std::runtime_error("CreateGraphicsPipelineState Failed."); }
}
