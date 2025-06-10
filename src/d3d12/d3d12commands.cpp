#include "d3d12commands.h"
#include "d3d12buffer.h"
#include "d3d12pipeline.h"
#include "d3d12descriptor_manager.h"
#include <vector>
#include <array>
#define USE_PIX
#define PIX_ENABLE_GPU_EVENTS 1
#include "WinPixEventRuntime/pix3.h"

constexpr D3D12_COMMAND_LIST_TYPE QueueToCommandListType(VgQueue queue)
{
	switch (queue)
	{
	case VG_QUEUE_GRAPHICS: return D3D12_COMMAND_LIST_TYPE_DIRECT;
	case VG_QUEUE_COMPUTE: return D3D12_COMMAND_LIST_TYPE_COMPUTE;
	case VG_QUEUE_TRANSFER: return D3D12_COMMAND_LIST_TYPE_COPY;
	default: return D3D12_COMMAND_LIST_TYPE_NONE;
	}
}

constexpr std::wstring_view SelectCommandPoolName(VgQueue queue)
{
	switch (queue)
	{
	case VG_QUEUE_GRAPHICS: return L"Graphics Command Pool";
	case VG_QUEUE_COMPUTE: return L"Compute Command Pool";
	case VG_QUEUE_TRANSFER: return L"Transfer Command Pool";
	default: return L"[unknown queue] Command Pool";
	}
}

D3D12CommandPool::D3D12CommandPool(D3D12Device& device, VgCommandPoolFlags flags, VgQueue queue)
	: _device(&device)
{
	// VG_COMMAND_POOL_FLAG_TRANSIENT is only usable as Vulkan driver hint
	(void)flags;
	_queue = queue;
	_type = QueueToCommandListType(queue);
	Assert(_type != D3D12_COMMAND_LIST_TYPE_NONE);

	ThrowOnError(_device->Device()->CreateCommandAllocator(_type, IID_PPV_ARGS(&_allocator)));
	_allocator->SetName(SelectCommandPoolName(queue).data());
}

D3D12CommandPool::~D3D12CommandPool()
{
	std::scoped_lock lock(_cmdMutex);

	for (auto it = _lists.begin(); it != _lists.end(); it++)
	{
		auto list = *it;
		GetAllocator().Delete(list);
	}
}

VgCommandList D3D12CommandPool::AllocateCommandList()
{
	std::scoped_lock lock(_cmdMutex);

	auto list = new(GetAllocator().Allocate<D3D12CommandList>()) D3D12CommandList(*this);
	_lists.insert(list);
	return list;
}

void D3D12CommandPool::FreeCommandList(VgCommandList list)
{
	std::scoped_lock lock(_cmdMutex);
	
	_lists.erase(static_cast<D3D12CommandList*>(list));
	GetAllocator().Delete(list);
}

void D3D12CommandPool::Reset()
{
	std::scoped_lock lock(_cmdMutex);
	
	_allocator->Reset();
	for (auto list : _lists)
	{
		if (list->GetState() & VgCommandList_t::STATE_OPEN)
		{
			list->Cmd()->Close();
		}
		list->Cmd()->Reset(_allocator.Get(), nullptr);
		list->SetState(list->GetState() | VgCommandList_t::STATE_OPEN);
	}
}



constexpr std::wstring_view SelectCommandListName(VgQueue queue)
{
	switch (queue)
	{
	case VG_QUEUE_GRAPHICS: return L"Graphics Command List";
	case VG_QUEUE_COMPUTE: return L"Compute Command List";
	case VG_QUEUE_TRANSFER: return L"Transfer Command List";
	default: return L"[unknown queue] Command List";
	}
}

void D3D12CommandList::ResetRefValues()
{
	_state = STATE_NONE;
	_currentIndexType = static_cast<VgIndexType>(-1);
	_boundPipeline = nullptr;
	memset(_computeRootConstants.data(), 0, _computeRootConstants.size() * sizeof(_computeRootConstants[0]));
}

D3D12CommandList::D3D12CommandList(D3D12CommandPool& pool)
	: _pool(&pool), _indirectCommandBuffer{ nullptr }
{
	ThrowOnError(pool.Device()->Device()->CreateCommandList(0, pool.Type(), pool.Allocator(), nullptr, IID_PPV_ARGS(&_cmd)));
	ThrowOnError(_cmd->Close());
	_cmd->SetName(SelectCommandListName(pool.Queue()).data());

	memset(_viewports.data(), 0, _viewports.size() * sizeof(_viewports[0]));
	memset(_scissors.data(), 0, _scissors.size() * sizeof(_scissors[0]));
	ResetRefValues();
}

D3D12CommandList::~D3D12CommandList()
{
	if (_indirectCommandBuffer)
		_pool->Device()->DestroyBuffer(_indirectCommandBuffer);
}

void D3D12CommandList::RestoreDescriptorState()
{
	auto device = _pool->Device();
	std::array<ID3D12DescriptorHeap*, 2> heaps = {
		device->DescriptorManager().ResourceHeap().Heap(),
		device->DescriptorManager().SamplerHeap().Heap()
	};
	_cmd->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());

	switch (_pool->Queue())
	{
	case VG_QUEUE_GRAPHICS:
		_cmd->SetGraphicsRootSignature(device->GetGraphicsRootSignature());
		_cmd->SetGraphicsRootDescriptorTable(2, device->DescriptorManager().ResourceHeap().GetGPUDescriptorHandle(0));
		_cmd->SetGraphicsRootDescriptorTable(3, device->DescriptorManager().SamplerHeap().GetGPUDescriptorHandle(0));
	case VG_QUEUE_COMPUTE:
		_cmd->SetComputeRootSignature(device->GetComputeRootSignature());
		_cmd->SetComputeRootDescriptorTable(2, device->DescriptorManager().ResourceHeap().GetGPUDescriptorHandle(0));
		_cmd->SetComputeRootDescriptorTable(3, device->DescriptorManager().SamplerHeap().GetGPUDescriptorHandle(0));
		break;
	}
}

void D3D12CommandList::Begin()
{
	if (!(_state & STATE_OPEN))
	{
		_cmd->Reset(_pool->Allocator(), nullptr);
		_state |= STATE_OPEN;
	}

	if (_pool->Queue() == VG_QUEUE_TRANSFER) return;

	if (!_oldIndirectCommandBuffers.empty())
	{
		for (D3D12Buffer* buffer : _oldIndirectCommandBuffers)
		{
			_pool->Device()->DestroyBuffer(buffer);
		}
		_oldIndirectCommandBuffers.clear();
	}
	if (!_oldBufferViews.empty())
	{
		for (const auto [buffer, view] : _oldBufferViews)
		{
			_pool->Device()->DescriptorManager().DestroyBufferView(buffer, view);
		}
		_oldBufferViews.clear();
	}
	RestoreDescriptorState();
}

void D3D12CommandList::End()
{
	_cmd->Close();
	ResetRefValues();
}

void D3D12CommandList::SetVertexBuffers(uint32_t startSlot, uint32_t numBuffers, const VgVertexBufferView* buffers)
{
	std::array<D3D12_VERTEX_BUFFER_VIEW, vg_num_max_vertex_buffers> views;
	for (uint32_t i = 0; i < numBuffers; i++)
	{
		const auto buffer = static_cast<D3D12Buffer*>(buffers[i].buffer);
		views[i] = {
			.BufferLocation = buffer->Resource()->GetGPUVirtualAddress() + buffers[i].offset,
			.SizeInBytes = static_cast<UINT>(buffer->Desc().size),
			.StrideInBytes = buffers[i].stride_in_bytes
		};
	}
	_cmd->IASetVertexBuffers(startSlot, numBuffers, views.data());
}

constexpr DXGI_FORMAT IndexTypeToDXGIFormat(VgIndexType indexType)
{
	switch (indexType)
	{
	case VG_INDEX_TYPE_UINT16: return DXGI_FORMAT_R16_UINT;
	default: return DXGI_FORMAT_R32_UINT;
	}
}

void D3D12CommandList::SetIndexBuffer(VgIndexType indexType, uint64_t offset, VgBuffer buffer)
{
	const auto b = static_cast<D3D12Buffer*>(buffer);
	const D3D12_INDEX_BUFFER_VIEW view = {
		.BufferLocation = b->Resource()->GetGPUVirtualAddress() + offset,
		.SizeInBytes = static_cast<UINT>(b->Desc().size - offset),
		.Format = IndexTypeToDXGIFormat(indexType)
	};
	_cmd->IASetIndexBuffer(&view);
	if (indexType != _currentIndexType)
	{
		_currentIndexType = indexType;
		if (_boundPipeline && _boundPipeline->Type() == VG_PIPELINE_TYPE_GRAPHICS && static_cast<D3D12GraphicsPipeline*>(_boundPipeline)->PrimitiveRestart())
		{
			_cmd->IASetIndexBufferStripCutValue(indexType == VG_INDEX_TYPE_UINT16
				? D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF : D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF);
		}
	}
}

void D3D12CommandList::SetRootConstants(VgPipelineType pipelineType, uint32_t offsetIn32bitValues, uint32_t num32bitValues, const void* data)
{
	switch (pipelineType)
	{
	case VG_PIPELINE_TYPE_GRAPHICS:
		_cmd->SetGraphicsRoot32BitConstants(0, num32bitValues, data, offsetIn32bitValues);
		break;
	case VG_PIPELINE_TYPE_COMPUTE:
		_cmd->SetComputeRoot32BitConstants(0, num32bitValues, data, offsetIn32bitValues);
		memcpy(_computeRootConstants.data() + offsetIn32bitValues, data, num32bitValues * sizeof(uint32_t));
		break;
	}
}

void D3D12CommandList::SetPipeline(VgPipeline pipeline)
{
	if (pipeline == _boundPipeline) return;
	_cmd->SetPipelineState(static_cast<D3D12Pipeline*>(pipeline)->GetPipelineState().Get());

	auto d3d12Pipeline = static_cast<D3D12Pipeline*>(pipeline);
	if (pipeline->Type() == VG_PIPELINE_TYPE_GRAPHICS)
	{
		auto graphicsPipeline = static_cast<D3D12GraphicsPipeline*>(d3d12Pipeline);
		if (!_boundPipeline	|| _boundPipeline->Type() != VG_PIPELINE_TYPE_COMPUTE ||
			(!static_cast<D3D12GraphicsPipeline*>(_boundPipeline)->PrimitiveRestart()
				&& graphicsPipeline->PrimitiveRestart()))
		{
			switch (_currentIndexType)
			{
			case VG_INDEX_TYPE_UINT16:
				_cmd->IASetIndexBufferStripCutValue(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF);
				break;
			case VG_INDEX_TYPE_UINT32:
				_cmd->IASetIndexBufferStripCutValue(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF);
				break;
			}
		}
		if (graphicsPipeline->DepthBoundsTest())
		{
			_cmd->OMSetDepthBounds(graphicsPipeline->DepthBoundsMin(), graphicsPipeline->DepthBoundsMax());
		}
		_cmd->IASetPrimitiveTopology(graphicsPipeline->PrimitiveTopology());
		
	}
	_boundPipeline = static_cast<D3D12Pipeline*>(pipeline);
}

void D3D12CommandList::Dispatch(uint32_t groups_x, uint32_t groups_y, uint32_t groups_z)
{
	_cmd->Dispatch(groups_x, groups_y, groups_z);
}

struct DrawIdEmulationCopyComputeShaderBindData
{
	uint32_t OutputBuffer;
	uint32_t SourceBuffer;
	uint32_t CountBuffer;
	uint32_t Stride;
	uint32_t NumCommands;
	uint32_t Mode; // 0 - draw, 1 - draw indexed
};

constexpr uint32_t minAllocatedDrawCount = 1000u;
void D3D12CommandList::CreateOrSyncIndirectEmulationBuffer(uint32_t drawCount, uint64_t drawCommandSize)
{
	const uint64_t requiredSize = std::max(static_cast<uint64_t>(drawCount) * (sizeof(uint32_t) + drawCommandSize),
		minAllocatedDrawCount * (sizeof(uint32_t) + sizeof(VgDrawIndexedIndirectCommand)));
	if (!_indirectCommandBuffer || _indirectCommandBuffer->Desc().size < requiredSize)
	{
		LOG(DEBUG, "D3D12CommandList::DrawIndirect(): creating buffer for DrawId emulation for {} commands",
			requiredSize, requiredSize / (sizeof(uint32_t) + drawCommandSize));

		if (_indirectCommandBuffer)
			_oldIndirectCommandBuffers.push_back(_indirectCommandBuffer);
		VgBufferDesc desc = {
			.size = requiredSize,
			.usage = VG_BUFFER_USAGE_GENERAL,
			.heap_type = VG_HEAP_TYPE_GPU
		};
		_indirectCommandBuffer = static_cast<D3D12Buffer*>(static_cast<D3D12Device*>(_pool->Device())->CreateBuffer(desc));
		_indirectCommandBuffer->SetName("[IndirectCommandBuffer | DrawId]");

		VgBufferViewDesc viewDesc = {
			.descriptor_type = VG_BUFFER_DESCRIPTOR_TYPE_UAV,
			.view_type = VG_BUFFER_VIEW_TYPE_STRUCTURED_BUFFER,
			.offset = 0,
			.size = requiredSize,
			.element_size = sizeof(uint32_t) + drawCommandSize
		};
		_indirectCommandBufferUAV = _indirectCommandBuffer->CreateView(viewDesc);
	}
	else
	{
		D3D12_BUFFER_BARRIER barrier = {
			.SyncBefore = D3D12_BARRIER_SYNC_EXECUTE_INDIRECT,
			.SyncAfter = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
			.AccessBefore = D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT,
			.AccessAfter = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
			.pResource = _indirectCommandBuffer->Resource().Get(),
			.Offset = 0,
			.Size = _indirectCommandBuffer->Desc().size
		};
		D3D12_BARRIER_GROUP group = {
			.Type = D3D12_BARRIER_TYPE_BUFFER,
			.NumBarriers = 1,
			.pBufferBarriers = &barrier
		};
		_cmd->Barrier(1, &group);
	}
}

uint32_t D3D12CommandList::PrepareForIndirectDrawingWithExecuteIndirectTier10(D3D12Buffer* buffer, uint64_t offset, uint64_t drawCount, uint32_t stride)
{
	VgBufferViewDesc viewDesc = {
			.descriptor_type = VG_BUFFER_DESCRIPTOR_TYPE_SRV,
			.view_type = VG_BUFFER_VIEW_TYPE_BYTE_ADDRESS_BUFFER,
			.offset = offset,
			.size = drawCount * stride,
	};
	uint32_t sourceView = static_cast<D3D12Buffer*>(buffer)->CreateView(viewDesc);
	_oldBufferViews.push_back({ static_cast<D3D12Buffer*>(buffer), sourceView });

	_cmd->SetPipelineState(_pool->Device()->GetUtilityPipelines().DrawIdEmulationComputeShader.Get());
	return sourceView;
}

uint32_t D3D12CommandList::PrepareCountBufferForIndirectDrawing(D3D12Buffer* buffer, uint64_t offset)
{
	VgBufferViewDesc viewDesc = {
			.descriptor_type = VG_BUFFER_DESCRIPTOR_TYPE_SRV,
			.view_type = VG_BUFFER_VIEW_TYPE_BYTE_ADDRESS_BUFFER,
			.offset = offset,
			.size = sizeof(uint32_t),
	};
	uint32_t countView = static_cast<D3D12Buffer*>(buffer)->CreateView(viewDesc);
	_oldBufferViews.push_back({ static_cast<D3D12Buffer*>(buffer), countView });
	return countView;
}

void D3D12CommandList::FinishIndirectDrawingWithExecuteIndirectTier10(ID3D12CommandSignature* commandSignature,
	uint32_t drawCount, ID3D12Resource* countBuffer, uint64_t countBufferOffset)
{
	D3D12_BUFFER_BARRIER barrier = {
		   .SyncBefore = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
		   .SyncAfter = D3D12_BARRIER_SYNC_EXECUTE_INDIRECT,
		   .AccessBefore = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
		   .AccessAfter = D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT,
		   .pResource = _indirectCommandBuffer->Resource().Get(),
		   .Offset = 0,
		   .Size = _indirectCommandBuffer->Desc().size
	};
	D3D12_BARRIER_GROUP group = {
		.Type = D3D12_BARRIER_TYPE_BUFFER,
		.NumBarriers = 1,
		.pBufferBarriers = &barrier
	};
	_cmd->Barrier(1, &group);

	_cmd->SetPipelineState(_boundPipeline->GetPipelineState().Get());
	_cmd->ExecuteIndirect(commandSignature, drawCount, _indirectCommandBuffer->Resource().Get(), 0, countBuffer, countBufferOffset);

	_cmd->SetComputeRoot32BitConstants(0, sizeof(DrawIdEmulationCopyComputeShaderBindData) / sizeof(uint32_t), _computeRootConstants.data(), 0);
}

void D3D12CommandList::DrawIndirect(VgBuffer buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)
{
	auto commandSignature = _pool->Device()->GetCommandSignatureManager().GetDrawCommandSignature(stride);
	if (_pool->Device()->GetExecuteIndirectTier() >= D3D12_EXECUTE_INDIRECT_TIER_1_1)
	{
		_cmd->SetGraphicsRoot32BitConstant(1, 0, 0);
		_cmd->ExecuteIndirect(commandSignature.Get(), drawCount, static_cast<D3D12Buffer*>(buffer)->Resource().Get(), offset, nullptr, 0);
		return;
	}

	CreateOrSyncIndirectEmulationBuffer(drawCount, sizeof(VgDrawIndirectCommand));
	uint32_t sourceView = PrepareForIndirectDrawingWithExecuteIndirectTier10(static_cast<D3D12Buffer*>(buffer), offset, drawCount, stride);

	DrawIdEmulationCopyComputeShaderBindData bindData = {
		.OutputBuffer = _indirectCommandBufferUAV,
		.SourceBuffer = sourceView,
		.CountBuffer = static_cast<uint32_t>(-1),
		.Stride = stride,
		.NumCommands = drawCount,
		.Mode = 0
	};
	_cmd->SetComputeRoot32BitConstants(0, sizeof(bindData) / sizeof(uint32_t), &bindData, 0);
	_cmd->Dispatch((drawCount + 63) / 64, 1, 1);
	
	FinishIndirectDrawingWithExecuteIndirectTier10(commandSignature.Get(), drawCount);
}

void D3D12CommandList::DrawIndirectCount(VgBuffer buffer, uint64_t offset, VgBuffer countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
	auto commandSignature = _pool->Device()->GetCommandSignatureManager().GetDrawCommandSignature(stride);
	if (_pool->Device()->GetExecuteIndirectTier() >= D3D12_EXECUTE_INDIRECT_TIER_1_1)
	{
		_cmd->SetGraphicsRoot32BitConstant(1, 0, 0);
		_cmd->ExecuteIndirect(commandSignature.Get(), maxDrawCount, static_cast<D3D12Buffer*>(buffer)->Resource().Get(), offset,
			static_cast<D3D12Buffer*>(countBuffer)->Resource().Get(), countBufferOffset);
		return;
	}

	CreateOrSyncIndirectEmulationBuffer(maxDrawCount, sizeof(VgDrawIndirectCommand));
	uint32_t sourceView = PrepareForIndirectDrawingWithExecuteIndirectTier10(static_cast<D3D12Buffer*>(buffer), offset, maxDrawCount, stride);
	uint32_t countView = PrepareCountBufferForIndirectDrawing(static_cast<D3D12Buffer*>(countBuffer), countBufferOffset);

	DrawIdEmulationCopyComputeShaderBindData bindData = {
		.OutputBuffer = _indirectCommandBufferUAV,
		.SourceBuffer = sourceView,
		.CountBuffer = countView,
		.Stride = stride,
		.Mode = 0
	};
	_cmd->SetComputeRoot32BitConstants(0, sizeof(bindData) / sizeof(uint32_t), &bindData, 0);
	_cmd->Dispatch((maxDrawCount + 63) / 64, 1, 1);

	FinishIndirectDrawingWithExecuteIndirectTier10(commandSignature.Get(), maxDrawCount, static_cast<D3D12Buffer*>(countBuffer)->Resource().Get(), countBufferOffset);
}

void D3D12CommandList::DrawIndexedIndirect(VgBuffer buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)
{
	auto commandSignature = _pool->Device()->GetCommandSignatureManager().GetDrawIndexedCommandSignature(stride);
	if (_pool->Device()->GetExecuteIndirectTier() >= D3D12_EXECUTE_INDIRECT_TIER_1_1)
	{
		_cmd->SetGraphicsRoot32BitConstant(1, 0, 0);
		_cmd->ExecuteIndirect(commandSignature.Get(), drawCount, static_cast<D3D12Buffer*>(buffer)->Resource().Get(), offset, nullptr, 0);
		return;
	}

	CreateOrSyncIndirectEmulationBuffer(drawCount, sizeof(VgDrawIndexedIndirectCommand));
	uint32_t sourceView = PrepareForIndirectDrawingWithExecuteIndirectTier10(static_cast<D3D12Buffer*>(buffer), offset, drawCount, stride);

	DrawIdEmulationCopyComputeShaderBindData bindData = {
		.OutputBuffer = _indirectCommandBufferUAV,
		.SourceBuffer = sourceView,
		.CountBuffer = static_cast<uint32_t>(-1),
		.Stride = stride,
		.NumCommands = drawCount,
		.Mode = 1
	};
	_cmd->SetComputeRoot32BitConstants(0, sizeof(bindData) / sizeof(uint32_t), &bindData, 0);
	_cmd->Dispatch((drawCount + 63) / 64, 1, 1);

	FinishIndirectDrawingWithExecuteIndirectTier10(commandSignature.Get(), drawCount);
}

void D3D12CommandList::DrawIndexedIndirectCount(VgBuffer buffer, uint64_t offset, VgBuffer countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
	auto commandSignature = _pool->Device()->GetCommandSignatureManager().GetDrawIndexedCommandSignature(stride);
	if (_pool->Device()->GetExecuteIndirectTier() >= D3D12_EXECUTE_INDIRECT_TIER_1_1)
	{
		_cmd->SetGraphicsRoot32BitConstant(1, 0, 0);
		_cmd->ExecuteIndirect(commandSignature.Get(), maxDrawCount, static_cast<D3D12Buffer*>(buffer)->Resource().Get(), offset,
			static_cast<D3D12Buffer*>(countBuffer)->Resource().Get(), countBufferOffset);
		return;
	}

	CreateOrSyncIndirectEmulationBuffer(maxDrawCount, sizeof(VgDrawIndexedIndirectCommand));
	uint32_t sourceView = PrepareForIndirectDrawingWithExecuteIndirectTier10(static_cast<D3D12Buffer*>(buffer), offset, maxDrawCount, stride);
	uint32_t countView = PrepareCountBufferForIndirectDrawing(static_cast<D3D12Buffer*>(countBuffer), countBufferOffset);

	DrawIdEmulationCopyComputeShaderBindData bindData = {
		.OutputBuffer = _indirectCommandBufferUAV,
		.SourceBuffer = sourceView,
		.CountBuffer = countView,
		.Stride = stride,
		.Mode = 1
	};
	_cmd->SetComputeRoot32BitConstants(0, sizeof(bindData) / sizeof(uint32_t), &bindData, 0);
	_cmd->Dispatch((maxDrawCount + 63) / 64, 1, 1);

	FinishIndirectDrawingWithExecuteIndirectTier10(commandSignature.Get(), maxDrawCount, static_cast<D3D12Buffer*>(countBuffer)->Resource().Get(), countBufferOffset);
}

void D3D12CommandList::DispatchIndirect(VgBuffer buffer, uint64_t offset)
{
	auto commandSignature = _pool->Device()->GetCommandSignatureManager().GetDispatchCommandSignature(sizeof(VgDispatchIndirectCommand));
	_cmd->ExecuteIndirect(commandSignature.Get(), 1, static_cast<D3D12Buffer*>(buffer)->Resource().Get(), offset, nullptr, 0);
}

/*static D3D12_BARRIER_LAYOUT MapLayoutToQueue(D3D12_BARRIER_LAYOUT layout, VgQueue queue)
{
	const vg::UnorderedMap<VgQueue, vg::UnorderedMap<D3D12_BARRIER_LAYOUT, D3D12_BARRIER_LAYOUT>> bigMap = {
		{ VG_QUEUE_GRAPHICS, {
			{ D3D12_BARRIER_LAYOUT_COMMON, D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COMMON },
			{ D3D12_BARRIER_LAYOUT_GENERIC_READ, D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_GENERIC_READ },
			{ D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS, D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS },
			{ D3D12_BARRIER_LAYOUT_SHADER_RESOURCE, D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE },
			{ D3D12_BARRIER_LAYOUT_COPY_SOURCE, D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_SOURCE },
			{ D3D12_BARRIER_LAYOUT_COPY_DEST, D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST },
		} },
		{ VG_QUEUE_COMPUTE, {
			{ D3D12_BARRIER_LAYOUT_COMMON, D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COMMON },
			{ D3D12_BARRIER_LAYOUT_GENERIC_READ, D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_GENERIC_READ },
			{ D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS, D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_UNORDERED_ACCESS },
			{ D3D12_BARRIER_LAYOUT_SHADER_RESOURCE, D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_SHADER_RESOURCE },
			{ D3D12_BARRIER_LAYOUT_COPY_SOURCE, D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COPY_SOURCE },
			{ D3D12_BARRIER_LAYOUT_COPY_DEST, D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COPY_DEST },
		} }
	};
	const auto it = bigMap.find(queue);
	if (it == bigMap.end()) return layout;
	const auto l = it->second.find(layout);
	return l != it->second.end() ? l->second : layout;
}*/

void D3D12CommandList::Barrier(const VgDependencyInfo& dependencyInfo)
{
	_globalBarriers.clear();
	_bufferBarriers.clear();
	_textureBarriers.clear();

	for (uint32_t i = 0; i < dependencyInfo.num_memory_barriers; i++)
	{
		D3D12_GLOBAL_BARRIER barrier{
			.SyncBefore = VgPipelineStageToBarrierSync(dependencyInfo.memory_barriers[i].src_stage),
			.SyncAfter = VgPipelineStageToBarrierSync(dependencyInfo.memory_barriers[i].dst_stage),
			.AccessBefore = VgAccessToBarrierAccess(dependencyInfo.memory_barriers[i].src_access),
			.AccessAfter = VgAccessToBarrierAccess(dependencyInfo.memory_barriers[i].dst_access)
		};
		if (barrier.AccessBefore == D3D12_BARRIER_ACCESS_COMMON)
			barrier.AccessAfter = D3D12_BARRIER_ACCESS_COMMON;

		if (barrier.SyncBefore & D3D12_BARRIER_SYNC_COMPUTE_SHADING || _pool->Type() != D3D12_COMMAND_LIST_TYPE_DIRECT)
			barrier.AccessBefore &= ~(D3D12_BARRIER_ACCESS_RENDER_TARGET | D3D12_BARRIER_ACCESS_RESOLVE_DEST
				| D3D12_BARRIER_ACCESS_VERTEX_BUFFER | D3D12_BARRIER_ACCESS_INDEX_BUFFER
				| D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ | D3D12_BARRIER_ACCESS_RESOLVE_SOURCE
				| D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE);
		if (barrier.SyncAfter & D3D12_BARRIER_SYNC_COMPUTE_SHADING || _pool->Type() != D3D12_COMMAND_LIST_TYPE_DIRECT)
			barrier.AccessAfter &= ~(D3D12_BARRIER_ACCESS_VERTEX_BUFFER | D3D12_BARRIER_ACCESS_INDEX_BUFFER
				| D3D12_BARRIER_ACCESS_RENDER_TARGET | D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ
				| D3D12_BARRIER_ACCESS_RESOLVE_DEST | D3D12_BARRIER_ACCESS_RESOLVE_SOURCE
				| D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE);

		_globalBarriers.push_back(barrier);
	}
	for (uint32_t i = 0; i < dependencyInfo.num_buffer_barriers; i++)
	{
		auto eiTier = _pool->Device()->GetExecuteIndirectTier();
		D3D12_BUFFER_BARRIER barrier{
			.SyncBefore = VgPipelineStageToBarrierSync(dependencyInfo.buffer_barriers[i].src_stage, true, eiTier),
			.SyncAfter = VgPipelineStageToBarrierSync(dependencyInfo.buffer_barriers[i].dst_stage, true, eiTier),
			.AccessBefore = VgAccessToBarrierAccess(dependencyInfo.buffer_barriers[i].src_access, true, eiTier),
			.AccessAfter = VgAccessToBarrierAccess(dependencyInfo.buffer_barriers[i].dst_access, true, eiTier),
			.pResource = static_cast<D3D12Buffer*>(dependencyInfo.buffer_barriers[i].buffer)->Resource().Get(),
			.Offset = 0,
			.Size = static_cast<D3D12Buffer*>(dependencyInfo.buffer_barriers[i].buffer)->Desc().size
		};
		if (dependencyInfo.buffer_barriers[i].src_stage == VG_PIPELINE_STAGE_NONE
			&& dependencyInfo.buffer_barriers[i].src_access == VG_ACCESS_NONE)
			barrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;

		_bufferBarriers.push_back(barrier);
	}
	for (uint32_t i = 0; i < dependencyInfo.num_texture_barriers; i++)
	{
		auto texture = static_cast<D3D12Texture*>(dependencyInfo.texture_barriers[i].texture);
		D3D12_TEXTURE_BARRIER barrier{
			.SyncBefore = VgPipelineStageToBarrierSync(dependencyInfo.texture_barriers[i].src_stage),
			.SyncAfter = VgPipelineStageToBarrierSync(dependencyInfo.texture_barriers[i].dst_stage),
			.AccessBefore = VgAccessToBarrierAccess(dependencyInfo.texture_barriers[i].src_access),
			.AccessAfter = VgAccessToBarrierAccess(dependencyInfo.texture_barriers[i].dst_access),
			.LayoutBefore = VgTextureLayoutToBarrierLayout(dependencyInfo.texture_barriers[i].old_layout),
			.LayoutAfter = VgTextureLayoutToBarrierLayout(dependencyInfo.texture_barriers[i].new_layout),
			.pResource = texture->Resource().Get(),
			.Subresources = {
				.IndexOrFirstMipLevel = dependencyInfo.texture_barriers[i].subresource_range.base_mip_level,
				.NumMipLevels = dependencyInfo.texture_barriers[i].subresource_range.mip_levels,
				.FirstArraySlice = dependencyInfo.texture_barriers[i].subresource_range.base_array_layer,
				.NumArraySlices = dependencyInfo.texture_barriers[i].subresource_range.array_layers,
				.FirstPlane = 0,
				.NumPlanes = FormatPlaneCount(texture->Desc().format)
			},
			.Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
		};
		if (barrier.AccessBefore == D3D12_BARRIER_ACCESS_COMMON && barrier.LayoutBefore == D3D12_BARRIER_LAYOUT_UNDEFINED)
			barrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
		barrier.AccessBefore &= ~(D3D12_BARRIER_ACCESS_VERTEX_BUFFER | D3D12_BARRIER_ACCESS_CONSTANT_BUFFER
			| D3D12_BARRIER_ACCESS_INDEX_BUFFER | D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT
			| D3D12_BARRIER_ACCESS_PREDICATION | D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ);
		barrier.AccessAfter &= ~(D3D12_BARRIER_ACCESS_VERTEX_BUFFER | D3D12_BARRIER_ACCESS_CONSTANT_BUFFER
			| D3D12_BARRIER_ACCESS_INDEX_BUFFER | D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT
			| D3D12_BARRIER_ACCESS_PREDICATION | D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ);

		if (!(texture->Desc().usage & VG_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT))
		{
			barrier.AccessBefore &= ~(D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ | D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ);
			barrier.AccessAfter &= ~(D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ | D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ);
		}
		if (barrier.LayoutBefore == D3D12_BARRIER_LAYOUT_COMMON)
			barrier.AccessBefore &= ~(D3D12_BARRIER_ACCESS_RESOLVE_SOURCE | D3D12_BARRIER_ACCESS_RESOLVE_DEST);
		if (barrier.LayoutAfter == D3D12_BARRIER_LAYOUT_COMMON)
			barrier.AccessAfter &= ~(D3D12_BARRIER_ACCESS_RESOLVE_SOURCE | D3D12_BARRIER_ACCESS_RESOLVE_DEST);
		if (barrier.AccessBefore == D3D12_BARRIER_ACCESS_COMMON && barrier.SyncBefore == D3D12_BARRIER_SYNC_NONE)
			barrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
		if (barrier.AccessAfter == D3D12_BARRIER_ACCESS_COMMON && barrier.SyncAfter == D3D12_BARRIER_SYNC_NONE)
			barrier.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;

		//barrier.LayoutBefore = MapLayoutToQueue(barrier.LayoutBefore, dependencyInfo.texture_barriers[i].old_queue);
		//barrier.LayoutAfter = MapLayoutToQueue(barrier.LayoutAfter, dependencyInfo.texture_barriers[i].new_queue);
		_textureBarriers.push_back(barrier);
	}

	const D3D12_BARRIER_GROUP globalGroup = {
		.Type = D3D12_BARRIER_TYPE_GLOBAL,
		.NumBarriers = static_cast<UINT>(_globalBarriers.size()),
		.pGlobalBarriers = _globalBarriers.data()
	};
	const D3D12_BARRIER_GROUP bufferGroup = {
		.Type = D3D12_BARRIER_TYPE_BUFFER,
		.NumBarriers = static_cast<UINT>(_bufferBarriers.size()),
		.pBufferBarriers = _bufferBarriers.data()
	};
	const D3D12_BARRIER_GROUP textureGroup = {
		.Type = D3D12_BARRIER_TYPE_TEXTURE,
		.NumBarriers = static_cast<UINT>(_textureBarriers.size()),
		.pTextureBarriers = _textureBarriers.data()
	};

	uint32_t numGroups = 0;
	std::array<D3D12_BARRIER_GROUP, 3> groups;
	if (dependencyInfo.num_memory_barriers > 0) { groups[numGroups++] = globalGroup; }
	if (dependencyInfo.num_buffer_barriers > 0) { groups[numGroups++] = bufferGroup; }
	if (dependencyInfo.num_texture_barriers > 0) { groups[numGroups++] = textureGroup; }

	_cmd->Barrier(numGroups, groups.data());
}

void D3D12CommandList::BeginRendering(const VgRenderingInfo& info)
{
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, vg_num_max_color_attachments> color_attachments;
	for (uint32_t i = 0; i < info.num_color_attachments; i++)
	{
		color_attachments[i] = _pool->Device()->DescriptorManager().RTVHeap().GetCPUDescriptorHandle(info.color_attachments[i].view);
		if (info.color_attachments[i].load_op == VG_ATTACHMENT_OP_CLEAR)
		{
			_cmd->ClearRenderTargetView(color_attachments[i], info.color_attachments[i].clear.color, 0, nullptr);
		}
	}

	if (info.depth_stencil_attachment.view != VG_NO_VIEW)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE depthStencilDescriptor = _pool->Device()->DescriptorManager().DSVHeap()
			.GetCPUDescriptorHandle(info.depth_stencil_attachment.view);
		if (info.depth_stencil_attachment.load_op == VG_ATTACHMENT_OP_CLEAR)
		{
			_cmd->ClearDepthStencilView(depthStencilDescriptor, D3D12_CLEAR_FLAG_DEPTH, info.depth_stencil_attachment.clear.depth, 0, 0, nullptr);
		}

		_cmd->OMSetRenderTargets(info.num_color_attachments, color_attachments.data(), false, &depthStencilDescriptor);
	}
	else
	{
		_cmd->OMSetRenderTargets(info.num_color_attachments, color_attachments.data(), false, nullptr);
	}

	_renderingNumColorAttachments = info.num_color_attachments;
	memcpy(_renderingColorAttachments.data(), info.color_attachments, info.num_color_attachments * sizeof(VgAttachmentInfo));
	_renderingDepthStencilAttachment = info.depth_stencil_attachment;
	_state |= STATE_RENDERING;
}

void D3D12CommandList::EndRendering()
{
	// Maybe should implement this? But will need to somehow get ID3D12Resource* from attachment view
	/*
	for (uint32_t i = 0; i < _renderingNumColorAttachments; i++)
	{
		if (_renderingColorAttachments[i].store_op == VG_ATTACHMENT_OP_DONT_CARE)
		{
			D3D12_DISCARD_REGION region = {
				.NumRects = 0,
				.pRects = nullptr,
				.FirstSubresource = 0,
				.NumSubresources = D3D12_SUBRESOURCE
			};
			_cmd->DiscardResource();
		}
	}*/

	_state &= ~STATE_RENDERING;
}

void D3D12CommandList::SetViewport(uint32_t firstViewport, uint32_t numViewports, VgViewport* viewports)
{
	//thread_local std::array<D3D12_VIEWPORT, vg_num_max_viewports_and_scissors> d3d12Viewports;
	memcpy(_viewports.data() + firstViewport, viewports, numViewports * sizeof(viewports[0]));
	_cmd->RSSetViewports(static_cast<UINT>(_viewports.size()), _viewports.data());
}

void D3D12CommandList::SetScissor(uint32_t firstScissor, uint32_t numScissors, VgScissor* scissors)
{
	//thread_local std::array<D3D12_RECT, vg_num_max_viewports_and_scissors> d3d12Scissors;
	//memcpy(_scissors.data() + first_scissor, scissors, num_scissors * sizeof(scissors[0]));
	for (uint32_t i = firstScissor; i < firstScissor + numScissors; i++)
	{
		_scissors[i] = {
			.left = static_cast<LONG>(scissors[i].x),
			.top = static_cast<LONG>(scissors[i].y),
			.right = static_cast<LONG>(scissors[i].x + scissors[i].width),
			.bottom = static_cast<LONG>(scissors[i].y + scissors[i].height)
		};
	}
	_cmd->RSSetScissorRects(static_cast<UINT>(_scissors.size()), _scissors.data());
}

void D3D12CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
	_cmd->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
}

void D3D12CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
{
	_cmd->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

constexpr size_t AlignRowPitch(size_t width) {
	const size_t alignment = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT; // 256-byte alignment
	return (width + alignment - 1) & ~(alignment - 1);
}

constexpr uint32_t GetSubresourceIndex(const VgRegion& region) {
	return region.mip + (region.base_array_layer * region.array_layers);
}

constexpr size_t AlignRowPitchWithFormat(size_t width, VgFormat format) {
	const auto bcSize = GetBCFormatBlockSize(format);
	if (bcSize == 0) return AlignRowPitch(width * FormatSizeBytes(format));
	return AlignRowPitch(((width + 3) / 4) * bcSize);
}

void D3D12CommandList::CopyBufferToBuffer(VgBuffer dst, uint64_t dstOffset, VgBuffer src, uint64_t srcOffset, uint64_t size)
{
	_cmd->CopyBufferRegion(static_cast<D3D12Buffer*>(dst)->Resource().Get(), dstOffset,
		static_cast<D3D12Buffer*>(src)->Resource().Get(), srcOffset, size);
}

void D3D12CommandList::CopyBufferToTexture(VgTexture dst, const VgRegion& dstRegion, VgBuffer src, uint64_t srcOffset)
{
	auto dstTexture = static_cast<D3D12Texture*>(dst);
	D3D12_TEXTURE_COPY_LOCATION dstLocation = {
		.pResource = dstTexture->Resource().Get(),
		.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
		.SubresourceIndex = GetSubresourceIndex(dstRegion)
	};

	D3D12_TEXTURE_COPY_LOCATION srcLocation = {
		.pResource = static_cast<D3D12Buffer*>(src)->Resource().Get(),
		.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
		.PlacedFootprint = {
			.Offset = srcOffset,
			.Footprint = {
				.Format = FormatToDXGIFormat(dstTexture->Desc().format),
				.Width = dstRegion.width,
				.Height = dstRegion.height,
				.Depth = dstRegion.depth,
				.RowPitch = static_cast<UINT>(AlignRowPitchWithFormat(dstRegion.width, dstTexture->Desc().format))
			}
		}
	};
	_cmd->CopyTextureRegion(&dstLocation, dstRegion.offset.x, dstRegion.offset.y, dstRegion.offset.z, &srcLocation, nullptr);
}

void D3D12CommandList::CopyTextureToBuffer(VgBuffer dst, uint64_t dstOffset, VgTexture src, const VgRegion& srcRegion)
{
	auto srcTexture = static_cast<D3D12Texture*>(src);
	D3D12_TEXTURE_COPY_LOCATION dstLocation = {
		.pResource = static_cast<D3D12Buffer*>(dst)->Resource().Get(),
		.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
		.PlacedFootprint = {
			.Offset = dstOffset,
			.Footprint = {
				.Format = FormatToDXGIFormat(srcTexture->Desc().format),
				.Width = srcRegion.width,
				.Height = srcRegion.height,
				.Depth = srcRegion.depth,
				.RowPitch = static_cast<UINT>(AlignRowPitchWithFormat(srcRegion.width, srcTexture->Desc().format))
			}
		}
	};
	D3D12_TEXTURE_COPY_LOCATION srcLocation = {
		.pResource = srcTexture->Resource().Get(),
		.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
		.SubresourceIndex = GetSubresourceIndex(srcRegion)
	};
	_cmd->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
}

void D3D12CommandList::CopyTextureToTexture(VgTexture dst, const VgRegion& dstRegion, VgTexture src, const VgRegion& srcRegion)
{
	auto srcTexture = static_cast<D3D12Texture*>(src);
	auto dstTexture = static_cast<D3D12Texture*>(dst);

	D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
	dstLocation.pResource = dstTexture->Resource().Get();
	dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dstLocation.SubresourceIndex = GetSubresourceIndex(dstRegion);
	D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
	srcLocation.pResource = srcTexture->Resource().Get();
	srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	srcLocation.SubresourceIndex = GetSubresourceIndex(srcRegion);
	_cmd->CopyTextureRegion(&dstLocation, dstRegion.offset.x, dstRegion.offset.y, dstRegion.offset.z, &srcLocation, nullptr);
}

void D3D12CommandList::BeginMarker(const char* name, float color[3])
{
	auto& runtime = static_cast<D3D12Device*>(_pool->Device())->PixEventRuntime();
	if (!runtime.BeginEvent) return;

	// PIXBeginEvent
	runtime.BeginEvent(_cmd.Get(), PIX_COLOR(static_cast<UINT8>(color[0] * 255), static_cast<UINT8>(color[1] * 255),
		static_cast<UINT8>(color[2] * 255)), name);
}

void D3D12CommandList::EndMarker()
{
	auto& runtime = static_cast<D3D12Device*>(_pool->Device())->PixEventRuntime();
	if (!runtime.EndEvent) return;

	// PIXEndEvent
	runtime.EndEvent(_cmd.Get());
}
