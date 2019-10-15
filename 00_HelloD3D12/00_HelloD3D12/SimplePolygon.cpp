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

	//追加で三角形を生成
	{
		constexpr int c_ExtensionContentsSize = 100;
		m_vPolygonVertices.reserve(c_ExtensionContentsSize);
		m_vPolygonIndices.reserve(c_ExtensionContentsSize);
		DirectX::XMFLOAT3 p{ -0.5f,-0.5f,0 }, c{};
		for (size_t i = 0; i < 10; i++)
		{
			//座標
			p.x = p.y += 0.1f;
			p.z += 0.1f;
			//色
			c.x = c.y = c.z += 0.1f;
			//生成
			AddTriangle(p, c);
		}
	}

	//四角形
	{
		m_StartQuadVertex = static_cast<UINT>(m_vPolygonVertices.size());
		m_StartQuadIndex = static_cast<UINT>(m_vPolygonIndices.size());
		DirectX::XMFLOAT3 p{ 0.0f,0.0f,0.0f };
		float scale{ 1.0f };
		AddQuad(p, scale);
	}

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
	//commandList->DrawIndexedInstanced(
	//	3,  // 1回の描画に使うインデックスの数
	//	1,  // 繰り返し数
	//	0,  // インデクスバッファの何番目の要素から読み取り開始するか
	//	0,  // 頂点バッファの何番目の要素から読み取り開始するか
	//	0);  // 頂点バッファからデータを読み取るときに、インデックスに加算する値

	// 頂点数 / 3 で三角ポリゴンの数を計算
	const auto size = m_vPolygonVertices.size() / 3;
	for (size_t i = 0; i < size; i++)
	{
		UINT offset = i * 3;

		commandList->DrawIndexedInstanced(
			3,//1回の描画に使うインデックスの数
			1,//繰り返し回数
			offset,//インデックスバッファの読み出し位置指定
			offset,//頂点バッファの読み出し位置指定
			0
		);

		// よくよく考えると3角形の頂点はそれぞれ固有の情報(座標とかね)
		// しかしインデクスはどの三角形でも共通
		// なので今回のサンプルではインデクス配列を増やさず使いまわすこともできる(やってみよう)
		// 今後モデル描画なども考えてこのようにしてみた
	}
	//四角形描画用コード
	{
		auto size = m_vPolygonVertices.size() - m_StartQuadVertex;
		if (size < 4) { return; }//四角形がなかった
		size /= 4;
		for (size_t i = 0; i < size; i++)
		{
			int iOffset = i * 6 + m_StartQuadIndex;
			int vOffset = i * 4 + m_StartQuadVertex;
			commandList->DrawIndexedInstanced(
				6,//1回の描画に使うインデックスの数
				1,//繰り返し数S
				iOffset,
				vOffset,
				0
			);
		}
	}
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

void dxapp::SimplePolygon::AddTriangle(const DirectX::XMFLOAT3 pos, const DirectX::XMFLOAT3 cr)
{
	//三角ポリゴン構成用頂点 = ３頂点
	VertexPositionColor vertices[]
	{
		//0番目
		{
			//座標
			{0.0f + pos.x,0.5f + pos.y,pos.z},
			//色
			{1.0f * cr.x,0.0f,0.0f,1.0f}
		},
		{
			//座標
			{0.5f + pos.x,-0.5f + pos.y,pos.z},
			//色
			{0.0f,1.0f * cr.y,0.0f,1.0f}
		},
		{
			//座標
			{-0.5f + pos.x,-0.5f + pos.y,pos.z},
			//色
			{0.0f,0.0f ,1.0f * cr.x,1.0f}
		},
	};
	std::uint32_t indices[] = { 0,1,2 };
	//ポリゴンの頂点配列に追加
	std::copy(std::begin(vertices), std::end(vertices), std::back_inserter(m_vPolygonVertices));
	//	ポリゴンのインデックス配列に追加
	std::copy(std::begin(indices), std::end(indices), std::back_inserter(m_vPolygonIndices));
}

void dxapp::SimplePolygon::AddQuad(const DirectX::XMFLOAT3 pos, const float scale)
{
	// 0ーー1   4角形は4つの頂点をつかってつくる。
	 // |    |   あれ？3角形を組み合わせるから6頂点いるのでは・・・？
	// 2ーー3
	float point{ 0.5f * scale };
	VertexPositionColor vertices[]{
		// 0番
		{{-point + pos.x, point + pos.y, pos.z}, {1.f, 0.f, 0.f, 1.f}},
		// 1番
		{{point + pos.x, point + pos.y, 0.f + pos.z}, {0.f, 1.f, 0.f, 1.f}},
		// 2番
		{{-point + pos.x, -point + pos.y, 0.f + pos.z}, {0.f, 0.f, 1.f, 1.f}},
		// 3番
		{{point + pos.x, -point + pos.y, 0.f + pos.z}, {1.f, 1.f, 1.f, 1.f}},
	};

	// 4頂点だがインデクスをつかって2ポリゴンを作る
	// 0ーー1         1
	// | ／   +    ／ |
	// 2         2ーー3
	//     ！合体！
	//     0ーー1   4頂点で2ポリゴンができた
	//     | ／ |
	//     2ーー3

	 // なおDirectXはポリゴンのインデクスは
	// ポリゴン正面からみて左回りで頂点を結ぶのが基本
	std::uint32_t indices[] = { 0, 1, 2, 1, 3, 2 };

	// polygonVertecies_の最後尾にvertexの内容をコピー
	std::copy(std::begin(vertices), std::end(vertices),
		std::back_inserter(m_vPolygonVertices));

	// polygonVertecies_の最後尾にvertexの内容をコピー
	std::copy(std::begin(indices), std::end(indices),
		std::back_inserter(m_vPolygonIndices));
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
