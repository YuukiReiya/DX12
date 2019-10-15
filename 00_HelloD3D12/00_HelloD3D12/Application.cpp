#include "pch.h"
#include "Application.hpp"
#include "Device.hpp"
#include "SimplePolygon.hpp"

dxapp::Application::Application() {}

dxapp::Application::~Application()
{
	Teardown();
}

void dxapp::Application::Setup(HWND hWnd, std::uint32_t width, std::uint32_t height)
{
	m_hWnd = hWnd;
	m_pDevice = std::make_unique<DX12::Device>();
	m_pDevice->Setup(hWnd, width, height);
	m_pSimplePolygon = std::make_unique<SimplePolygon>();
	m_pSimplePolygon->Setup(m_pDevice->GetDevice());
}

void dxapp::Application::Execute()
{
	Update();
	Render();
}

void dxapp::Application::Teardown()
{
	m_pDevice->WaitForGPU();
}

void dxapp::Application::Update()
{
}

void dxapp::Application::Render()
{
	//�`�揈��
	m_pDevice->PrepareRendering();
	ClearRTV();
	m_pSimplePolygon->MakeCommandList(m_pDevice->GetGraphicsCommandList());
	//�`��X�V����
	m_pDevice->Present();
}

void dxapp::Application::ClearRTV()
{
	// �����_�[�^�[�Q�b�g(���̃t���[���ŕ`��Ɏg�p����o�b�t�@)�ɕ`�悵�Ă���
	// ���e���N���A(�h��Ԃ��C���[�W)���܂�
	auto commandList = m_pDevice->GetGraphicsCommandList();

	// �t���[�����ƂɃo�b�N�o�b�t�@���؂�ւ�̂ŁA���̃t���[���ł̃����_�[�^�[�Q�b�g�����炤
	auto RTV = m_pDevice->GetCurrentRTV();
	// �R�}���h���X�g�ɂ��̃t���[���ł̃����_�[�^�[�Q�b�g��ݒ肷��
	commandList->OMSetRenderTargets(
		1,			//�ݒ肷��RT�̐��B�Ƃ��1��
		&RTV,	//RT�n���h��
		FALSE,	//�Ƃ��FALSE
		nullptr	//�Ƃ�܃k���ۂ�OK
	);
	// �w�肵�������_�[�^�[�Q�b�g���A����̐F�œh��Ԃ�
	commandList->ClearRenderTargetView(
		RTV,										//�h��Ԃ�RTV
		DirectX::Colors::CadetBlue,		//�h��Ԃ��F
		0,											//�O�ł�k
		nullptr									//�k���ۂł�k
	);
	// ����̓����_�[�^�[�Q�b�g�̃N���A�Ƃ͒��ڊ֌W�͂Ȃ��̂�����
	// �L�q���Ă����̂ɂ͂��傤�ǂ����̂ł����ɓ���܂��B
	auto vp = m_pDevice->GetScreeViewport();
	commandList->RSSetViewports(1, &vp);

	auto scissorRect = m_pDevice->GetScissorRect();
	commandList->RSSetScissorRects(1, &scissorRect);
}
