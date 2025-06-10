#pragma once

#include "../interface.h"
#include "../common.h"
#define NOMINMAX
#include <agilitysdk/d3d12.h>
#include <agilitysdk/d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include "D3D12MemAlloc.h"
#undef ERROR
#include <unordered_map>
#include <stdexcept>
#include <format>
#include <memory>
#include <mutex>

using namespace Microsoft::WRL;

#define ThrowOnError(x) do { \
const HRESULT _hr_ = x; \
if (FAILED(_hr_)) \
	if (_hr_ == DXGI_ERROR_DEVICE_REMOVED || _hr_ == DXGI_ERROR_DEVICE_HUNG || _hr_ == DXGI_ERROR_DEVICE_RESET) throw VgDeviceLostError(\
		std::format("`{}`. File {}, Line {}", #x, __FILE__, __LINE__)); \
	else throw VgFailure(std::format("`{}` failed with code {:x}. File {}, Line {}", #x, _hr_, __FILE__, __LINE__)); \
} while (false)

#define ThrowOnErrorMsg(x, msg, ...) do { \
const HRESULT _hr_ = x; \
if (FAILED(_hr_)) \
	if (_hr_ == DXGI_ERROR_DEVICE_REMOVED || _hr_ == DXGI_ERROR_DEVICE_HUNG || _hr_ == DXGI_ERROR_DEVICE_RESET) throw VgDeviceLostError(\
		std::format("`{}`. File {}, Line {}", #x, __FILE__, __LINE__)); \
	else throw VgFailure(std::format("`{}` failed with code {:x}: {}", #x, _hr_, std::format(msg, ##__VA_ARGS__))); \
} while (false)

#define ThrowOnErrorWithBlob(x, blob) do { \
const HRESULT _hr_ = x; \
if (FAILED(_hr_)) \
	if (_hr_ == DXGI_ERROR_DEVICE_REMOVED || _hr_ == DXGI_ERROR_DEVICE_HUNG || _hr_ == DXGI_ERROR_DEVICE_RESET) throw VgDeviceLostError(\
		std::format("`{}`. File {}, Line {}", #x, __FILE__, __LINE__)); \
	else throw VgFailure(std::format("`{}` failed with code {:x}. File {}, Line {}. Error blob: {}", #x, _hr_, __FILE__, __LINE__, static_cast<const char*>(blob->GetBufferPointer()))); \
} while (false)

constexpr DXGI_FORMAT FormatToDXGIFormat(VgFormat format)
{
	return static_cast<DXGI_FORMAT>(format);
}

constexpr D3D12_HEAP_TYPE HeapTypeToD3D12HeapType(VgHeapType heapType)
{
	return static_cast<D3D12_HEAP_TYPE>(heapType);
}

constexpr D3D12_BARRIER_SYNC VgPipelineStageToBarrierSync(VgPipelineStageFlags flags, bool isBuffer = false, D3D12_EXECUTE_INDIRECT_TIER eiTier = D3D12_EXECUTE_INDIRECT_TIER_1_0)
{
	D3D12_BARRIER_SYNC sync = D3D12_BARRIER_SYNC_NONE;
	if (flags & (VG_PIPELINE_STAGE_NONE |
		VG_PIPELINE_STAGE_TOP_OF_PIPE |
		VG_PIPELINE_STAGE_BOTTOM_OF_PIPE)) sync |= D3D12_BARRIER_SYNC_NONE;
	if (flags & VG_PIPELINE_STAGE_DRAW_INDIRECT)
	{
		sync |= D3D12_BARRIER_SYNC_EXECUTE_INDIRECT;
		if (isBuffer && eiTier == D3D12_EXECUTE_INDIRECT_TIER_1_0)
			sync |= D3D12_BARRIER_SYNC_COMPUTE_SHADING;
	}
	if (flags & VG_PIPELINE_STAGE_VERTEX_INPUT) sync |= D3D12_BARRIER_SYNC_INDEX_INPUT;
	if (flags & (VG_PIPELINE_STAGE_VERTEX_SHADER |
		VG_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER |
		VG_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER |
		VG_PIPELINE_STAGE_GEOMETRY_SHADER)) sync |= D3D12_BARRIER_SYNC_VERTEX_SHADING;
	if (flags & VG_PIPELINE_STAGE_FRAGMENT_SHADER) sync |= D3D12_BARRIER_SYNC_PIXEL_SHADING;
	if (flags & (VG_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS |
		VG_PIPELINE_STAGE_LATE_FRAGMENT_TESTS)) sync |= D3D12_BARRIER_SYNC_DEPTH_STENCIL;
	if (flags & VG_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT) sync |= D3D12_BARRIER_SYNC_RENDER_TARGET;
	if (flags & VG_PIPELINE_STAGE_COMPUTE_SHADER) sync |= D3D12_BARRIER_SYNC_COMPUTE_SHADING;
	if (flags & (VG_PIPELINE_STAGE_TRANSFER | VG_PIPELINE_STAGE_ALL_TRANSFER)) sync |= D3D12_BARRIER_SYNC_COPY;
	if (flags & VG_PIPELINE_STAGE_ALL_GRAPHICS) sync |= D3D12_BARRIER_SYNC_ALL_SHADING;
	if (flags & VG_PIPELINE_STAGE_ALL_COMMANDS) sync |= D3D12_BARRIER_SYNC_ALL;
	//if (flags & VG_PIPELINE_STAGE_VIDEO_DECODE) sync |= D3D12_BARRIER_SYNC_VIDEO_DECODE;
	//if (flags & VG_PIPELINE_STAGE_VIDEO_ENCODE) sync |= D3D12_BARRIER_SYNC_VIDEO_ENCODE;
	if (flags & VG_PIPELINE_STAGE_RAY_TRACING_SHADER) sync |= D3D12_BARRIER_SYNC_RAYTRACING;
	if (flags & VG_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD) sync |= D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
	if (flags & VG_PIPELINE_STAGE_ACCELERATION_STRUCTURE_COPY) sync |= D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE;
	if (flags & (VG_PIPELINE_STAGE_TASK_SHADER | VG_PIPELINE_STAGE_MESH_SHADER)) sync |= D3D12_BARRIER_SYNC_NON_PIXEL_SHADING;
	return sync;
}

constexpr D3D12_BARRIER_ACCESS VgAccessToBarrierAccess(VgAccessFlags flags, bool isBuffer = false, D3D12_EXECUTE_INDIRECT_TIER eiTier = D3D12_EXECUTE_INDIRECT_TIER_1_0)
{
	D3D12_BARRIER_ACCESS access = D3D12_BARRIER_ACCESS_COMMON;

	//if (flags & VG_ACCESS_NONE) access = D3D12_BARRIER_ACCESS_NO_ACCESS;
	if (flags & VG_ACCESS_INDIRECT_COMMAND_READ)
	{
		access |= D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT;
		if (isBuffer && eiTier == D3D12_EXECUTE_INDIRECT_TIER_1_0)
			access |= D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
	}
	if (flags & VG_ACCESS_INDEX_READ) access |= D3D12_BARRIER_ACCESS_INDEX_BUFFER;
	if (flags & VG_ACCESS_VERTEX_ATTRIBUTE_READ) access |= D3D12_BARRIER_ACCESS_VERTEX_BUFFER;
	if (flags & VG_ACCESS_UNIFORM_READ) access |= D3D12_BARRIER_ACCESS_CONSTANT_BUFFER;
	if (flags & VG_ACCESS_INPUT_ATTACHMENT_READ) access |= D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
	if (flags & VG_ACCESS_SHADER_READ) access |= D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
	if (flags & VG_ACCESS_SHADER_WRITE) access |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
	if (flags & (VG_ACCESS_COLOR_ATTACHMENT_READ | VG_ACCESS_COLOR_ATTACHMENT_WRITE))
		access |= D3D12_BARRIER_ACCESS_RENDER_TARGET;
	if (flags & VG_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ) access |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;
	if (flags & VG_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE) access |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;
	if (flags & VG_ACCESS_TRANSFER_READ) access |= D3D12_BARRIER_ACCESS_COPY_SOURCE;
	if (flags & VG_ACCESS_TRANSFER_WRITE) access |= D3D12_BARRIER_ACCESS_COPY_DEST;
	if (flags & VG_ACCESS_MEMORY_READ) access |= D3D12_BARRIER_ACCESS_COPY_SOURCE
		| D3D12_BARRIER_ACCESS_CONSTANT_BUFFER | D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ
		| D3D12_BARRIER_ACCESS_INDEX_BUFFER | D3D12_BARRIER_ACCESS_VERTEX_BUFFER
		| D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT | D3D12_BARRIER_ACCESS_RESOLVE_SOURCE
		| D3D12_BARRIER_ACCESS_SHADER_RESOURCE | D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;
	if (flags & VG_ACCESS_MEMORY_WRITE) access |= D3D12_BARRIER_ACCESS_RENDER_TARGET
		| D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE | D3D12_BARRIER_ACCESS_COPY_DEST
		| D3D12_BARRIER_ACCESS_RESOLVE_DEST | D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE
		| D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
	if (flags & (VG_ACCESS_SHADER_SAMPLED_READ | VG_ACCESS_SHADER_STORAGE_READ))
		access |= D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
	if (flags & VG_ACCESS_SHADER_STORAGE_WRITE) access |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
	//if (flags & VG_ACCESS_VIDEO_DECODE_READ) access |= D3D12_BARRIER_ACCESS_VIDEO_DECODE_READ;
	//if (flags & VG_ACCESS_VIDEO_DECODE_WRITE) access |= D3D12_BARRIER_ACCESS_VIDEO_DECODE_WRITE;
	//if (flags & VG_ACCESS_VIDEO_ENCODE_READ) access |= D3D12_BARRIER_ACCESS_VIDEO_ENCODE_READ;
	//if (flags & VG_ACCESS_VIDEO_ENCODE_WRITE) access |= D3D12_BARRIER_ACCESS_VIDEO_ENCODE_WRITE;
	if (flags & VG_ACCESS_ACCELERATION_STRUCTURE_READ) access |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;
	if (flags & VG_ACCESS_ACCELERATION_STRUCTURE_WRITE) access |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
	if (flags & VG_ACCESS_SHADER_BINDING_TABLE_READ) access |= D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
	//if (flags & VG_ACCESS_COMMON) access = static_cast<D3D12_BARRIER_ACCESS>(VG_ACCESS_COMMON | access);

	return access;
}

constexpr D3D12_BARRIER_LAYOUT VgTextureLayoutToBarrierLayout(VgTextureLayout layout)
{
	switch (layout)
	{
	case VG_TEXTURE_LAYOUT_UNDEFINED: return D3D12_BARRIER_LAYOUT_UNDEFINED;
	case VG_TEXTURE_LAYOUT_GENERAL: return D3D12_BARRIER_LAYOUT_COMMON;
	case VG_TEXTURE_LAYOUT_COLOR_ATTACHMENT: return D3D12_BARRIER_LAYOUT_RENDER_TARGET;
	case VG_TEXTURE_LAYOUT_DEPTH_STENCIL: return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
	case VG_TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY: return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;
	case VG_TEXTURE_LAYOUT_SHADER_RESOURCE: return D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
	case VG_TEXTURE_LAYOUT_TRANSFER_SOURCE: return D3D12_BARRIER_LAYOUT_COPY_SOURCE;
	case VG_TEXTURE_LAYOUT_TRANSFER_DEST: return D3D12_BARRIER_LAYOUT_COPY_DEST;
	case VG_TEXTURE_LAYOUT_RESOLVE_SOURCE: return D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE;
	case VG_TEXTURE_LAYOUT_RESOLVE_DEST: return D3D12_BARRIER_LAYOUT_RESOLVE_DEST;
	case VG_TEXTURE_LAYOUT_PRESENT: return D3D12_BARRIER_LAYOUT_PRESENT;
	case VG_TEXTURE_LAYOUT_READ_ONLY: return D3D12_BARRIER_LAYOUT_GENERIC_READ;
	case VG_TEXTURE_LAYOUT_UNORDERED_ACCESS: return D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
	default: return D3D12_BARRIER_LAYOUT_COMMON;
	}
}

constexpr D3D12_COMPARISON_FUNC VgCompareOpToComparisonFunc(VgCompareOp op)
{
	switch (op)
	{
	case VG_COMPARE_OP_LESS: return D3D12_COMPARISON_FUNC_LESS;
	case VG_COMPARE_OP_EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
	case VG_COMPARE_OP_LESS_OR_EQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case VG_COMPARE_OP_GREATER: return D3D12_COMPARISON_FUNC_GREATER;
	case VG_COMPARE_OP_NOT_EQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case VG_COMPARE_OP_GREATER_OR_EQUAL: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case VG_COMPARE_OP_ALWAYS: return D3D12_COMPARISON_FUNC_ALWAYS;
	default: return D3D12_COMPARISON_FUNC_NEVER;
	}
}

constexpr D3D12_STENCIL_OP VgStencilOpToD3D12(VgStencilOp op)
{
	switch (op)
	{
	case VG_STENCIL_OP_ZERO: return D3D12_STENCIL_OP_ZERO;
	case VG_STENCIL_OP_REPLACE: return D3D12_STENCIL_OP_REPLACE;
	case VG_STENCIL_OP_INCREMENT_AND_CLAMP: return D3D12_STENCIL_OP_INCR_SAT;
	case VG_STENCIL_OP_DECREMENT_AND_CLAMP: return D3D12_STENCIL_OP_DECR_SAT;
	case VG_STENCIL_OP_INVERT: return D3D12_STENCIL_OP_INVERT;
	case VG_STENCIL_OP_INCREMENT_AND_WRAP: return D3D12_STENCIL_OP_INCR;
	case VG_STENCIL_OP_DECREMENT_AND_WRAP: return D3D12_STENCIL_OP_DECR;
	default: return D3D12_STENCIL_OP_KEEP;
	}
}

static vg::WString ConvertToWideString(const char* narrowStr)
{
	if (!narrowStr) return L"";

	int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, narrowStr, -1, nullptr, 0);
	vg::WString wideStr(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, narrowStr, -1, &wideStr[0], sizeNeeded);

	return wideStr;
}

struct D3D12DrawIndirectCmd
{
	uint32_t VertexCountPerInstance;
	uint32_t InstanceCount;
	uint32_t StartVertexLocation;
	uint32_t StartInstanceLocation;
};

struct D3D12DrawIndexedIndirectCmd
{
	uint32_t IndexCountPerInstance;
	uint32_t InstanceCount;
	uint32_t StartIndexLocation;
	uint32_t BaseVertexLocation;
	uint32_t StartInstanceLocation;
};

class D3D12Device;
class D3D12IndirectCommandSignatureManager
{
public:
	D3D12IndirectCommandSignatureManager(D3D12Device& device);
	~D3D12IndirectCommandSignatureManager();

	ComPtr<ID3D12CommandSignature> GetDrawCommandSignature(uint32_t stride);
	ComPtr<ID3D12CommandSignature> GetDrawIndexedCommandSignature(uint32_t stride);
	ComPtr<ID3D12CommandSignature> GetDispatchCommandSignature(uint32_t stride);

private:
	D3D12Device* _device;

	vg::UnorderedMap<uint32_t, ComPtr<ID3D12CommandSignature>> _drawCommandSignatures;
	vg::UnorderedMap<uint32_t, ComPtr<ID3D12CommandSignature>> _drawIndexedCommandSignatures;
	vg::UnorderedMap<uint32_t, ComPtr<ID3D12CommandSignature>> _dispatchCommandSignatures;
	std::mutex _drawMutex;
	std::mutex _drawIndexedMutex;
	std::mutex _dispatchMutex;
};

struct WinPixEventRuntime
{
	inline static constexpr const wchar_t* Dll = L"WinPixEventRuntime.dll";

	using FnBeginEvent = void(WINAPI*)(ID3D12GraphicsCommandList* commandList, UINT64 color, _In_ PCSTR formatString);
	using FnEndEvent = void(WINAPI*)(ID3D12GraphicsCommandList* commandList);
	using FnSetMarker = void(WINAPI*)(ID3D12GraphicsCommandList* commandList, UINT64 color, _In_ PCSTR formatString);

	FnBeginEvent BeginEvent{ nullptr };
	FnEndEvent EndEvent{ nullptr };
	FnSetMarker SetMarker{ nullptr };

	WinPixEventRuntime()
	{
		_module = LoadLibraryW(WinPixEventRuntime::Dll);
		if (!_module) return;

		BeginEvent = reinterpret_cast<FnBeginEvent>(GetProcAddress(_module, "PIXBeginEventOnCommandList"));
		EndEvent = reinterpret_cast<FnEndEvent>(GetProcAddress(_module, "PIXEndEventOnCommandList"));
		SetMarker = reinterpret_cast<FnSetMarker>(GetProcAddress(_module, "PIXSetMarkerOnCommandList"));
	}
	~WinPixEventRuntime()
	{
		if (_module) FreeLibrary(_module);
	}

private:
	HMODULE _module;
};

struct D3D12UtilityPipelines
{
	ComPtr<ID3D12PipelineState> DrawIdEmulationComputeShader;
};

class D3D12Adapter;
class D3D12DescriptorManager;
class D3D12Device final : public VgDevice_t
{
public:
	D3D12Device(D3D12Adapter& adapter, VgInitFlags initFlags);
	~D3D12Device();

	void* GetApiObject() const override { return _device.Get(); }
	VgGraphicsApi Api() const override { return VG_GRAPHICS_API_D3D12; }
	const VgMemoryStatistics& GetMemoryStatistics() const override { return _memStats; }
	VgMemoryStatistics& GetMemoryStatistics() { return _memStats; }

	uint32_t NodeMask() const { return 0; }
	VgAdapter Adapter() const override;
	ComPtr<ID3D12Device5> Device() const { return _device; }
	ComPtr<D3D12MA::Allocator> Allocator() const { return _allocator; }
	D3D12DescriptorManager& DescriptorManager() { return *_descriptorManager; }
	const D3D12DescriptorManager& DescriptorManager() const { return *_descriptorManager; }
	const D3D12UtilityPipelines& GetUtilityPipelines() const { return _utilityPipelines; }
	const WinPixEventRuntime& PixEventRuntime() const { return _winPixEventRuntime; }

	ComPtr<ID3D12CommandQueue> GetQueue(VgQueue queue) const
	{
		switch (queue)
		{
		case VG_QUEUE_COMPUTE: return _computVgQueue;
		case VG_QUEUE_TRANSFER: return _transferQueue;
		default: return _graphicsQueue;
		}
	}

	ID3D12RootSignature* GetComputeRootSignature() const { return _computeRootSignature.Get(); }
	ID3D12RootSignature* GetGraphicsRootSignature() const { return _graphicsRootSignature.Get(); }

	D3D12IndirectCommandSignatureManager& GetCommandSignatureManager() { return _commandSignatureManager; }
	D3D12_EXECUTE_INDIRECT_TIER GetExecuteIndirectTier() const { return _executeIndirectTier; }

	VgCommandPool CreateCommandPool(VgCommandPoolFlags flags, VgQueue queue) override;
	void DestroyCommandPool(VgCommandPool pool) override;

	VgBuffer CreateBuffer(const VgBufferDesc& desc) override;
	void DestroyBuffer(VgBuffer buffer) override;

	VgShaderModule CreateShaderModule(const void* data, uint64_t size) override;
	void DestroyShaderModule(VgShaderModule module) override;

	VgPipeline CreateGraphicsPipeline(const VgGraphicsPipelineDesc& desc) override;
	VgPipeline CreateComputePipeline(VgShaderModule shaderModule) override;
	void DestroyPipeline(VgPipeline pipeline) override;

	VgFence CreateFence(uint64_t initialValue) override;
	void DestroyFence(VgFence fence) override;

	VgSampler CreateSampler(const VgSamplerDesc& desc) override;
	void DestroySampler(VgSampler sampler) override;

	VgTexture CreateTexture(const VgTextureDesc& desc) override;
	void DestroyTexture(VgTexture texture) override;

	VgSwapChain CreateSwapChain(const VgSwapChainDesc& desc) override;
	void DestroySwapChain(VgSwapChain swapChain) override;

	void WaitQueueIdle(VgQueue queue) override;
	void WaitIdle() override;

	void SignalFence(VgFence fence, uint64_t value) override;
	void WaitFence(VgFence fence, uint64_t value) override;
	uint64_t GetFenceValue(VgFence fence) override;

	void SubmitCommandLists(uint32_t numSubmits, const VgSubmitInfo* submits) override;

	uint32_t GetSamplerIndex(VgSampler sampler) override;

private:
	D3D12Adapter* _adapter;
	ComPtr<ID3D12Device> _baseDevice;
	ComPtr<ID3D12Device10> _device;
	ComPtr<ID3D12Debug5> _debugInterface;
	ComPtr<ID3D12DebugDevice2> _debugDevice;

	D3D12MA::ALLOCATION_CALLBACKS _allocationCallbacks;
	ComPtr<D3D12MA::Allocator> _allocator;

	ComPtr<ID3D12CommandQueue> _graphicsQueue;
	ComPtr<ID3D12CommandQueue> _computVgQueue;
	ComPtr<ID3D12CommandQueue> _transferQueue;

	std::unique_ptr<D3D12DescriptorManager> _descriptorManager;
	ComPtr<ID3D12RootSignature> _computeRootSignature;
	ComPtr<ID3D12RootSignature> _graphicsRootSignature;

	D3D12IndirectCommandSignatureManager _commandSignatureManager;
	D3D12_EXECUTE_INDIRECT_TIER _executeIndirectTier;

	WinPixEventRuntime _winPixEventRuntime;
	D3D12UtilityPipelines _utilityPipelines;
	
	std::mutex _fenceMutex;
	struct FenceWithEvent
	{
		ID3D12Fence* Fence;
		HANDLE Event;
	};
	uintptr_t _nextFenceIndex{ 1 };
	vg::UnorderedMap<void*, FenceWithEvent> _fences;

	VgMemoryStatistics _memStats;

	void InitDescriptorManagement();
};