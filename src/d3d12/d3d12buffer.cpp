#include "d3d12buffer.h"
#include "d3d12descriptor_manager.h"

#if VG_D3D12_SUPPORTED
#include <agilitysdk/d3dx12/d3dx12.h>

D3D12Buffer::~D3D12Buffer()
{
	_device->DescriptorManager().FreeBufferViews(this);
	_device->GetMemoryStatistics().used_vram -= _allocation->GetSize();
	_device->GetMemoryStatistics().num_buffers--;
}

uint32_t D3D12Buffer::CreateView(const VgBufferViewDesc& desc)
{
	auto& heap = _device->DescriptorManager().ResourceHeap();
	auto index = heap.RequestDescriptor();

	uint64_t elementSize = desc.element_size;
	if (desc.view_type == VG_BUFFER_VIEW_TYPE_BUFFER) elementSize = FormatSizeBytes(desc.format);
	else if (desc.view_type == VG_BUFFER_VIEW_TYPE_BYTE_ADDRESS_BUFFER) elementSize = 4;

	if (desc.descriptor_type == VG_BUFFER_DESCRIPTOR_TYPE_CBV)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
			.BufferLocation = _resource->GetGPUVirtualAddress() + desc.offset,
			.SizeInBytes = static_cast<UINT>(desc.size)
		};
		_device->Device()->CreateConstantBufferView(&cbvDesc, heap.GetCPUDescriptorHandle(index));
	}
	else if (desc.descriptor_type == VG_BUFFER_DESCRIPTOR_TYPE_SRV)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = desc.offset / elementSize; // Structured uses element size
		srvDesc.Buffer.NumElements = static_cast<UINT>(desc.size / elementSize);
		srvDesc.Buffer.StructureByteStride = desc.view_type == VG_BUFFER_VIEW_TYPE_STRUCTURED_BUFFER ? static_cast<UINT>(desc.element_size) : 0;
		srvDesc.Buffer.Flags = desc.view_type == VG_BUFFER_VIEW_TYPE_BYTE_ADDRESS_BUFFER ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;
		if (desc.view_type == VG_BUFFER_VIEW_TYPE_BUFFER)
		{
			srvDesc.Format = FormatToDXGIFormat(desc.format);
		}
		else if (desc.view_type == VG_BUFFER_VIEW_TYPE_BYTE_ADDRESS_BUFFER)
		{
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		}
		_device->Device()->CreateShaderResourceView(_resource.Get(), &srvDesc, heap.GetCPUDescriptorHandle(index));
	}
	else if (desc.descriptor_type == VG_BUFFER_DESCRIPTOR_TYPE_UAV)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = desc.offset / elementSize; // Structured uses element size
		uavDesc.Buffer.NumElements = static_cast<UINT>(desc.size / elementSize);
		uavDesc.Buffer.StructureByteStride = desc.view_type == VG_BUFFER_VIEW_TYPE_STRUCTURED_BUFFER ? static_cast<UINT>(desc.element_size) : 0;
		uavDesc.Buffer.Flags = desc.view_type == VG_BUFFER_VIEW_TYPE_BYTE_ADDRESS_BUFFER ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;
		if (desc.view_type == VG_BUFFER_VIEW_TYPE_BUFFER)
		{
			uavDesc.Format = FormatToDXGIFormat(desc.format);
		}
		_device->Device()->CreateUnorderedAccessView(_resource.Get(), nullptr, &uavDesc, heap.GetCPUDescriptorHandle(index));
	}
	_device->DescriptorManager().RegisterBufferView(this, index);
	return index;
}

void D3D12Buffer::DestroyViews()
{
	_device->DescriptorManager().FreeBufferViews(this);
}

void* D3D12Buffer::Map()
{
	if (_mapped) return _mapped;

	ThrowOnError(_resource->Map(0, nullptr, reinterpret_cast<void**>(&_mapped)));
	return _mapped;
}

void D3D12Buffer::Unmap()
{
	if (_mapped)
	{
		_resource->Unmap(0, nullptr);
		_mapped = nullptr;
	}
}

constexpr D3D12_RESOURCE_FLAGS BufferUsageToResourceFlags(VgBufferUsage usage)
{
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
	if (usage == VG_BUFFER_USAGE_GENERAL)
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	return flags;
}

D3D12Buffer::D3D12Buffer(D3D12Device& device, const VgBufferDesc& desc) : _device(&device), _desc(desc)
{
	auto resourceUsageFlags = BufferUsageToResourceFlags(desc.usage);
	if (desc.heap_type != VG_HEAP_TYPE_GPU)
		resourceUsageFlags &= ~D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	auto bufferDesc = CD3DX12_RESOURCE_DESC1::Buffer(desc.size, resourceUsageFlags);

	D3D12MA::ALLOCATION_DESC allocationDesc = {
		.HeapType = HeapTypeToD3D12HeapType(desc.heap_type)
	};
	
	const auto layout = D3D12_BARRIER_LAYOUT_UNDEFINED;
	ThrowOnError(device.Allocator()->CreateResource3(&allocationDesc, &bufferDesc,
		layout, nullptr, 0, nullptr, &_allocation, IID_PPV_ARGS(&_resource)));

	_device->GetMemoryStatistics().used_vram += _allocation->GetSize();
	_device->GetMemoryStatistics().num_buffers++;
}
#endif