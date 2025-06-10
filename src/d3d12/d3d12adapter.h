#pragma once

#include "d3d12device.h"
#include <vector>
#include <memory>
#include <dxcore.h>

class D3D12Adapter final : public VgAdapter_t
{
public:
	static vg::Vector<VgAdapter_t*> CollectAdapters();

	D3D12Adapter(ComPtr<IDXCoreAdapter> adapter, ComPtr<IDXGIFactory6> factory);
	~D3D12Adapter();

	void* GetApiObject() const override { return _adapter.Get(); }
	ComPtr<IDXGIFactory4> Factory() const { return _factory; }
	ComPtr<IDXGIAdapter> Adapter() const { return _adapter; }
	VgGraphicsApi Api() const override { return VG_GRAPHICS_API_D3D12; }
	const VgAdapterProperties& GetProperties() const override { return _properties; }

	D3D12Device* CreateDevice(VgInitFlags initFlags) override;

private:
	ComPtr<IDXGIFactory4> _factory;
	ComPtr<IDXCoreAdapter> _coreAdapter;
	ComPtr<IDXGIAdapter1> _adapter;
	VgAdapterProperties _properties;
};