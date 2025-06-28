#include "d3d12swap_chain.h"
#include "d3d12adapter.h"

#if VG_D3D12_SUPPORTED

D3D12SwapChain::~D3D12SwapChain()
{
	for (auto backBuffer : _backBuffers)
	{
		GetAllocator().Delete(backBuffer);
	}
}

uint32_t D3D12SwapChain::AcquireNextImage()
{
	return _imageIndex;
}

D3D12Texture* D3D12SwapChain::GetBackBuffer(uint32_t index)
{
	return _backBuffers[index];
}

void D3D12SwapChain::Present(uint32_t numWaitFences, VgFenceOperation* waitFences)
{
	for (uint32_t i = 0; i < numWaitFences; i++)
	{
		_device->GetQueue(VG_QUEUE_GRAPHICS)->Wait(static_cast<ID3D12Fence*>(waitFences[i].fence), waitFences[i].value);
	}

	// TODO: look into querying support for tearing for VRR
	_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	_imageIndex = (_imageIndex + 1) % _backBuffers.size();
}

D3D12SwapChain::D3D12SwapChain(D3D12Device& device, const VgSwapChainDesc& desc)
	: _device(&device), _desc(desc)
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
		.Width = desc.width,
		.Height = desc.height,
		.Format = FormatToDXGIFormat(desc.format),
		.Stereo = false,
		.SampleDesc = {
			.Count = 1,
			.Quality = 0
		},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = desc.buffer_count,
		.Scaling = DXGI_SCALING_STRETCH,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
		.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
		.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING // TODO: query support?
	};

	ThrowOnError(static_cast<D3D12Adapter*>(device.Adapter())->Factory()->CreateSwapChainForHwnd(device.GetQueue(VG_QUEUE_GRAPHICS).Get(),
		reinterpret_cast<HWND>(desc.surface), &swapChainDesc, nullptr, nullptr, &_swapChain));

	_backBuffers.resize(desc.buffer_count);
	for (uint32_t i = 0; i < desc.buffer_count; i++)
	{
		ComPtr<ID3D12Resource> resource;
		ThrowOnError(_swapChain->GetBuffer(i, IID_PPV_ARGS(&resource)));
		_backBuffers[i] = new(GetAllocator().Allocate<D3D12Texture>()) D3D12Texture(*this, resource);
	}
}

#endif