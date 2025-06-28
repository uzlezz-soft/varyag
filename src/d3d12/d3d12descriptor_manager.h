#pragma once

#include "d3d12device.h"
#include "d3d12buffer.h"
#include "d3d12texture.h"
#include <unordered_map>
#include <string_view>
#include <vector>
#include <mutex>

#if VG_D3D12_SUPPORTED

class D3D12DescriptorHeap
{
public:
	D3D12DescriptorHeap(D3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptorCount, std::wstring_view name);
	~D3D12DescriptorHeap();

	uint32_t RequestDescriptor();
	void FreeDescriptor(uint32_t index);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t slot) {
		auto handle = _cpuStart;
		handle.ptr += slot * _descriptorSize; 
		return handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t slot) {
		auto handle = _gpuStart;
		handle.ptr += slot * _descriptorSize;
		return handle;
	}

	ID3D12DescriptorHeap* Heap() const { return _heap.Get(); }

private:
	D3D12Device* _device;
	ComPtr<ID3D12DescriptorHeap> _heap;
	uint32_t _descriptorSize;
	uint32_t _descriptorCount;
	std::mutex _access;

	vg::Vector<uint32_t> _freeSlots;
	D3D12_CPU_DESCRIPTOR_HANDLE _cpuStart;
	D3D12_GPU_DESCRIPTOR_HANDLE _gpuStart;
};

class D3D12DescriptorManager
{
public:
	// See https://learn.microsoft.com/en-us/windows/win32/direct3d12/hardware-support?redirectedfrom=MSDN
	inline static constexpr uint32_t NumResourceDescriptors = 1'000'000;
	inline static constexpr uint32_t NumSamplerDescriptors = 2'048;

	inline static constexpr uint32_t NumRTVDescriptors = 50'000;
	inline static constexpr uint32_t NumDSVDescriptors = 50'000;

	D3D12DescriptorManager(D3D12Device& device);
	~D3D12DescriptorManager();

	D3D12DescriptorHeap& ResourceHeap() { return _resourceHeap; }
	D3D12DescriptorHeap& SamplerHeap() { return _samplerHeap; }
	D3D12DescriptorHeap& RTVHeap() { return _rtvHeap; }
	D3D12DescriptorHeap& DSVHeap() { return _dsvHeap; }

	void RegisterBufferView(D3D12Buffer* buffer, uint32_t descriptorIndex) { _bufferViews.insert({ buffer, descriptorIndex }); }
	void FreeBufferViews(D3D12Buffer* buffer)
	{
		auto range = _bufferViews.equal_range(buffer);
		for (auto it = range.first; it != range.second; it++)
		{
			_resourceHeap.FreeDescriptor(it->second);
		}
		_bufferViews.erase(buffer);
	}
	void DestroyBufferView(D3D12Buffer* buffer, uint32_t descriptorIndex)
	{
		auto iterpair = _bufferViews.equal_range(buffer);
		for (auto it = iterpair.first; it != iterpair.second; ++it) {
			if (it->second == descriptorIndex) {
				_bufferViews.erase(it);
				_resourceHeap.FreeDescriptor(descriptorIndex);
				break;
			}
		}
	}

	void RegisterRTV(D3D12Texture* texture, uint32_t descriptorIndex) { _rtvs.insert({ texture, descriptorIndex }); }
	void RegisterDSV(D3D12Texture* texture, uint32_t descriptorIndex) { _dsvs.insert({ texture, descriptorIndex }); }
	void RegisterTextureView(D3D12Texture* texture, uint32_t descriptorIndex) { _textureViews.insert({ texture, descriptorIndex }); }
	void FreeTextureViews(D3D12Texture* texture);

private:
	D3D12Device* _device;
	D3D12DescriptorHeap _resourceHeap;
	D3D12DescriptorHeap _samplerHeap;
	D3D12DescriptorHeap _rtvHeap;
	D3D12DescriptorHeap _dsvHeap;

	vg::UnorderedMultimap<D3D12Buffer*, uint32_t> _bufferViews;
	vg::UnorderedMultimap<D3D12Texture*, uint32_t> _rtvs;
	vg::UnorderedMultimap<D3D12Texture*, uint32_t> _dsvs;
	vg::UnorderedMultimap<D3D12Texture*, uint32_t> _textureViews;
};

#endif