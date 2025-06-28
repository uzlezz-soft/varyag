#pragma once

#include "d3d12device.h"
#include "d3d12texture.h"
#include <memory>
#include <vector>

#if VG_D3D12_SUPPORTED

class D3D12SwapChain final : public VgSwapChain_t
{
public:
	~D3D12SwapChain();

	void* GetApiObject() const override { return _swapChain.Get(); }
	D3D12Device* Device() const override { return _device; }
	const VgSwapChainDesc& Desc() const override { return _desc; }

	uint32_t AcquireNextImage() override;
	D3D12Texture* GetBackBuffer(uint32_t index) override;
	void Present(uint32_t numWaitFences, VgFenceOperation* waitFences) override;

private:
	D3D12Device* _device;
	VgSwapChainDesc _desc;
	ComPtr<IDXGISwapChain1> _swapChain;

	vg::Vector<D3D12Texture*> _backBuffers;
	uint32_t _imageIndex;

	friend D3D12Device;

	D3D12SwapChain(D3D12Device& device, const VgSwapChainDesc& desc);
};

#endif