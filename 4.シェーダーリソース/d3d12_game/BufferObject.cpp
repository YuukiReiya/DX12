#include "pch.h"
#include "BufferObject.hpp"

bool dxapp::BufferObject::Setup(ID3D12Device* device, const BufferType type, std::size_t size)
{
	auto heapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	auto state = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ;

	auto bufferSize = size;
	// コンスタントバッファのメモリアラインメント
	if (type == BufferType::ConstantBuffer)
	{
		//コンスタントバッファは256バイト単位で作る必要がある
		bufferSize = ((bufferSize + 255) & ~(255));
	}
	//リソース作成
	auto prop = CD3DX12_HEAP_PROPERTIES(heapType);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	auto hr = device->CreateCommittedResource(
		&prop, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr,
		IID_PPV_ARGS(m_Resource.ReleaseAndGetAddressOf())
	);
	if (FAILED(hr)) { return false; }
	m_HeapProp = prop;
	m_ResourceDesc = desc;
	m_BufferSize = bufferSize;
	return true;
}

void dxapp::BufferObject::Teardown()
{
	m_Resource.Reset();
}

void* dxapp::BufferObject::Map()
{
	void* p{ nullptr };
	m_Resource->Map(0, nullptr, &p);
	return p;
}

void dxapp::BufferObject::Unmap()
{
	m_Resource->Unmap(0, nullptr);
}

void dxapp::BufferObject::Update(const void* data, std::size_t size, std::size_t offset)
{
	if (m_HeapProp.Type == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
	{
		std::uint8_t* addr = static_cast<std::uint8_t*>(Map());
		memcpy(addr + offset, data, size);
		Unmap();
	}else{
	//とりま空
	}
}
