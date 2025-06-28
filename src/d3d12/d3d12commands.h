#pragma once

#include "d3d12device.h"
#include <unordered_set>
#include <mutex>

#if VG_D3D12_SUPPORTED

class D3D12CommandList;
class D3D12CommandPool final : public VgCommandPool_t
{
public:
	D3D12CommandPool(D3D12Device& device, VgCommandPoolFlags flags, VgQueue queue);
	~D3D12CommandPool();

	void* GetApiObject() const override { return _allocator.Get(); }
	void SetName(const char* name) override { _allocator->SetName(ConvertToWideString(name).data()); }
	D3D12Device* Device() const override { return _device; }
	ID3D12CommandAllocator* Allocator() const { return _allocator.Get(); }
	D3D12_COMMAND_LIST_TYPE Type() const { return _type; }

	VgCommandList AllocateCommandList() override;
	void FreeCommandList(VgCommandList list) override;
	void Reset() override;

private:
	D3D12Device* _device;
	ComPtr<ID3D12CommandAllocator> _allocator;
	D3D12_COMMAND_LIST_TYPE _type;
	vg::UnorderedSet<D3D12CommandList*> _lists;
	std::mutex _cmdMutex;
};

// TODO: Split to D3D12GraphicsCommandList, D3D12ComputeCommandList, D3D12TransferCommandList
// Or to D3D12GraphicsCommandList and D3D12NonGraphicsCommandList?
// Right now contains too much shit just for graphics

class D3D12Pipeline;
class D3D12Buffer;
class D3D12CommandList final : public VgCommandList_t
{
public:
	~D3D12CommandList();

	void* GetApiObject() const override { return _cmd.Get(); }
	void SetName(const char* name) override { _cmd->SetName(ConvertToWideString(name).data()); }
	D3D12Device* Device() const override { return _pool->Device(); }
	D3D12CommandPool* CommandPool() const override { return _pool; }
	ID3D12GraphicsCommandList9* Cmd() const { return _cmd.Get(); }
	void RestoreDescriptorState() override;

	void Begin() override;
	void End() override;

	void SetVertexBuffers(uint32_t startSlot, uint32_t numBuffers, const VgVertexBufferView* buffers) override;
	void SetIndexBuffer(VgIndexType indexType, uint64_t offset, VgBuffer buffer) override;

	void SetRootConstants(VgPipelineType pipelineType, uint32_t offsetIn32bitValues, uint32_t num32bitValues, const void* data) override;
	void SetPipeline(VgPipeline pipeline) override;

	void Barrier(const VgDependencyInfo& dependencyInfo) override;

	void BeginRendering(const VgRenderingInfo& info) override;
	void EndRendering() override;
	void SetViewport(uint32_t firstViewport, uint32_t numViewports, VgViewport* viewports) override;
	void SetScissor(uint32_t firstScissor, uint32_t numScissors, VgScissor* scissors) override;

	void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
	void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) override;
	void Dispatch(uint32_t groups_x, uint32_t groups_y, uint32_t groups_z) override;
	void DrawIndirect(VgBuffer buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) override;
	void DrawIndirectCount(VgBuffer buffer, uint64_t offset, VgBuffer countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride) override;
	void DrawIndexedIndirect(VgBuffer buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) override;
	void DrawIndexedIndirectCount(VgBuffer buffer, uint64_t offset, VgBuffer countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride) override;
	void DispatchIndirect(VgBuffer buffer, uint64_t offset) override;

	void CopyBufferToBuffer(VgBuffer dst, uint64_t dstOffset, VgBuffer src, uint64_t srcOffset, uint64_t size) override;
	void CopyBufferToTexture(VgTexture dst, const VgRegion& dstRegion, VgBuffer src, uint64_t srcOffset) override;
	void CopyTextureToBuffer(VgBuffer dst, uint64_t dstOffset, VgTexture src, const VgRegion& srcRegion) override;
	void CopyTextureToTexture(VgTexture dst, const VgRegion& dstRegion, VgTexture src, const VgRegion& srcRegion) override;

	void BeginMarker(const char* name, float color[3]) override;
	void EndMarker() override;

private:
	D3D12CommandPool* _pool;
	ComPtr<ID3D12GraphicsCommandList9> _cmd;

	vg::Vector<D3D12_VERTEX_BUFFER_VIEW> _vertexBufferViews;

	D3D12Pipeline* _boundPipeline;
	VgIndexType _currentIndexType;
	std::array<D3D12_VIEWPORT, vg_num_max_viewports_and_scissors> _viewports;
	std::array<D3D12_RECT, vg_num_max_viewports_and_scissors> _scissors;

	std::array<uint32_t, vg_num_allowed_root_constants> _computeRootConstants;
	vg::Vector<D3D12Buffer*> _oldIndirectCommandBuffers;
	vg::Vector<std::pair<D3D12Buffer*, uint32_t>> _oldBufferViews;
	D3D12Buffer* _indirectCommandBuffer;
	uint32_t _indirectCommandBufferUAV;

	uint32_t _renderingNumColorAttachments;
	std::array<VgAttachmentInfo, vg_num_max_color_attachments> _renderingColorAttachments;
	VgAttachmentInfo _renderingDepthStencilAttachment;

	vg::Vector<D3D12_GLOBAL_BARRIER> _globalBarriers;
	vg::Vector<D3D12_BUFFER_BARRIER> _bufferBarriers;
	vg::Vector<D3D12_TEXTURE_BARRIER> _textureBarriers;

	void ResetRefValues();

	friend D3D12CommandPool;
	D3D12CommandList(D3D12CommandPool& pool);

	void CreateOrSyncIndirectEmulationBuffer(uint32_t drawCount, uint64_t drawCommandSize);
	uint32_t PrepareForIndirectDrawingWithExecuteIndirectTier10(D3D12Buffer* buffer, uint64_t offset, uint64_t drawCount, uint32_t stride);
	uint32_t PrepareCountBufferForIndirectDrawing(D3D12Buffer* buffer, uint64_t offset);
	void FinishIndirectDrawingWithExecuteIndirectTier10(ID3D12CommandSignature* commandSignature, uint32_t drawCount,
		ID3D12Resource* countBuffer = nullptr, uint64_t countBufferOffset = 0);
};

#endif