#pragma once

#include "d3d12device.h"

class D3D12Texture final : public VgTexture_t
{
public:
	~D3D12Texture();

	void* GetApiObject() const override { return _resource.Get(); }
	void SetName(const char* name) override { _resource->SetName(ConvertToWideString(name).data()); }
	D3D12Device* Device() const override { return _device; }
	const VgTextureDesc& Desc() const override { return _desc; }
	ComPtr<ID3D12Resource> Resource() const { return _resource; }
	bool OwnedBySwapChain() const override { return _allocation == nullptr; }

	uint32_t CreateAttachmentView(const VgAttachmentViewDesc& desc) override;
	uint32_t CreateView(const VgTextureViewDesc& desc) override;
	void DestroyViews() override;

private:
	D3D12Device* _device;
	VgTextureDesc _desc;

	ComPtr<ID3D12Resource> _resource;
	ComPtr<D3D12MA::Allocation> _allocation;

	friend D3D12Device;
	D3D12Texture(D3D12Device& device, const VgTextureDesc& desc);

	friend class D3D12SwapChain;
	D3D12Texture(D3D12SwapChain& swapChain, ComPtr<ID3D12Resource> resource);
};