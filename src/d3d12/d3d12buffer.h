#pragma once

#include "d3d12device.h"
#include <vector>
#include <mutex>

#if VG_D3D12_SUPPORTED

class D3D12Buffer final : public VgBuffer_t
{
public:
	~D3D12Buffer();

	void* GetApiObject() const override { return _resource.Get(); }
	void SetName(const char* name) override { _resource->SetName(ConvertToWideString(name).data()); }
	D3D12Device* Device() const override { return _device; }
	const VgBufferDesc& Desc() const override { return _desc; }

	ComPtr<ID3D12Resource> Resource() const { return _resource; }
	ComPtr<D3D12MA::Allocation> Allocation() const { return _allocation; }

	uint32_t CreateView(const VgBufferViewDesc& desc) override;
	void DestroyViews() override;
	void* Map() override;
	void Unmap() override;

private:
	D3D12Device* _device;
	VgBufferDesc _desc;

	ComPtr<ID3D12Resource> _resource;
	ComPtr<D3D12MA::Allocation> _allocation;
	void* _mapped{ nullptr };

	friend D3D12Device;

	D3D12Buffer(D3D12Device& device, const VgBufferDesc& desc);
};

#endif