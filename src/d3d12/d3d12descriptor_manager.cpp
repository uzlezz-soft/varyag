#include "d3d12descriptor_manager.h"

D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type,
	uint32_t descriptorCount, std::wstring_view name) : _device(&device), _descriptorCount(descriptorCount)
{
	const D3D12_DESCRIPTOR_HEAP_FLAGS flags = (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV || type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
		? D3D12_DESCRIPTOR_HEAP_FLAG_NONE : D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	const D3D12_DESCRIPTOR_HEAP_DESC desc = {
		.Type = type,
		.NumDescriptors = descriptorCount,
		.Flags = flags,
		.NodeMask = 0
	};
	ThrowOnError(_device->Device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_heap)));
	_heap->SetName(name.data());
	_descriptorSize = _device->Device()->GetDescriptorHandleIncrementSize(type);

	_cpuStart = _heap->GetCPUDescriptorHandleForHeapStart();
	if (flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		_gpuStart = _heap->GetGPUDescriptorHandleForHeapStart();
	else
		_gpuStart = {0};

	_freeSlots.resize(descriptorCount);
	for (int32_t i = descriptorCount - 1; i >= 0; i--)
	{
		_freeSlots[i] = descriptorCount - i - 1;
	}

	if (flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
	{
		_device->GetMemoryStatistics().used_vram += _descriptorSize * descriptorCount;
	}
}

D3D12DescriptorHeap::~D3D12DescriptorHeap()
{
	if (_gpuStart.ptr != 0)
	{
		_device->GetMemoryStatistics().used_vram -= _descriptorSize * _descriptorCount;
	}
}

uint32_t D3D12DescriptorHeap::RequestDescriptor()
{
	std::scoped_lock lock(_access);
	if (_freeSlots.empty())
		throw std::runtime_error("No free descriptor slots");
	const auto slot = _freeSlots.back();
	_freeSlots.pop_back();
	return slot;
}

void D3D12DescriptorHeap::FreeDescriptor(uint32_t index)
{
	std::scoped_lock lock(_access);
	_freeSlots.push_back(index);
}

D3D12DescriptorManager::D3D12DescriptorManager(D3D12Device& device)
	: _device(&device),
	_resourceHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, NumResourceDescriptors, L"Resource Heap"),
	_samplerHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, NumSamplerDescriptors, L"Sampler Heap"),
	_rtvHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, NumRTVDescriptors, L"Render Target View Heap"),
	_dsvHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, NumDSVDescriptors, L"Depth Stencil View Heap")
{
}

D3D12DescriptorManager::~D3D12DescriptorManager()
{
}

void D3D12DescriptorManager::FreeTextureViews(D3D12Texture* texture)
{
	{
		auto range = _rtvs.equal_range(texture);
		for (auto it = range.first; it != range.second; it++)
		{
			_rtvHeap.FreeDescriptor(it->second);
		}
		_rtvs.erase(texture);
	}

	{
		auto range = _dsvs.equal_range(texture);
		for (auto it = range.first; it != range.second; it++)
		{
			_dsvHeap.FreeDescriptor(it->second);
		}
		_dsvs.erase(texture);
	}

	{
		auto range = _textureViews.equal_range(texture);
		for (auto it = range.first; it != range.second; it++)
		{
			_resourceHeap.FreeDescriptor(it->second);
		}
		_textureViews.erase(texture);
	}
}
