#pragma once

#include "varyag.h"
#include <optional>

struct VgAdapter_t
{
public:
	virtual ~VgAdapter_t() = default;

	virtual void* GetApiObject() const = 0;
	virtual VgGraphicsApi Api() const = 0;
	virtual const VgAdapterProperties& GetProperties() const = 0;

	virtual VgDevice CreateDevice(VgInitFlags initFlags) = 0;
	uint64_t Score() const
	{
		auto properties = GetProperties();
		uint64_t score = properties.type == VG_ADAPTER_TYPE_DISCRETE ? 10000 : (properties.type == VG_ADAPTER_TYPE_INTEGRATED ? 5000 : 0);
		score += properties.dedicated_vram / 200 + properties.hardware_ray_tracing * 1000 + properties.mesh_shaders * 1000;
		return score;
	}
};

struct VgDevice_t
{
public:
	virtual ~VgDevice_t() = default;

	virtual void* GetApiObject() const = 0;
	virtual VgGraphicsApi Api() const = 0;
	virtual VgAdapter Adapter() const = 0;
	virtual const VgMemoryStatistics& GetMemoryStatistics() const = 0;

	virtual VgCommandPool CreateCommandPool(VgCommandPoolFlags flags, VgQueue queue) = 0;
	virtual void DestroyCommandPool(VgCommandPool pool) = 0;
	virtual VgBuffer CreateBuffer(const VgBufferDesc& desc) = 0;
	virtual void DestroyBuffer(VgBuffer buffer) = 0;
	virtual VgShaderModule CreateShaderModule(const void* data, uint64_t size) = 0;
	virtual void DestroyShaderModule(VgShaderModule module) = 0;
	virtual VgPipeline CreateGraphicsPipeline(const VgGraphicsPipelineDesc& desc) = 0;
	virtual VgPipeline CreateComputePipeline(VgShaderModule shaderModule) = 0;
	virtual void DestroyPipeline(VgPipeline pipeline) = 0;
	virtual VgFence CreateFence(uint64_t initialValue) = 0;
	virtual void DestroyFence(VgFence fence) = 0;
	virtual VgSampler CreateSampler(const VgSamplerDesc& desc) = 0;
	virtual void DestroySampler(VgSampler sampler) = 0;
	virtual VgTexture CreateTexture(const VgTextureDesc& desc) = 0;
	virtual void DestroyTexture(VgTexture texture) = 0;
	virtual VgSwapChain CreateSwapChain(const VgSwapChainDesc& desc) = 0;
	virtual void DestroySwapChain(VgSwapChain swapChain) = 0;

	virtual void WaitQueueIdle(VgQueue queue) = 0;
	virtual void WaitIdle() = 0;

	virtual void SignalFence(VgFence_t* fence, uint64_t value) = 0;
	virtual void WaitFence(VgFence_t* fence, uint64_t value) = 0;
	virtual uint64_t GetFenceValue(VgFence_t* fence) = 0;

	virtual void SubmitCommandLists(uint32_t numSubmits, const VgSubmitInfo* submits) = 0;

	virtual uint32_t GetSamplerIndex(VgSampler_t* sampler) = 0;
};

struct VgCommandPool_t
{
public:
	virtual ~VgCommandPool_t() = default;

	virtual void* GetApiObject() const = 0;
	virtual void SetName(const char* name) = 0;
	virtual VgDevice Device() const = 0;
	VgQueue Queue() const { return _queue; }

	virtual VgCommandList AllocateCommandList() = 0;
	virtual void FreeCommandList(VgCommandList list) = 0;
	virtual void Reset() = 0;

protected:
	VgQueue _queue;
};

struct VgCommandList_t
{
public:
	enum StateFlags
	{
		STATE_NONE = 0,
		STATE_OPEN = 1,
		STATE_RENDERING = 2
	};

	virtual ~VgCommandList_t() = default;

	virtual void* GetApiObject() const = 0;
	virtual void SetName(const char* name) = 0;
	virtual VgDevice Device() const = 0;
	virtual VgCommandPool CommandPool() const = 0;
	virtual void RestoreDescriptorState() = 0;
	StateFlags GetState() const { return _state; }

	void SetState(StateFlags flags) { _state = flags; }

	virtual void Begin() = 0;
	virtual void End() = 0;

	virtual void SetVertexBuffers(uint32_t startSlot, uint32_t numBuffers, const VgVertexBufferView* buffers) = 0;
	virtual void SetIndexBuffer(VgIndexType indexType, uint64_t offset, VgBuffer buffer) = 0;

	virtual void SetRootConstants(VgPipelineType pipelineType, uint32_t offsetIn32bitValues, uint32_t num32bitValues, const void* data) = 0;
	virtual void SetPipeline(VgPipeline pipeline) = 0;


	virtual void Barrier(const VgDependencyInfo& dependencyInfo) = 0;

	virtual void BeginRendering(const VgRenderingInfo& info) = 0;
	virtual void EndRendering() = 0;
	virtual void SetViewport(uint32_t firstViewport, uint32_t numViewports, VgViewport* viewports) = 0;
	virtual void SetScissor(uint32_t firstScissor, uint32_t numScissors, VgScissor* scissors) = 0;

	virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
	virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) = 0;
	virtual void Dispatch(uint32_t groups_x, uint32_t groups_y, uint32_t groups_z) = 0;
	virtual void DrawIndirect(VgBuffer buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) = 0;
	virtual void DrawIndirectCount(VgBuffer buffer, uint64_t offset, VgBuffer countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride) = 0;
	virtual void DrawIndexedIndirect(VgBuffer buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) = 0;
	virtual void DrawIndexedIndirectCount(VgBuffer buffer, uint64_t offset, VgBuffer countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride) = 0;
	virtual void DispatchIndirect(VgBuffer buffer, uint64_t offset) = 0;

	virtual void CopyBufferToBuffer(VgBuffer dst, uint64_t dstOffset, VgBuffer src, uint64_t srcOffset, uint64_t size) = 0;
	virtual void CopyBufferToTexture(VgTexture dst, const VgRegion& dstRegion, VgBuffer src, uint64_t srcOffset) = 0;
	virtual void CopyTextureToBuffer(VgBuffer dst, uint64_t dstOffset, VgTexture src, const VgRegion& srcRegion) = 0;
	virtual void CopyTextureToTexture(VgTexture dst, const VgRegion& dstRegion, VgTexture src, const VgRegion& srcRegion) = 0;

	virtual void BeginMarker(const char* name, float color[3]) = 0;
	virtual void EndMarker() = 0;

protected:
	StateFlags _state;
};

struct VgBuffer_t
{
public:
	virtual ~VgBuffer_t() = default;

	virtual void* GetApiObject() const = 0;
	virtual void SetName(const char* name) = 0;
	virtual VgDevice Device() const = 0;
	virtual const VgBufferDesc& Desc() const = 0;
	virtual uint32_t CreateView(const VgBufferViewDesc& desc) = 0;
	virtual void DestroyViews() = 0;

	virtual void* Map() = 0;
	virtual void Unmap() = 0;
};

struct VgTexture_t
{
public:
	virtual ~VgTexture_t() = default;

	virtual void* GetApiObject() const = 0;
	virtual void SetName(const char* name) = 0;
	virtual VgDevice Device() const = 0;
	virtual const VgTextureDesc& Desc() const = 0;
	virtual bool OwnedBySwapChain() const = 0;
	virtual uint32_t CreateAttachmentView(const VgAttachmentViewDesc& desc) = 0;
	virtual uint32_t CreateView(const VgTextureViewDesc& desc) = 0;
	virtual void DestroyViews() = 0;

	//virtual void* Map() = 0;
	//virtual void Unmap() = 0;
};

struct VgShaderModule_t
{
public:
	virtual ~VgShaderModule_t() = default;
};

struct VgPipeline_t
{
public:
	virtual ~VgPipeline_t() = default;

	virtual void* GetApiObject() const = 0;
	virtual void SetName(const char* name) = 0;
	virtual VgDevice Device() const = 0;
	VgPipelineType Type() const { return _type; }

protected:
	VgPipelineType _type;
};

struct VgSwapChain_t
{
public:
	virtual ~VgSwapChain_t() = default;

	virtual void* GetApiObject() const = 0;
	virtual VgDevice Device() const = 0;
	virtual const VgSwapChainDesc& Desc() const = 0;

	virtual uint32_t AcquireNextImage() = 0;
	virtual VgTexture GetBackBuffer(uint32_t index) = 0;
	virtual void Present(uint32_t numWaitFences, VgFenceOperation* waitFences) = 0;
};


#include <utility>
#include <type_traits>
#define VG_ENUM_FLAGS(e) \
constexpr e operator|(e a, e b) { return static_cast<e>(static_cast<std::underlying_type_t<e>>(a) | static_cast<std::underlying_type_t<e>>(b)); } \
constexpr e& operator|=(e& a, e b) { a = a | b; return a; } \
constexpr e operator&(e a, e b) { return static_cast<e>(static_cast<std::underlying_type_t<e>>(a) & static_cast<std::underlying_type_t<e>>(b)); } \
constexpr e& operator&=(e& a, e b) { a = a & b; return a; } \
constexpr e operator^(e a, e b) { return static_cast<e>(static_cast<std::underlying_type_t<e>>(a) ^ static_cast<std::underlying_type_t<e>>(b)); } \
constexpr e& operator^=(e& a, e b) { a = a ^ b; return a; } \
constexpr e operator<<(e a, std::underlying_type_t<e> b) { return static_cast<e>(static_cast<std::underlying_type_t<e>>(a) << b); } \
constexpr e& operator<<=(e& a, e b) { a = a << b; return a; } \
constexpr e operator>>(e a, std::underlying_type_t<e> b) { return static_cast<e>(static_cast<std::underlying_type_t<e>>(a) >> b); } \
constexpr e& operator>>=(e& a, e b) { a = a >> b; return a; } \
constexpr e operator~(e a) { return static_cast<e>(~static_cast<std::underlying_type_t<e>>(a)); } \

VG_ENUM_FLAGS(VgCommandList_t::StateFlags)