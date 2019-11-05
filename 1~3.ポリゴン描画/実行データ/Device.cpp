#include "pch.h"
#include "Device.hpp"

DX12::Device::Device() {}

DX12::Device::~Device()
{
	// GPU�̏������I����Ă邢��̂��m�F���Ă���I���悤�ɂ���
	WaitForGPU();
#if  _DEBUG||DEBUG
	{
		//D3D�̐����Ă���I�u�W�F�N�g���m�F�ł���
		ComPtr<ID3D12DebugDevice>debugInterface;
		if (SUCCEEDED(m_pDevice->QueryInterface(IID_PPV_ARGS(&debugInterface)))) { debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL); }
	}
#endif
}

void DX12::Device::Setup(HWND hWnd, std::uint32_t width, std::uint32_t height)
{
#pragma region �f�o�C�X
	CreateDevice();
#pragma endregion

	// �`�施�߂𐶐��E�ςݍ��݂Ɏg���I�u�W�F�N�g�����܂�
	// �����ō��3��ނ̃I�u�W�F�N�g��g�ݍ��킹�邱�Ƃŕ`�悪�ł���悤�ɂȂ�܂�

#pragma region �R�}���h�L���[
	{
		// D3D12_COMMAND_LIST_TYPE_DIRECT�͕`�施�߂Ŏg������
		D3D12_COMMAND_QUEUE_DESC desc{};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//�l�߂郊�X�g�̎��
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		// D3D12_COMMAND_LIST_TYPE_DIRECT�̃L���[�ɂ�
		// LIST_TYPE_DIRECT��LIST_TYPE_BUNDLE�̃R�}���h��ςނ��Ƃ��ł���B
		// �����LIST_TYPE_DIRECT�������BBUNDLE�ɂ��Ă͍���Ɋ��ҁB
		auto hr = m_pDevice->CreateCommandQueue(
			&desc,
			IID_PPV_ARGS(m_pCommandQueue.ReleaseAndGetAddressOf()));
		if (FAILED(hr)) { throw std::runtime_error("Device::CreateCommandQueue Failed."); }
		m_pCommandQueue->SetName(L"Device::ID3D12CommandQueue");
		// �R�}���h���X�g�����ɂ̓R�}���h���X�g�̎�ނɉ������������A���P�[�^������
		// �R�}���h�������ɃA���P�[�^�������������蓖�ĂĂ�����
	}
#pragma endregion
#pragma region �R�}���h�A���P�[�^�[
	{
		for (auto& alloc : m_pCommandAllocators)
		{
			auto hr = m_pDevice->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,// ��������R�}���h���X�g�̎�ނ��w��
				IID_PPV_ARGS(alloc.ReleaseAndGetAddressOf()));
			if (FAILED(hr)) { throw std::runtime_error("Device::CreateCommandAllocator Failed."); }
			alloc->SetName(L"Device::ID3D12CommandAllocator");
		}
	}
#pragma endregion
#pragma region �R�}���h���X�g
	{
		auto hr = m_pDevice->CreateCommandList(
			0,//��{��"0"
			D3D12_COMMAND_LIST_TYPE_DIRECT,//�ʏ��̕`��ɂ�"DIRECT"���g��
			m_pCommandAllocators[0].Get(),//���������m�ۂ���A���P�[�^�[
			nullptr,//�k���ۂ�OK
			IID_PPV_ARGS(m_pGraphicsCommandList.ReleaseAndGetAddressOf())
		);
		if (FAILED(hr)) { throw std::runtime_error("Device::CreateCommandList Failed."); }
		m_pGraphicsCommandList->SetName(L"Device::CreateCommandList");
		m_pGraphicsCommandList->Close();
	}
#pragma endregion
#pragma region �t�F���X
	CreateFence();
#pragma endregion
#pragma region �X���b�v�`�F�C��
	CreateSwapChain(hWnd, width, height, m_dfBackBufferFormat);
#pragma endregion
#pragma region RTV
	m_ScreenViewport.Width = static_cast<FLOAT>(width);
	m_ScreenViewport.Height = static_cast<FLOAT>(height);
	m_ScreenViewport.TopLeftX = 0.0f;
	m_ScreenViewport.TopLeftY = 0.0f;
	m_ScreenViewport.MinDepth = D3D12_MIN_DEPTH;
	m_ScreenViewport.MaxDepth = D3D12_MAX_DEPTH;

	m_ScissorRect.left = 0;
	m_ScissorRect.top = 0;
	m_ScissorRect.right = width;
	m_ScissorRect.bottom = height;
#pragma endregion
}

void DX12::Device::Present()
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_RenderTargets[m_iBackBufferIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
	);
	// �R�}���h���X�g��Close���Ă����Ȃ��Ǝ��s�ł��܂����
	m_pGraphicsCommandList->Close();

	// �����ł����GPU�ɕ`�揈����������
	// Execute��ID3D12CommandList����Ȃ��ƃ_���Ȃ̂ŃL���X�g���Ă邾��
	auto cl = reinterpret_cast<ID3D12CommandList* const*>(
		m_pGraphicsCommandList.GetAddressOf());

	// �L���[�ɐς܂ꂽ�R�}���h�����s����
	m_pCommandQueue->ExecuteCommandLists(
		1,// �L���[�ɂ͕����̃��X�g���l�߂�̂Ŏ��s���郊�X�g�����w��
		cl
	);

	// �X���b�v�`�F�C���̓��e��؂�ւ���
	auto hr = m_pSwapChain->Present(
		1,// �o�b�N�o�b�t�@��؂�ւ���VSync�����x�҂��B1��1��܂�
		0
	);

	// ������ExecuteCommandLists / Present��GPU�Ɂu�����I�v�Ǝw�����Ă��邾���B
	// �܂�񓯊����s�Ȃ̂ŁA�`�悪�����̂��܂��Ď���̃t���[���ɐi�ޕK�v������
	WaitForRenderingCompletion();
}

void DX12::Device::PrepareRendering()
{
	// �A���P�[�^�ƃR�}���h���X�g�����Z�b�g���đO�̓��e��Y����
	m_pCommandAllocators[m_iBackBufferIndex]->Reset();
	m_pGraphicsCommandList->Reset(m_pCommandAllocators[m_iBackBufferIndex].Get(), nullptr);

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_RenderTargets[m_iBackBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	// ���\�[�X�o���A��ݒ�
	m_pGraphicsCommandList->ResourceBarrier(1, &barrier);
}

void DX12::Device::WaitForRenderingCompletion()
{
	const auto currentValue = m_FenceValues[m_iBackBufferIndex];
	m_pCommandQueue->Signal(m_pFence.Get(), currentValue);

	// �o�b�N�o�b�t�@���؂�ւ���Ă���̂ŐV�����C���f�b�N�X�����炤
	m_iBackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// �������x�ɂ���Ă͂����ɓ��B�������_�ŕ`�悪�I���t�F���X�l���X�V����Ă���\��������
	// ���̏�Ԃ�WaitForSingleObjectEx������Ɩ����ɑ҂��ƂɂȂ邼�B
	// �����Ȃ�Ȃ��悤��GetCompletedValue�Ńt�F���X�̌��ݒl���m�F����B
	if (m_pFence->GetCompletedValue() < m_FenceValues[m_iBackBufferIndex])
	{
		// �܂��`�悪�I����Ă��Ȃ��̂ő҂K�v������
		m_pFence->SetEventOnCompletion(m_FenceValues[m_iBackBufferIndex], m_FenceEvent.Get());
		WaitForSingleObjectEx(m_FenceEvent.Get(), INFINITE, false);
	}
	// ���̃t���[���̂��߂Ƀt�F���X�l�X�V
	m_FenceValues[m_iBackBufferIndex] = currentValue + 1;
}

void DX12::Device::WaitForGPU()
{
	if (m_pCommandQueue && m_pFence && m_FenceEvent.IsValid())
	{
		// ���݂̃t�F���X�l�����[�J���ɃR�s�[
		// ���[�J���ɃR�s�[�����ق������x�ʂŗL���Ƃ̂��킳(���؂��ĂȂ�)
		auto currentValue = m_FenceValues[m_iBackBufferIndex];

		// �L���[�ɐς񂾃R�}���h�̎��s������҂��܂�

		// CommandQueue::Signal(�t�F���X, �X�V���ꂽ���̒l)�������
		// �L���[�̎��s���I������Ƃ��Ƀt�F���X�̒��g����2�����̒l�ɍX�V�����
		if (SUCCEEDED(m_pCommandQueue->Signal(m_pFence.Get(), currentValue))) {
			// SetEventOnCompletion��fence_�������Ă���l���AcurrentValue��
			// �X�V���ꂽ�Ƃ��ɃC�x���g�����ł���悤�ɐݒ肷��
			if (SUCCEEDED(m_pFence->SetEventOnCompletion(currentValue, m_FenceEvent.Get())))
			{
				// �����ő҂��܂�
				// �������[�v�݂����Ȃ����ł��҂Ă邯�ǁACPU�𖳑ʂɎg���̂ł����߂��Ȃ�
				WaitForSingleObjectEx(m_pFence.Get(), INFINITE, FALSE);

				// ����̏����̂��߂Ƀt�F���X�l�𑝂₵�Ă���
				m_FenceValues[m_iBackBufferIndex]++;
			}
		}
	}
}

void DX12::Device::CreateDevice()
{
	HRESULT hr = S_FALSE;
#if _DEBUG
	{
		ComPtr<ID3D12Debug>debug;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) { debug->EnableDebugLayer(); }
		else { throw std::runtime_error("Device::D3D12GetDebugInterface Failed."); }
		//�@�\�ݒ�
		ComPtr<IDXGIInfoQueue>DXGIInfoQueue;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(DXGIInfoQueue.GetAddressOf()))))
		{
			m_DXGIFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
			DXGIInfoQueue->SetBreakOnSeverity(
				DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			DXGIInfoQueue->SetBreakOnSeverity(
				DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		}
	}
#endif // _DEBUG

#pragma region �t�@�N�g��
	hr = CreateDXGIFactory2(m_DXGIFactoryFlags, IID_PPV_ARGS(m_pDXGIFactory.ReleaseAndGetAddressOf()));
	if (FAILED(hr)) { throw std::runtime_error("Device::CreateDXGIFactory2 Failed."); }
#pragma endregion

#pragma region �A�_�v�^
	// �A�_�v�^(Windows���猩����GPU���ۂ����)�쐬
	ComPtr<IDXGIAdapter1>adapter{ nullptr };
	for (UINT i = 0;; ++i)
	{
		// �g����A�_�v�^�����Ԓ��ׂ�
		if (DXGI_ERROR_NOT_FOUND ==
			m_pDXGIFactory->EnumAdapters1(i, adapter.ReleaseAndGetAddressOf())) {
			// DXGI_ERROR_NOT_FOUND�̓A�_�v�^���Ȃ����
			break;
		}

		// �A�_�v�^�̏����擾���ăQ�[���Ŏg���邩���`�F�b�N
		DXGI_ADAPTER_DESC1 desc{};
		hr = adapter->GetDesc1(&desc);
		if (FAILED(hr)) { throw std::runtime_error("Device::IDXGIAdapter1::GetDesc1 Failed."); }

		// �\�t�g�E�F�A�A�_�v�^(CPU����)�͒x������̂Ŏg��Ȃ�
		if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) { continue; }

		// �A�_�v�^��D3D12���g���邩�`�F�b�N
		// ���߂��ɃA�_�v�^����f�o�C�X������Ă݂�
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0,
			_uuidof(ID3D12Device), nullptr))) {
			// D3D12���g����A�_�v�^���������̂Ń��[�v������
			break;
		}
	}
#if _DEBUG
	if (!adapter) {
		if (UseWrapAdapter && FAILED(m_pDXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf())))) { throw std::runtime_error("WARP adapter not found."); }
	}
#endif
	if (!adapter) { throw std::runtime_error("D3D12 device not found."); }
#pragma endregion

	//�A�_�v�^����f�o�C�X�̍쐬
	hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_pDevice.ReleaseAndGetAddressOf()));
	if (FAILED(hr)) { throw std::runtime_error("Device::CreateDevice Failed."); }
	m_pDevice->SetName(L"Device::ID3D12Device");
}

void DX12::Device::CreateFence()
{
	//�t�F���X�I�u�W�F�N�g�����
	auto hr = m_pDevice->CreateFence(
		m_FenceValues[m_iBackBufferIndex],//�����l
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(m_pFence.ReleaseAndGetAddressOf())
	);
	if (FAILED(hr)) { throw std::runtime_error("Device::CreateFence Failed."); }
	m_pFence->SetName(L"Device::ID3D12Fence");
	//����̃t�F���X�l��ݒ�
	m_FenceValues[m_iBackBufferIndex]++;
	// GPU�̏������I���̑ҋ@���邽�߂Ɏg���܂�
	m_FenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
	if (!m_FenceEvent.IsValid()) { throw std::runtime_error("Device::CreateEventEx Failed."); }
}

void DX12::Device::CreateSwapChain(HWND hWnd, std::uint32_t width, std::uint32_t height, DXGI_FORMAT format)
{
	WaitForGPU();

#pragma region RTV�ƃt�F���X�̏�����
	for (std::uint32_t i = 0; i < c_MaxBackBufferSize; ++i)
	{
		m_RenderTargets[i].Reset();
		m_FenceValues[i] = m_FenceValues[m_iBackBufferIndex];
	}
#pragma endregion

#pragma region �X���b�v�`�F�C���̒�`
	DXGI_SWAP_CHAIN_DESC1 desc{};
	desc.BufferCount = m_iBackBufferSize;// �o�b�t�@�̐�
	desc.Width = width;
	desc.Height = height;
	desc.Format = format;// �o�b�N�o�b�t�@�̃t�H�[�}�b�g
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;//	�����_�[�^�[�Q�b�g�̏ꍇ�͂���
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
#pragma endregion

#pragma region �X���b�v�`�F�C���쐬
	ComPtr<IDXGISwapChain1>sc;
	auto hr = m_pDXGIFactory->CreateSwapChainForHwnd(
		m_pCommandQueue.Get(), hWnd, &desc, nullptr, nullptr, sc.GetAddressOf()
	);
	if (FAILED(hr)) { throw std::runtime_error("CreateSwapChainForHwnd Failed."); }
#pragma endregion
	//As�֐��ň����̌^(�X���b�v�`�F�C��)�̃C���^�[�t�F�[�X���擾
	hr = sc.As(&m_pSwapChain);
	if (FAILED(hr)) { throw std::runtime_error("Obtaining IDXGISwapChain 4 Interface Failed."); }
#pragma region RT
	// �����_�[�^�[�Q�b�g(RT)�����܂�
	{
		// RT�̃f�X�N���v�^�[���쐬
		// VRAM�ɂ��郊�\�[�X�͂����̃������̉�B
		// �����Ńf�X�N���v�^�[���g���ăf�[�^�̎�ނ⃁�����ڂ��Ă����B
		// GPU�̓f�X�N���v�^�[�g���ă��\�[�X�̌�����m��B
		// �ŁI�f�X�N���v�^�[�q�[�v�͂��̃f�X�N���v�^�[���L�^���Ă������������m�ۂ���
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//���\�[�X�̎��
		desc.NumDescriptors = m_iBackBufferSize;//RT�̐������������m��
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//RT�̏ꍇ�͂���

		auto hr = m_pDevice->CreateDescriptorHeap(
			&desc, IID_PPV_ARGS(m_pRTVDescriptorHeap.ReleaseAndGetAddressOf())
		);
		if (FAILED(hr)) { throw std::runtime_error("CreateDescriptorHeap Failed."); }
		// �������̃T�C�Y���擾
		// �������ɂ���f�X�N���v�^�[�ɃA�N�Z�X����Ƃ��Ɏg���܂�
		m_RTVDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	for (std::uint32_t i = 0; i < m_iBackBufferSize; ++i)
	{
		// �X���b�v�`�F�C������o�b�N�o�b�t�@���擾���Ă���
		auto hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_RenderTargets[i].GetAddressOf()));
		if (FAILED(hr)) { throw std::runtime_error("IDGXSwapChain::GetBuffer Failed"); }
		m_RenderTargets[i]->SetName(L"Device::RenderTarget");
		// �����_�[�^�[�Q�b�g�r���[�̃f�X�N���v�^�[�����
		// ���̓��e���f�X�N���v�^�[�q�[�v�ŗp�ӂ����������ɏ������܂��
		D3D12_RENDER_TARGET_VIEW_DESC desc{};
		desc.Format = m_dfBackBufferFormat;//RT�̃t�H�[�}�b�g(�o�b�N�o�b�t�@�Ɠ���)
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;//2�����e�N�X�`��

		// ��ɍ����RT�̃f�X�N���v�^�q�[�v���珑�����ރA�h���X�����߂�
		CD3DX12_CPU_DESCRIPTOR_HANDLE RTVDescriptor(
			m_pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), i, m_RTVDescriptorSize);
		//	�����GPU��RT�ɃA�N�Z�X�ł���悤�ɂȂ��
		m_pDevice->CreateRenderTargetView(m_RenderTargets[i].Get(), &desc, RTVDescriptor);
		// �X���b�v�`�F�C������g���Ă���o�b�N�o�b�t�@�̃C���f�N�X��������Ă���
		m_iBackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	}
#pragma endregion
}
