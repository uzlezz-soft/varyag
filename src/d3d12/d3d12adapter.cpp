#include "d3d12adapter.h"

#if VG_D3D12_SUPPORTED
#include <agilitysdk/d3dx12/d3dx12.h>

#pragma comment(lib, "dxcore.lib")

// why?...
extern "C" const GUID DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS = { 0x0c9ece4d, 0x2f6e, 0x4f01, { 0x8c, 0x96, 0xe8, 0x9e, 0x33, 0x1b, 0x47, 0xb1 } };

vg::Vector<VgAdapter_t*> D3D12Adapter::CollectAdapters()
{
    vg::Vector<VgAdapter_t*> adapters;

    ComPtr<IDXGIFactory6> dxgiFactory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory)))) return {};

    ComPtr<IDXCoreAdapterFactory> adapterFactory;
    if (FAILED(DXCoreCreateAdapterFactory(IID_PPV_ARGS(&adapterFactory)))) return {};

    ComPtr<IDXCoreAdapterList> coreAdapters;
    auto attribute = DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS;
    if (FAILED(adapterFactory->CreateAdapterList(1, &attribute, IID_PPV_ARGS(&coreAdapters)))) return {};

    std::array sortPreferences = { DXCoreAdapterPreference::Hardware, DXCoreAdapterPreference::HighPerformance };
    if (FAILED(coreAdapters->Sort(static_cast<uint32_t>(sortPreferences.size()), sortPreferences.data()))) return {};

    const auto count = coreAdapters->GetAdapterCount();
    for (uint32_t i = 0; i < count; ++i)
    {
        ComPtr<IDXCoreAdapter> adapter;
        if (FAILED(coreAdapters->GetAdapter(i, IID_PPV_ARGS(&adapter)))) continue;

        try {
            auto a = new((GetAllocator().Allocate<D3D12Adapter>())) D3D12Adapter(adapter, dxgiFactory);
            adapters.push_back(a);
        }
        catch (...) {}
    }

    return adapters;
}

static vg::String ConvertWCharToString(const wchar_t* wstr)
{
    if (!wstr) return {};
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0)
        return {};

    vg::String result(sizeNeeded, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result.data(), sizeNeeded, nullptr, nullptr);
    result.pop_back();
    return result;
}

D3D12Adapter::D3D12Adapter(ComPtr<IDXCoreAdapter> adapter, ComPtr<IDXGIFactory6> factory)
    : _factory(factory), _coreAdapter(adapter)
{
    LUID luid;
    ThrowOnError(adapter->GetProperty(DXCoreAdapterProperty::InstanceLuid, &luid));
    ThrowOnError(factory->EnumAdapterByLuid(luid, IID_PPV_ARGS(&_adapter)));

    ComPtr<ID3D12Device> device;
    if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device))))
        throw std::runtime_error("Unsuitable adapter");

    CD3DX12FeatureSupport features;
    features.Init(device.Get());

    if (features.HighestShaderModel() < D3D_SHADER_MODEL_6_6
        || features.ResourceBindingTier() < D3D12_RESOURCE_BINDING_TIER_3)
        throw std::runtime_error("Unsuitable adapter");

    DXGI_ADAPTER_DESC1 adapterDesc;
    _adapter->GetDesc1(&adapterDesc);

    bool isHardware, isIntegrated;
    ThrowOnError(adapter->GetProperty(DXCoreAdapterProperty::IsHardware, &isHardware));
    ThrowOnError(adapter->GetProperty(DXCoreAdapterProperty::IsIntegrated, &isIntegrated));

    _properties = {
        .type = isHardware ? (isIntegrated ? VG_ADAPTER_TYPE_INTEGRATED : VG_ADAPTER_TYPE_DISCRETE) : VG_ADAPTER_TYPE_SOFTWARE,
        .dedicated_vram = adapterDesc.DedicatedVideoMemory,
        .dedicated_ram = adapterDesc.DedicatedSystemMemory,
        .shared_ram = adapterDesc.SharedSystemMemory,
        .mesh_shaders = features.MeshShaderTier() >= D3D12_MESH_SHADER_TIER_1,
        .hardware_ray_tracing = features.RaytracingTier() >= D3D12_RAYTRACING_TIER_1_1
    };

    auto name = ConvertWCharToString(adapterDesc.Description);
    memcpy(_properties.name, name.data(), name.size());
}

D3D12Adapter::~D3D12Adapter()
{
}

D3D12Device* D3D12Adapter::CreateDevice(VgInitFlags initFlags)
{
    return new(GetAllocator().Allocate<D3D12Device>()) D3D12Device(*this, initFlags);
}
#endif