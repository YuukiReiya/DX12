#include "pch.h"
#include "TextureManager.hpp"
#include "Device.hpp"

//WICTextureLoader
#include "External/WICTextureLoader12.h"

using namespace dxapp;

#pragma region Impl
class TextureManager::Impl {
	// �����N���X�́A�N���X��::�����N���X���Œ�`�ł��܂��B
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
	 * @brief ID3D12Resource�̎擾
	 * @return �A�Z�b�g�����݂����ID3D12Resource*���A�Ȃ����nullptr��Ԃ�
	 */
	Microsoft::WRL::ComPtr<ID3D12Resource> resource(
		const std::string& assetName) {
		const auto end = std::end(textures_);  // �A�z�z��̏I�����擾(�܂�)
		const auto ret = textures_.find(assetName);  // �A�Z�b�g�ŒT����...
		// �A�z�z��Ƀf�[�^���Ȃ��ƁAstd::end(textures_)���Ԃ��Ă���
		if (ret == end) {
			return nullptr;  // �Ȃ�����
		}
		return ret->second->resource;  // ��������
	}

	//! �e�N�X�`���Ǘ��p�̍\����
	struct Texture {
		std::wstring fileName;                            //!< �t�@�C����
		std::string assetName;                            //<! �A�Z�b�g��
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;  //!< �e�N�X�`���̎���
	};
	// �e�N�X�`�������[�h����ƒ��_�o�b�t�@�ȂǂƓ�����ID3D12Resource�ɂȂ�
	// ���\�[�X��GPU����̓������̉�ɂ��������Ȃ����Ă��Ƃ���

	//! �e�N�X�`�����i�[���Ă����R���e�i
	std::unordered_map<std::string, std::unique_ptr<Texture>> textures_{};
	// unordered_map�͂�����A�z�z��
	// unordered_map<�L�[�̌^, �i�[�������^>�ɂȂ�B
	// �L�[�̒l�͏d���ł��Ȃ��̂ŁA�����e�N�X�`������ȏネ�[�h���Ȃ��d�g�݂Ɏg����
};
#pragma endregion

dxapp::TextureManager::TextureManager() :impl_(new Impl()) {}

dxapp::TextureManager::~TextureManager(){}

bool dxapp::TextureManager::LoadWICTextureFromFile(Device* device, const std::wstring& fileName, const std::string& assetName)
{
	// �e�N�X�`���̑��݃`�F�b�N
	if (impl_->resource(assetName)) {
		return true;
	}

	// ���[�h���ɕK�v�ȕϐ����m��
	ID3D12Resource* resource;
	std::unique_ptr<uint8_t[]> decodedData{};
	D3D12_SUBRESOURCE_DATA subresource{};

	// LoadWICTextureFromFile��WIC��������bmp,png,jpg�Ƃ����ǂ߂�
	// �Ƃ肠�����t�@�C�����烁�����ɓǂݍ���
	auto hr = DirectX::LoadWICTextureFromFile(
		device->device(),  // �f�o�C�X
		fileName.c_str(),  // �t�@�C����
		&resource,     // D3D�Ŏg�p�\�ɂȂ����e�N�X�`���f�[�^
		decodedData,   // �t�@�C������ǂݍ��܂ꂽ�o�C�g�f�[�^
		subresource);  // �e�N�X�`���f�[�^�̃A�h���X�ƃf�[�^�̕��т��Ԃ��Ă���

	if (FAILED(hr)) {
		return false;
	}

	// VRAM�ɑ���T�C�Y���擾
	const auto uploadBufferSize = GetRequiredIntermediateSize(resource, 0, 1);

	auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeap;

	// �A�b�v���[�h��̃��������m��
	device->device()->CreateCommittedResource(
		&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(uploadHeap.GetAddressOf()));

	// �A�b�v���[�h�R�}���h�̐���
	// �e�N�X�`���̓]���̓R�}���h���X�g���o�R����K�v������
	auto cl = device->CreateNewGraphicsCommandList();
	{
		// VRAM�ւ̓]���R�}���h���s
		// d3dx12�̃w���p�[�֐��𗘗p���܂��B�֗��֗�
		UpdateSubresources(
			cl.Get(),
			resource,          // �e�N�X�`�����\�[�X
			uploadHeap.Get(),  // �����
			0, 0, 1,           // �Ƃ肠�������̐ݒ��OK
			&subresource);  // ���[�h���ɓ���D3D12_SUBRESOURCE_DATA��ݒ�

		// �]���\����s�N�Z���V�F�[�_�ɂȂ�܂ő҂�
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			resource, D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cl->ResourceBarrier(1, &barrier);
	}

	// �]���J�n
	// ���R����1�t�@�C�������s������A�܂Ƃ܂����P�ʂ�Exex�����ق���������
	// ���܂͊ȒP�̂��߂�1������Ă��܂��B
	{
		cl->Close();
		ID3D12CommandList* lists[]{ cl.Get() };
		device->commandQueue()->ExecuteCommandLists(1, lists);

		// �R�}���h���X�g�œ]������̂�GPU�̏����҂������܂�
		{
			// Device::WaitForGPU��Device�N���X���߂̂��̂Ȃ̂Ŏ��O�ō��
			Microsoft::WRL::ComPtr<ID3D12Fence> fence;
			UINT64 value = 0;
			device->device()->CreateFence(
				value++, D3D12_FENCE_FLAG_NONE,
				IID_PPV_ARGS(fence.ReleaseAndGetAddressOf()));

			Microsoft::WRL::Wrappers::Event fenceEvent{};
			fenceEvent.Attach(
				CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
			if (SUCCEEDED(device->commandQueue()->Signal(fence.Get(), value))) {
				if (SUCCEEDED(fence->SetEventOnCompletion(value, fenceEvent.Get()))) {
					WaitForSingleObjectEx(fenceEvent.Get(), INFINITE, FALSE);
				}
			}
		}

		// �I������̂Ńf�[�^���i�[
		auto tex = std::make_unique<Impl::Texture>();
		tex->fileName = fileName;
		tex->assetName = assetName;
		tex->resource.Attach(resource);
		impl_->textures_.emplace(assetName, std::move(tex));
	}

	// ���Ȃ݂Ƀe�N�X�`���̍폜�͗p�ӂ��ĂȂ�
	return true;
}

Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::texture(
	const std::string& assetName) {
	return impl_->resource(assetName);
}