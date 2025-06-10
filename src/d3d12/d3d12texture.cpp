#include "d3d12texture.h"
#include "d3d12descriptor_manager.h"
#include "d3d12swap_chain.h"
#include <agilitysdk/d3dx12/d3dx12.h>

D3D12Texture::~D3D12Texture()
{
	_device->DescriptorManager().FreeTextureViews(this);
	_device->GetMemoryStatistics().used_vram -= _allocation
		? _allocation->GetSize()
		: (_desc.width * _desc.height * FormatSizeBytes(_desc.format));
	_device->GetMemoryStatistics().num_textures--;
}

uint32_t D3D12Texture::CreateAttachmentView(const VgAttachmentViewDesc& desc)
{
	if (!FormatIsDepthStencil(desc.format))
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtv = {
			.Format = FormatToDXGIFormat(desc.format)
		};

		switch (desc.type)
		{
		case VG_TEXTURE_ATTACHMENT_VIEW_TYPE_1D:
			rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
			rtv.Texture1D = { desc.mip };
			break;
		case VG_TEXTURE_ATTACHMENT_VIEW_TYPE_1D_ARRAY:
			rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
			rtv.Texture1DArray = { desc.mip, desc.base_array_layer, desc.array_layers };
			break;
		case VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D:
			rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtv.Texture2D = { desc.mip, 0 };
			break;
		case VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_ARRAY:
			rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			rtv.Texture2DArray = { desc.mip, desc.base_array_layer, desc.array_layers, 0 };
			break;
		case VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_MS:
			rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
			rtv.Texture2DMS = {};
			break;
		case VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_MS_ARRAY:
			rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
			rtv.Texture2DMSArray = { desc.base_array_layer, desc.array_layers };
			break;
		case VG_TEXTURE_ATTACHMENT_VIEW_TYPE_3D:
			rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
			rtv.Texture3D = { desc.mip, desc.base_array_layer, desc.array_layers };
			break;
		default:
			rtv.ViewDimension = D3D12_RTV_DIMENSION_UNKNOWN;
			break;
		}

		auto& heap = _device->DescriptorManager().RTVHeap();
		uint32_t index = heap.RequestDescriptor();
		_device->Device()->CreateRenderTargetView(_resource.Get(), &rtv, heap.GetCPUDescriptorHandle(index));
		_device->DescriptorManager().RegisterRTV(this, index);
		return index;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC dsv{
		.Format = FormatToDXGIFormat(desc.format),
		.Flags = D3D12_DSV_FLAG_NONE
	};

	switch (desc.type)
	{
	case VG_TEXTURE_ATTACHMENT_VIEW_TYPE_1D:
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
		dsv.Texture1D = { desc.mip };
		break;
	case VG_TEXTURE_ATTACHMENT_VIEW_TYPE_1D_ARRAY:
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
		dsv.Texture1DArray = { desc.mip, desc.base_array_layer, desc.array_layers };
		break;
	case VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D:
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv.Texture2D = { desc.mip };
		break;
	case VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_ARRAY:
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsv.Texture2DArray = { desc.mip, desc.base_array_layer, desc.array_layers };
		break;
	case VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_MS:
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
		dsv.Texture2DMS = {};
		break;
	case VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_MS_ARRAY:
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
		dsv.Texture2DMSArray = { desc.base_array_layer, desc.array_layers };
		break;
	}
	auto& heap = _device->DescriptorManager().DSVHeap();
	uint32_t index = heap.RequestDescriptor();
	_device->Device()->CreateDepthStencilView(_resource.Get(), &dsv, heap.GetCPUDescriptorHandle(index));
	_device->DescriptorManager().RegisterDSV(this, index);
	return index;
}

constexpr D3D12_SHADER_COMPONENT_MAPPING MapComponent(VgComponentMapping mapping, uint32_t number)
{
	switch (mapping)
	{
	case VG_COMPONENT_MAPPING_IDENTITY:
		switch (number) {
		case 0: return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0;
		case 1: return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1;
		case 2: return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2;
		case 3: return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3;
		}
	case VG_COMPONENT_MAPPING_ZERO: return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0;
	case VG_COMPONENT_MAPPING_ONE: return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1;
	case VG_COMPONENT_MAPPING_R: return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0;
	case VG_COMPONENT_MAPPING_G: return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1;
	case VG_COMPONENT_MAPPING_B: return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2;
	case VG_COMPONENT_MAPPING_A: return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3;
	default: return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0;
	}
}

constexpr uint32_t MakeSwizzle(VgComponentSwizzle swizzle)
{
	const auto r = MapComponent(swizzle.r, 0);
	const auto g = MapComponent(swizzle.g, 1);
	const auto b = MapComponent(swizzle.b, 2);
	const auto a = MapComponent(swizzle.a, 3);
	return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(r, g, b, a);
}

uint32_t D3D12Texture::CreateView(const VgTextureViewDesc& desc)
{
	auto& heap = _device->DescriptorManager().ResourceHeap();
	uint32_t index = heap.RequestDescriptor();
	_device->DescriptorManager().RegisterTextureView(this, index);

	// TODO: look into
	uint32_t planeSlice = 0;

	if (desc.descriptor_type == VG_TEXTURE_DESCRIPTOR_TYPE_SRV)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srv = {
			.Format = FormatToDXGIFormat(desc.format),
			.Shader4ComponentMapping = MakeSwizzle(desc.components)
		};

		switch (desc.type)
		{
		case VG_TEXTURE_VIEW_TYPE_1D:
			srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
			srv.Texture1D = { desc.base_mip_level, desc.mip_levels, 0 };
			break;
		case VG_TEXTURE_VIEW_TYPE_1D_ARRAY:
			srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
			srv.Texture1DArray = { desc.base_mip_level, desc.mip_levels, desc.base_array_layer, desc.array_layers, 0 };
			break;
		case VG_TEXTURE_VIEW_TYPE_2D:
			srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srv.Texture2D = { desc.base_mip_level, desc.mip_levels, planeSlice, 0 };
			break;
		case VG_TEXTURE_VIEW_TYPE_2D_ARRAY:
			srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srv.Texture2DArray = { desc.base_mip_level, desc.mip_levels, desc.base_array_layer, desc.array_layers, planeSlice, 0 };
			break;
		case VG_TEXTURE_VIEW_TYPE_2D_MS:
			srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
			srv.Texture2DMS = {};
			break;
		case VG_TEXTURE_VIEW_TYPE_2D_MS_ARRAY:
			srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
			srv.Texture2DMSArray = { desc.base_array_layer, desc.array_layers };
			break;
		case VG_TEXTURE_VIEW_TYPE_3D:
			srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srv.Texture3D = { desc.base_mip_level, desc.mip_levels, 0 };
			break;
		case VG_TEXTURE_VIEW_TYPE_CUBE:
			srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srv.TextureCube = { desc.base_mip_level, desc.mip_levels, 0 };
			break;
		case VG_TEXTURE_VIEW_TYPE_CUBE_ARRAY:
			srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
			srv.TextureCubeArray = { desc.base_mip_level, desc.mip_levels, desc.base_array_layer, desc.array_layers / 6, 0 };
			break;
		}

		_device->Device()->CreateShaderResourceView(_resource.Get(), &srv, heap.GetCPUDescriptorHandle(index));
		return index;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC uav = {
		.Format = FormatToDXGIFormat(desc.format)
	};

	switch (desc.type)
	{
	case VG_TEXTURE_VIEW_TYPE_1D:
		uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
		uav.Texture1D = { desc.base_mip_level };
		break;
	case VG_TEXTURE_VIEW_TYPE_1D_ARRAY:
		uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
		uav.Texture1DArray = { desc.base_mip_level, desc.base_array_layer, desc.array_layers };
		break;
	case VG_TEXTURE_VIEW_TYPE_2D:
		uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uav.Texture2D = { desc.base_mip_level, planeSlice };
		break;
	case VG_TEXTURE_VIEW_TYPE_2D_ARRAY:
		uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uav.Texture2DArray = { desc.base_mip_level, desc.base_array_layer, desc.array_layers, planeSlice };
		break;
	case VG_TEXTURE_VIEW_TYPE_2D_MS:
		uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMS;
		uav.Texture2DMS = {};
		break;
	case VG_TEXTURE_VIEW_TYPE_2D_MS_ARRAY:
		uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
		uav.Texture2DMSArray = { desc.base_array_layer, desc.array_layers };
		break;
	case VG_TEXTURE_VIEW_TYPE_3D:
		uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		uav.Texture3D = { desc.base_mip_level, 0, static_cast<UINT>(-1) };
		break;
	}

	_device->Device()->CreateUnorderedAccessView(_resource.Get(), nullptr, &uav, heap.GetCPUDescriptorHandle(index));
	return index;
}

void D3D12Texture::DestroyViews()
{
	_device->DescriptorManager().FreeTextureViews(this);
}

constexpr D3D12_RESOURCE_FLAGS TextureUsageToResourceFlags(VgTextureUsageFlags usage)
{
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

	if (!(usage & VG_TEXTURE_USAGE_SHADER_RESOURCE)) flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	if (usage & VG_TEXTURE_USAGE_UNORDERED_ACCESS) flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	if (usage & VG_TEXTURE_USAGE_COLOR_ATTACHMENT) flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (usage & VG_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT) flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (usage & VG_TEXTURE_USAGE_ALLOW_SIMULTANEOUS_ACCESS) flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;

	if (!(flags & (D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY
		| D3D12_RESOURCE_FLAG_VIDEO_ENCODE_REFERENCE_ONLY)))
		flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

	return flags;
}

constexpr D3D12_RESOURCE_DIMENSION TextureTypeToResourceDimension(VgTextureType type)
{
	switch (type)
	{
	case VG_TEXTURE_TYPE_1D: return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	case VG_TEXTURE_TYPE_2D: return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	case VG_TEXTURE_TYPE_3D: return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	default: return D3D12_RESOURCE_DIMENSION_UNKNOWN;
	}
}

constexpr D3D12_TEXTURE_LAYOUT TextureTilingToLayout(VgTextureTiling tiling)
{
	switch (tiling)
	{
	case VG_TEXTURE_TILING_OPTIMAL: return D3D12_TEXTURE_LAYOUT_UNKNOWN;
	case VG_TEXTURE_TILING_LINEAR: return D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	default: return D3D12_TEXTURE_LAYOUT_UNKNOWN;
	}
}

D3D12Texture::D3D12Texture(D3D12Device& device, const VgTextureDesc& desc) : _device(&device), _desc(desc)
{
	auto dimension = TextureTypeToResourceDimension(desc.type);
	auto format = FormatToDXGIFormat(desc.format);
	auto resourceUsageFlags = TextureUsageToResourceFlags(desc.usage);

	auto textureDesc = D3D12_RESOURCE_DESC1{
		.Dimension = dimension,
		.Alignment = 0,
		.Width = desc.width,
		.Height = desc.height,
		.DepthOrArraySize = static_cast<UINT16>(desc.depth_or_array_layers),
		.MipLevels = static_cast<UINT16>(desc.mip_levels),
		.Format = format,
		.SampleDesc = { SampleCount(desc.sample_count), 0 },
		.Layout = TextureTilingToLayout(desc.tiling),
		.Flags = resourceUsageFlags,
		.SamplerFeedbackMipRegion = {
			.Width = 0,
			.Height = 0,
			.Depth = 0
		}
	};

	D3D12MA::ALLOCATION_DESC allocationDesc = {
		.HeapType = HeapTypeToD3D12HeapType(desc.heap_type)
	};

	auto layout = VgTextureLayoutToBarrierLayout(desc.initial_layout);
	ThrowOnError(device.Allocator()->CreateResource3(&allocationDesc, &textureDesc,
		layout, nullptr, 0, nullptr, &_allocation, IID_PPV_ARGS(&_resource)));

	_device->GetMemoryStatistics().used_vram += _allocation->GetSize();
	_device->GetMemoryStatistics().num_textures++;
}

D3D12Texture::D3D12Texture(D3D12SwapChain& swapChain, ComPtr<ID3D12Resource> resource)
	: _device(swapChain.Device()), _resource(resource), _allocation(nullptr)
{
	_desc = {
		.type = VG_TEXTURE_TYPE_2D,
		.format = swapChain.Desc().format,
		.width = swapChain.Desc().width,
		.height = swapChain.Desc().height,
		.depth_or_array_layers = 1,
		.mip_levels = 1,
		.sample_count = VG_SAMPLE_COUNT_1,
		.usage = VG_TEXTURE_USAGE_COLOR_ATTACHMENT,
		.tiling = VG_TEXTURE_TILING_OPTIMAL,
		.initial_layout = VG_TEXTURE_LAYOUT_COLOR_ATTACHMENT,
		.heap_type = VG_HEAP_TYPE_GPU
	};
	_device->GetMemoryStatistics().used_vram += _desc.width * _desc.height * FormatSizeBytes(_desc.format);
	_device->GetMemoryStatistics().num_textures++;
}