#include "d3d12pipeline.h"
#include "d3d12shader_module.h"
#include <agilitysdk/d3dx12/d3dx12_pipeline_state_stream.h>

D3D12ComputePipeline::D3D12ComputePipeline(D3D12Device& device, VgShaderModule computeModule)
{
	_device = &device;
	_type = VG_PIPELINE_TYPE_COMPUTE;
	D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
		.pRootSignature = device.GetComputeRootSignature(),
		.CS = static_cast<D3D12ShaderModule*>(computeModule)->GetBytecode(),
		.NodeMask = device.NodeMask(),
		.CachedPSO = {
			.pCachedBlob = nullptr,
			.CachedBlobSizeInBytes = 0
		},
		.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
	};
	ThrowOnError(device.Device()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&_state)));
	_device->GetMemoryStatistics().num_pipelines++;
}

D3D12ComputePipeline::~D3D12ComputePipeline()
{
	_device->GetMemoryStatistics().num_pipelines--;
}

constexpr D3D12_INPUT_ELEMENT_DESC VertexAttributeToD3D12InputElement(const VgVertexAttribute& attribute, uint32_t i)
{
	D3D12_INPUT_ELEMENT_DESC element = {
		.SemanticName = "ATTRIBUTE",
		.SemanticIndex = i,
		.Format = FormatToDXGIFormat(attribute.format),
		.InputSlot = attribute.vertex_buffer_index,
		.AlignedByteOffset = attribute.offset,
		.InputSlotClass = attribute.input_rate == VG_ATTRIBUTE_INPUT_RATE_VERTEX
			? D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA : D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
		.InstanceDataStepRate = attribute.instance_step_rate
	};
	return element;
}

constexpr D3D12_FILL_MODE FillModeToD3D12(VgFillMode fillMode)
{
	switch (fillMode)
	{
	case VG_FILL_MODE_FILL: return D3D12_FILL_MODE_SOLID;
	case VG_FILL_MODE_WIREFRAME: return D3D12_FILL_MODE_WIREFRAME;
	default: return D3D12_FILL_MODE_SOLID;
	}
}

constexpr D3D12_CULL_MODE CullModeToD3D12(VgCullMode cullMode)
{
	switch (cullMode)
	{
	case VG_CULL_MODE_FRONT: return D3D12_CULL_MODE_FRONT;
	case VG_CULL_MODE_BACK: return D3D12_CULL_MODE_BACK;
	default: return D3D12_CULL_MODE_NONE;
	}
}

static CD3DX12_RASTERIZER_DESC RasterizationToD3D12(const VgGraphicsPipelineDesc& desc)
{
	const auto& state = desc.rasterization_state;
	return CD3DX12_RASTERIZER_DESC(FillModeToD3D12(state.fill_mode), CullModeToD3D12(state.cull_mode),
		state.front_face == VG_FRONT_FACE_COUNTER_CLOCKWISE, state.depth_bias, state.depth_bias_clamp,
		state.depth_bias_slope_factor, state.depth_clip_mode == VG_DEPTH_CLIP_MODE_CLIP, desc.multisampling_state.sample_count > VG_SAMPLE_COUNT_1,
		FALSE, 0, state.conservative_rasterization_enable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
}

static CD3DX12_DEPTH_STENCIL_DESC1 DepthStencilToD3D12(const VgGraphicsPipelineDesc& desc)
{
	const auto& state = desc.depth_stencil_state;
	const bool depth_test = desc.rasterization_state.rasterization_discard_enable ? false : state.depth_test_enable;
	const bool depth_write = desc.rasterization_state.rasterization_discard_enable ? false : state.depth_write_enable;
	const bool stencil_test = desc.rasterization_state.rasterization_discard_enable ? false : state.stencil_test_enable;
	const bool depth_bounds_test = desc.rasterization_state.rasterization_discard_enable ? false : state.depth_bounds_test_enable;
	return CD3DX12_DEPTH_STENCIL_DESC1(depth_test, depth_write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO,
		depth_test ? VgCompareOpToComparisonFunc(state.depth_compare_op) : D3D12_COMPARISON_FUNC_ALWAYS, stencil_test,
		static_cast<uint8_t>(state.stencil_read_mask),
		static_cast<uint8_t>(state.stencil_write_mask), VgStencilOpToD3D12(state.front.fail_op), VgStencilOpToD3D12(state.front.depth_fail_op),
		VgStencilOpToD3D12(state.front.pass_op), VgCompareOpToComparisonFunc(state.front.compare_op), VgStencilOpToD3D12(state.back.fail_op),
		VgStencilOpToD3D12(state.back.depth_fail_op), VgStencilOpToD3D12(state.back.pass_op), VgCompareOpToComparisonFunc(state.back.compare_op),
		depth_bounds_test);
}

constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE ExtractPrimitiveTopologyType(const VgGraphicsPipelineDesc& desc)
{
	switch (desc.primitive_topology)
	{
	case VG_PRIMITIVE_TOPOLOGY_POINT_LIST: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case VG_PRIMITIVE_TOPOLOGY_PATCH_LIST: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	case VG_PRIMITIVE_TOPOLOGY_LINE_LIST:
	case VG_PRIMITIVE_TOPOLOGY_LINE_STRIP:
	case VG_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
	case VG_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	default: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}
}

constexpr D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopologyToD3D12(const VgGraphicsPipelineDesc& desc)
{
	switch (desc.primitive_topology)
	{
	case VG_PRIMITIVE_TOPOLOGY_POINT_LIST: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	case VG_PRIMITIVE_TOPOLOGY_LINE_LIST: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	case VG_PRIMITIVE_TOPOLOGY_LINE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case VG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case VG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	case VG_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY: return D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
	case VG_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
	case VG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
	case VG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
	case VG_PRIMITIVE_TOPOLOGY_PATCH_LIST: return static_cast<D3D12_PRIMITIVE_TOPOLOGY>(
		static_cast<UINT>(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST) + (desc.tesselation_control_points - 1));
	default: return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}
}

constexpr D3D12_BLEND BlendFactorToD3D12(VgBlendFactor factor)
{
	switch (factor)
	{
	case VG_BLEND_FACTOR_ZERO: return D3D12_BLEND_ZERO;
	case VG_BLEND_FACTOR_ONE: return D3D12_BLEND_ONE;
	case VG_BLEND_FACTOR_SRC_COLOR: return D3D12_BLEND_SRC_COLOR;
	case VG_BLEND_FACTOR_ONE_MINUS_SRC_COLOR: return D3D12_BLEND_INV_SRC_COLOR;
	case VG_BLEND_FACTOR_DST_COLOR: return D3D12_BLEND_DEST_COLOR;
	case VG_BLEND_FACTOR_ONE_MINUS_DST_COLOR: return D3D12_BLEND_INV_DEST_COLOR;
	case VG_BLEND_FACTOR_SRC_ALPHA: return D3D12_BLEND_SRC_ALPHA;
	case VG_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
	case VG_BLEND_FACTOR_DST_ALPHA: return D3D12_BLEND_DEST_ALPHA;
	case VG_BLEND_FACTOR_ONE_MINUS_DST_ALPHA: return D3D12_BLEND_INV_DEST_ALPHA;
	case VG_BLEND_FACTOR_CONSTANT_COLOR: return D3D12_BLEND_BLEND_FACTOR;
	case VG_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR: return D3D12_BLEND_INV_BLEND_FACTOR;
	case VG_BLEND_FACTOR_CONSTANT_ALPHA: return D3D12_BLEND_ALPHA_FACTOR;
	case VG_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA: return D3D12_BLEND_INV_ALPHA_FACTOR;
	case VG_BLEND_FACTOR_SRC_ALPHA_SATURATE: return D3D12_BLEND_SRC_ALPHA_SAT;
	case VG_BLEND_FACTOR_SRC1_COLOR: return D3D12_BLEND_SRC1_COLOR;
	case VG_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR: return D3D12_BLEND_INV_SRC1_COLOR;
	case VG_BLEND_FACTOR_SRC1_ALPHA: return D3D12_BLEND_SRC1_ALPHA;
	case VG_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA: return D3D12_BLEND_INV_SRC1_ALPHA;
	default: return D3D12_BLEND_ZERO;
	}
}

constexpr D3D12_BLEND_OP BlendOpToD3D12(VgBlendOp op)
{
	switch (op)
	{
	case VG_BLEND_OP_ADD: return D3D12_BLEND_OP_ADD;
	case VG_BLEND_OP_SUBTRACT: return D3D12_BLEND_OP_SUBTRACT;
	case VG_BLEND_OP_REVERSE_SUBTRACT: return D3D12_BLEND_OP_REV_SUBTRACT;
	case VG_BLEND_OP_MIN: return D3D12_BLEND_OP_MIN;
	case VG_BLEND_OP_MAX: return D3D12_BLEND_OP_MAX;
	default: return D3D12_BLEND_OP_ADD;
	}
}

constexpr D3D12_LOGIC_OP LogicOpToD3D12(VgLogicOp op)
{
	switch (op)
	{
	case VG_LOGIC_OP_CLEAR: return D3D12_LOGIC_OP_CLEAR;
	case VG_LOGIC_OP_AND: return D3D12_LOGIC_OP_AND;
	case VG_LOGIC_OP_AND_REVERSE: return D3D12_LOGIC_OP_AND_REVERSE;
	case VG_LOGIC_OP_COPY: return D3D12_LOGIC_OP_COPY;
	case VG_LOGIC_OP_AND_INVERTED: return D3D12_LOGIC_OP_AND_INVERTED;
	case VG_LOGIC_OP_NO_OP: return D3D12_LOGIC_OP_NOOP;
	case VG_LOGIC_OP_XOR: return D3D12_LOGIC_OP_XOR;
	case VG_LOGIC_OP_OR: return D3D12_LOGIC_OP_OR;
	case VG_LOGIC_OP_NOR: return D3D12_LOGIC_OP_NOR;
	case VG_LOGIC_OP_EQUIVALENT: return D3D12_LOGIC_OP_EQUIV;
	case VG_LOGIC_OP_INVERT: return D3D12_LOGIC_OP_INVERT;
	case VG_LOGIC_OP_OR_REVERSE: return D3D12_LOGIC_OP_OR_REVERSE;
	case VG_LOGIC_OP_COPY_INVERTED: return D3D12_LOGIC_OP_COPY_INVERTED;
	case VG_LOGIC_OP_OR_INVERTED: return D3D12_LOGIC_OP_OR_INVERTED;
	case VG_LOGIC_OP_NAND: return D3D12_LOGIC_OP_NAND;
	case VG_LOGIC_OP_SET: return D3D12_LOGIC_OP_SET;
	default: return D3D12_LOGIC_OP_NOOP;
	}
}

uint8_t ColorWriteMaskToD3D12(VgColorComponentFlags components)
{
	if (components == VG_COLOR_COMPONENT_ALL) return D3D12_COLOR_WRITE_ENABLE_ALL;
	uint8_t mask = 0;
	if (components & VG_COLOR_COMPONENT_R) mask |= D3D12_COLOR_WRITE_ENABLE_RED;
	if (components & VG_COLOR_COMPONENT_G) mask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
	if (components & VG_COLOR_COMPONENT_B) mask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
	if (components & VG_COLOR_COMPONENT_A) mask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
	return mask;
}

static CD3DX12_BLEND_DESC BlendToD3D12(const VgGraphicsPipelineDesc& desc)
{
	if (desc.num_color_attachments == 0) return {};
	const auto& state = desc.blend_state;
	CD3DX12_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = desc.multisampling_state.alpha_to_coverage
		&& desc.multisampling_state.sample_count > VG_SAMPLE_COUNT_1;
	blendDesc.IndependentBlendEnable = !state.logic_op_enable;
	auto logicOp = LogicOpToD3D12(state.logic_op);
	for (uint32_t i = 0; i < desc.num_color_attachments; i++)
	{
		const auto& rt = state.attachments[i];
		blendDesc.RenderTarget[i] = {
			.BlendEnable = rt.blend_enable,
			.LogicOpEnable = state.logic_op_enable,
			.SrcBlend = BlendFactorToD3D12(rt.src_color),
			.DestBlend = BlendFactorToD3D12(rt.dst_color),
			.BlendOp = BlendOpToD3D12(rt.color_op),
			.SrcBlendAlpha = BlendFactorToD3D12(rt.src_alpha),
			.DestBlendAlpha = BlendFactorToD3D12(rt.dst_alpha),
			.BlendOpAlpha = BlendOpToD3D12(rt.alpha_op),
			.LogicOp = logicOp,
			.RenderTargetWriteMask = ColorWriteMaskToD3D12(rt.color_write_mask)
		};
	}
	return blendDesc;
}

D3D12GraphicsPipeline::D3D12GraphicsPipeline(D3D12Device& device, const VgGraphicsPipelineDesc& desc)
{
	_device = &device;

	std::array<uint8_t, 1024> stream;
	uint64_t offset = 0;
	auto push = [&](const auto& obj) {
		constexpr size_t align = 8;
		offset = (offset + (align - 1)) & ~(align - 1);
		memcpy(stream.data() + offset, std::addressof(obj), sizeof(obj));
		offset += sizeof(obj);
		};

	D3D12_PIPELINE_STATE_FLAGS flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	if (desc.primitive_restart_enable)
	{
		flags |= D3D12_PIPELINE_STATE_FLAG_DYNAMIC_INDEX_BUFFER_STRIP_CUT;
		_primitiveRestart = true;
	}
	if (desc.depth_stencil_state.depth_test_enable && desc.depth_stencil_state.depth_bounds_test_enable)
	{
		_depthBoundsTest = true;
		_depthBoundsMin = desc.depth_stencil_state.min_depth_bounds;
		_depthBoundsMax = desc.depth_stencil_state.max_depth_bounds;
	}
	push(CD3DX12_PIPELINE_STATE_STREAM_FLAGS{ flags });
	push(CD3DX12_PIPELINE_STATE_STREAM_NODE_MASK(device.NodeMask()));
	push(CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE(device.GetGraphicsRootSignature()));

	if (desc.vertex_pipeline_type == VG_VERTEX_PIPELINE_FIXED_FUNCTION)
	{
		std::array<D3D12_INPUT_ELEMENT_DESC, vg_num_max_vertex_attributes> inputElements;
		for (uint32_t i = 0; i < desc.fixed_function.num_vertex_attributes; i++)
		{
			inputElements[i] = VertexAttributeToD3D12InputElement(desc.fixed_function.vertex_attributes[i], i);
		}
		D3D12_INPUT_LAYOUT_DESC inputLayout = {
			.pInputElementDescs = inputElements.data(),
			.NumElements = desc.fixed_function.num_vertex_attributes
		};
		push(CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT{ inputLayout });
		push(CD3DX12_PIPELINE_STATE_STREAM_VS{ static_cast<D3D12ShaderModule*>(desc.fixed_function.vertex_shader)->GetBytecode() });
		if (desc.fixed_function.hull_shader)
			push(CD3DX12_PIPELINE_STATE_STREAM_HS{ static_cast<D3D12ShaderModule*>(desc.fixed_function.hull_shader)->GetBytecode() });
		if (desc.fixed_function.domain_shader)
			push(CD3DX12_PIPELINE_STATE_STREAM_DS{ static_cast<D3D12ShaderModule*>(desc.fixed_function.domain_shader)->GetBytecode() });
		if (desc.fixed_function.geometry_shader)
			push(CD3DX12_PIPELINE_STATE_STREAM_GS{ static_cast<D3D12ShaderModule*>(desc.fixed_function.geometry_shader)->GetBytecode() });
	}
	else if (desc.vertex_pipeline_type == VG_VERTEX_PIPELINE_MESH_SHADER)
	{
		if (desc.mesh.amplification_shader)
			push(CD3DX12_PIPELINE_STATE_STREAM_AS{ static_cast<D3D12ShaderModule*>(desc.mesh.amplification_shader)->GetBytecode() });
		push(CD3DX12_PIPELINE_STATE_STREAM_MS{ static_cast<D3D12ShaderModule*>(desc.mesh.mesh_shader)->GetBytecode() });
	}

	if (!desc.rasterization_state.rasterization_discard_enable)
		push(CD3DX12_PIPELINE_STATE_STREAM_PS{ static_cast<D3D12ShaderModule*>(desc.pixel_shader)->GetBytecode() });
	push(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER{ RasterizationToD3D12(desc) });
	push(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1{ DepthStencilToD3D12(desc) });

	push(CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC{ BlendToD3D12(desc) });

	if (desc.num_color_attachments > 0)
	{
		D3D12_RT_FORMAT_ARRAY renderTargets;
		renderTargets.NumRenderTargets = desc.num_color_attachments;
		for (uint32_t i = 0; i < desc.num_color_attachments; i++)
		{
			renderTargets.RTFormats[i] = FormatToDXGIFormat(desc.color_attachment_formats[i]);
		}
		for (uint32_t i = desc.num_color_attachments; i < vg_num_max_color_attachments; i++)
		{
			renderTargets.RTFormats[i] = DXGI_FORMAT_UNKNOWN;
		}

		push(CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS{ renderTargets });
	}
	if (desc.depth_stencil_format != VG_FORMAT_UNKNOWN)
		push(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT{ FormatToDXGIFormat(desc.depth_stencil_format) });
	
	push(CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY{ ExtractPrimitiveTopologyType(desc) });

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {
			.SizeInBytes = offset,
			.pPipelineStateSubobjectStream = stream.data()
	};
	ThrowOnError(device.Device()->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&_state)));

	_primitiveTopology = PrimitiveTopologyToD3D12(desc);

	_device->GetMemoryStatistics().num_pipelines++;
}

D3D12GraphicsPipeline::~D3D12GraphicsPipeline()
{
	_device->GetMemoryStatistics().num_pipelines--;
}
