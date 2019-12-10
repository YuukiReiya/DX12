//
// Game.h
//

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#pragma region 前方宣言
class TextRenderer;
#pragma endregion


// A basic game implementation that creates a D3D12 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify {
public:
	Game() noexcept(false);
	~Game();

	// Initialization and management
	void Initialize(HWND window, int width, int height);

	// Basic game loop
	void Tick();

	// IDeviceNotify
	virtual void OnDeviceLost() override;
	virtual void OnDeviceRestored() override;

	// Messages
	void OnActivated();
	void OnDeactivated();
	void OnSuspending();
	void OnResuming();
	void OnWindowMoved();
	void OnWindowSizeChanged(int width, int height);

	// Properties
	void GetDefaultSize(int& width, int& height) const;

private:
	void Update(DX::StepTimer const& timer);
	void Render();

	void Clear();

	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();

	// Device resources.
	std::unique_ptr<DX::DeviceResources> m_deviceResources;

	// Rendering loop timer.
	DX::StepTimer m_timer;

#pragma region 変数宣言
	std::unique_ptr<TextRenderer>textRenderer_;
	// imguiが内部でSRV確保に使うので作成しておく
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
#pragma endregion

};
