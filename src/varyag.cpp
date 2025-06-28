#include "common.h"
#include "interface.h"
#if VG_D3D12_SUPPORTED
#include "d3d12/d3d12adapter.h"
#include "d3d12/d3d12device.h"

#if VG_FORCE_API == VG_API_D3D12
#include "d3d12/d3d12buffer.h"
#include "d3d12/d3d12commands.h"
#include "d3d12/d3d12pipeline.h"
#include "d3d12/d3d12shader_module.h"
#include "d3d12/d3d12swap_chain.h"
#include "d3d12/d3d12texture.h"
#endif
#endif

#include "varyag.h"
#include "vk/vkcore.h"

#define FUNC_DATA(func_name) \
constexpr std::string_view _func_name_ = #func_name;

#define CHECK_NOT_NULL(variable) do {\
if (!(variable)) { \
	LOG(WARN, "{}(): {} = NULL", _func_name_, #variable); \
	return; \
}}while (false)

#define CHECK_NOT_NULL_RETURN(variable) do {\
if (!(variable)) { \
	LOG(ERROR, "{}(): {} = NULL", _func_name_, #variable); \
	return VG_BAD_ARGUMENT; \
}}while (false)

#include <magic_enum.hpp>
#if VG_VALIDATION
#include <ranges>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <type_traits>
#include <unordered_set>
#include <set>

#define VALIDATE_FLAGS(variable, var_name, ...) do { \
if (!IsValidFlags(variable)) {\
	constexpr auto values = magic_enum::enum_values<std::remove_all_extents<decltype(variable)>::type>(); \
	LOG(ERROR, "{}(): {}({}) should be a valid combination of the following: {}", \
		_func_name_, std::format(var_name, ##__VA_ARGS__), static_cast<uint64_t>(variable), join(values.begin(), values.end())); \
	return; \
}} while (false)

#define VALIDATE_FLAGS_RETURN(variable, var_name, ...) do { \
if (!IsValidFlags(variable)) {\
	constexpr auto values = magic_enum::enum_values<std::remove_all_extents<decltype(variable)>::type>(); \
	LOG(ERROR, "{}(): {}({}) should be a valid combination of the following: {}", \
		_func_name_, std::format(var_name, ##__VA_ARGS__), static_cast<uint64_t>(variable), join(values.begin(), values.end())); \
	return VG_BAD_ARGUMENT; \
}} while (false)

#define VALIDATE_FLAGS_ALLOWED(allowed, variable, var_name, ...) do { \
if (!IsValidFlagsMask(variable, allowed)) { \
	const auto values = std::span(magic_enum::enum_values<std::remove_all_extents<decltype(variable)>::type>()) \
		| std::views::filter([](std::remove_all_extents<decltype(variable)>::type flag) { return static_cast<uint64_t>(flag) & (allowed); }); \
	LOG(ERROR, "{}(): {}({}) should be a valid combination of the following: {}", \
		_func_name_, std::format(var_name, ##__VA_ARGS__), static_cast<uint64_t>(variable), join(values)); \
	return; \
}} while (false)

#define VALIDATE_FLAGS_ALLOWED_RETURN(allowed, variable, var_name, ...) do { \
if (!IsValidFlagsMask(variable, allowed)) { \
	const auto values = std::span(magic_enum::enum_values<std::remove_all_extents<decltype(variable)>::type>()) \
		| std::views::filter([](std::remove_all_extents<decltype(variable)>::type flag) { return static_cast<uint64_t>(flag) & (allowed); }); \
	LOG(ERROR, "{}(): {}({}) should be a valid combination of the following: {}", \
		_func_name_, std::format(var_name, ##__VA_ARGS__), static_cast<uint64_t>(variable), join(values)); \
	return VG_BAD_ARGUMENT; \
}} while (false)

#define VALIDATE_ENUM(variable, var_name, ...) do { \
constexpr auto values = magic_enum::enum_values<std::remove_all_extents<decltype(variable)>::type>(); \
if (std::find(values.begin(), values.end(), variable) == values.end()) \
{ \
	LOG(ERROR, "{}(): {}({}) should be one of the following: {}", _func_name_, std::format(var_name, ##__VA_ARGS__), \
		static_cast<uint64_t>(variable), join(values.begin(), values.end())); \
	return; \
}} while (false)

#define VALIDATE_ENUM_RETURN(variable, var_name, ...) do { \
constexpr auto values = magic_enum::enum_values<std::remove_all_extents<decltype(variable)>::type>(); \
if (std::find(values.begin(), values.end(), variable) == values.end()) \
{ \
	LOG(ERROR, "{}(): {}({}) should be one of the following: {}", _func_name_, std::format(var_name, ##__VA_ARGS__), \
		static_cast<uint64_t>(variable), join(values.begin(), values.end())); \
	return VG_BAD_ARGUMENT; \
}} while (false)

#define VALIDATE_ENUM_ALLOWED(allowed, variable, var_name, ...) do { \
const auto allowed_values = allowed; \
if (std::find(allowed_values.begin(), allowed_values.end(), variable) ==  allowed_values.end()) \
{ \
	LOG(ERROR, "{}(): {}({}) should be one of the following: {}", _func_name_, std::format(var_name, ##__VA_ARGS__), \
		static_cast<uint64_t>(variable), join(allowed_values)); \
	return; \
}} while (false)

#define VALIDATE_ENUM_ALLOWED_RETURN(allowed, variable, var_name, ...) do { \
const auto allowed_values = allowed; \
if (std::find(allowed_values.begin(), allowed_values.end(), variable) ==  allowed_values.end()) \
{ \
	LOG(ERROR, "{}(): {}({}) should be one of the following: {}", _func_name_, std::format(var_name, ##__VA_ARGS__), \
		static_cast<uint64_t>(variable), join(allowed_values)); \
	return VG_BAD_ARGUMENT; \
}} while (false)

#define VALIDATE_DISALLOWED_FLAGS(variable, flags_a, flags_b, var_name, ...) do {\
if (((variable) & (flags_a)) && ((variable) & (flags_b))) {\
	const auto values_a = std::span(magic_enum::enum_values<std::remove_all_extents<decltype(variable)>::type>()) \
		| std::views::filter([](std::remove_all_extents<decltype(variable)>::type flag) { return static_cast<uint64_t>(flag) & (flags_a); }); \
	const auto values_b = std::span(magic_enum::enum_values<std::remove_all_extents<decltype(variable)>::type>()) \
		| std::views::filter([](std::remove_all_extents<decltype(variable)>::type flag) { return static_cast<uint64_t>(flag) & (flags_b); }); \
	LOG(ERROR, "{}(): {}({}): flags {} and {} are not allowed to be set simultaneously", _func_name_, \
		std::format(var_name, ##__VA_ARGS__), static_cast<uint64_t>(variable), join(values_a), join(values_b)); \
	return; \
}} while (false)

#define VALIDATE_DISALLOWED_FLAGS_RETURN(variable, flags_a, flags_b, var_name, ...) do {\
if (((variable) & (flags_a)) && ((variable) & (flags_b))) {\
	const auto values_a = std::span(magic_enum::enum_values<std::remove_all_extents<decltype(variable)>::type>()) \
		| std::views::filter([](std::remove_all_extents<decltype(variable)>::type flag) { return static_cast<uint64_t>(flag) & (flags_a); }); \
	const auto values_b = std::span(magic_enum::enum_values<std::remove_all_extents<decltype(variable)>::type>()) \
		| std::views::filter([](std::remove_all_extents<decltype(variable)>::type flag) { return static_cast<uint64_t>(flag) & (flags_b); }); \
	LOG(ERROR, "{}(): {}({}): flags {} and {} are not allowed to be set simultaneously", _func_name_, \
		std::format(var_name, ##__VA_ARGS__), static_cast<uint64_t>(variable), join(values_a), join(values_b)); \
	return VG_BAD_ARGUMENT; \
}} while (false)

template <> struct magic_enum::customize::enum_range<VgInitFlags> {
	static constexpr bool is_flags = true;
};
template <> struct magic_enum::customize::enum_range<VgCommandPoolFlags> {
	static constexpr bool is_flags = true;
};
template <> struct magic_enum::customize::enum_range<VgPipelineStageFlags> {
	static constexpr bool is_flags = true;
};
template <> struct magic_enum::customize::enum_range<VgAccessFlags> {
	static constexpr bool is_flags = true;
};

template <class TIter> requires requires (TIter t) { std::is_enum_v<std::remove_all_extents_t<decltype(*t)>>; }
static vg::String join(TIter begin, TIter end)
{
	vg::StringStream ss;
	int num = 0;
	while (begin != end)
	{
		if (num++ > 0) ss << ' ';
		ss << magic_enum::enum_name(*begin) << '(' << *begin << ')';
		std::advance(begin, 1);
	}
	return ss.str();
}

static vg::String join(auto iter)
{
	vg::StringStream ss;
	int num = 0;
	for (auto it : iter)
	{
		if (num++ > 0) ss << ' ';
		ss << magic_enum::enum_name(it) << '(' << it << ')';
	}
	return ss.str();
}

template <typename T> requires requires (T t) { std::is_enum_v<T>; }
constexpr bool IsValidFlags(T flags)
{
	auto valid_flags = magic_enum::enum_values<T>();
	uint64_t valid_mask = 0;
	for (auto flag : valid_flags)
	{
		valid_mask |= static_cast<uint64_t>(flag);
	}
	return (static_cast<uint64_t>(flags) & ~valid_mask) == 0;
}

template <typename T> requires requires (T t) { std::is_enum_v<T>; }
constexpr bool IsValidFlagsMask(T flags, uint64_t mask)
{
	return (static_cast<uint64_t>(flags) & ~mask) == 0;
}
#endif

static VgInitFlags init_flags;
bool debug = false;
VgMessageCallbackPFN messageCallback;
static VgAllocator allocator;
static bool wasInitialized = false;

struct Global
{
	vg::UnorderedMap<VgGraphicsApi, vg::Vector<VgAdapter>> adapters;
	vg::Set<VgGraphicsApi> unaskedGraphicsApis;

#if VG_VULKAN_SUPPORTED
	VulkanCore* vulkanCore;
#endif
};
static Global* s_global;

VgAllocator* GetVgAllocator() { return &allocator; }

constexpr std::string_view GraphicsApiToString(VgGraphicsApi api)
{
	switch (api)
	{
	case VG_GRAPHICS_API_AUTO: return "[Auto]";
	case VG_GRAPHICS_API_D3D12: return "D3D12";
	case VG_GRAPHICS_API_VULKAN: return "Vulkan";
	default: return "[unknown]";
	}
}

VG_API VgResult vgInit(const VgConfig* cfg)
{
	FUNC_DATA(vgInit);
	CHECK_NOT_NULL_RETURN(cfg);
#if VG_VALIDATION
	VALIDATE_FLAGS_RETURN(cfg->flags, "cfg->flags");
#endif
	if (wasInitialized)
	{
		return VG_ALREADY_INITIALIZED;
	}
	init_flags = cfg->flags;
	debug = cfg->flags & VG_INIT_DEBUG;
	messageCallback = ((cfg->flags & VG_INIT_ENABLE_MESSAGE_CALLBACK) && cfg->message_callback)
		? cfg->message_callback : nullptr;

	if (cfg->flags & VG_INIT_USE_PROVIDED_ALLOCATOR)
	{
		allocator = cfg->allocator;
	}
	else
	{
		allocator =
		{
			.user_data = nullptr,
			.alloc = [](void* _, size_t size, size_t alignment) -> void*
			{
#ifdef _MSC_VER
				return _aligned_malloc(size, alignment);
#else
				return std::aligned_alloc(alignment, size);
#endif
			},
			.realloc = [](void* _, void* original, size_t size, size_t alignment) -> void*
			{
#ifdef _MSC_VER
				return _aligned_realloc(original, size, alignment);
#else
				void* memory = std::aligned_malloc(alignment, size);
				std::memcpy(memory, original, size);
				std::free(original);
				return memory;
#endif
			},
			.free = [](void* _, void* memory)
			{
#ifdef _MSC_VER
				_aligned_free(memory);
#else
				std::free(memory);
#endif
			}
		};
	}

	s_global = new (GetAllocator().Allocate<Global>()) Global();

#if VG_VULKAN_SUPPORTED
	s_global->vulkanCore = VulkanCore::LoadVulkan(*cfg);
#endif

	uint32_t numAvailableApis;
	vgEnumerateApis(&numAvailableApis, nullptr);
	vg::Vector<VgGraphicsApi> availableApis(numAvailableApis);
	vgEnumerateApis(&numAvailableApis, availableApis.data());
	s_global->unaskedGraphicsApis.insert(availableApis.begin(), availableApis.end());

	wasInitialized = true;
	return VG_SUCCESS;
}

VG_API void vgShutdown()
{
	if (!wasInitialized) return;

	for (const auto& [api, apiAdapters] : s_global->adapters)
	{
		for (auto adapter : apiAdapters)
		{
			GetAllocator().Delete(adapter);
		}
	}

#if VG_VULKAN_SUPPORTED
	GetAllocator().Delete(s_global->vulkanCore);
#endif
	GetAllocator().Delete(s_global);
	s_global = nullptr;
	messageCallback = nullptr;
	allocator = {};
	wasInitialized = false;
}

static constexpr VgGraphicsApi SelectAutoGraphicsApi(VgGraphicsApi api)
{
	if (api != VG_GRAPHICS_API_AUTO) return api;

#if VG_D3D12_SUPPORTED && _WIN32
	return VG_GRAPHICS_API_D3D12;
#else
	return VG_GRAPHICS_API_VULKAN;
#endif
}

VG_API VgResult vgEnumerateApis(uint32_t* out_num_apis, VgGraphicsApi* out_apis)
{
	FUNC_DATA(vgEnumerateApis);
	CHECK_NOT_NULL_RETURN(out_num_apis);

	static std::array apis = {
#if VG_D3D12_SUPPORTED
		VG_GRAPHICS_API_D3D12,
#endif
#if VG_VULKAN_SUPPORTED
		VG_GRAPHICS_API_VULKAN,
#endif
	};

	*out_num_apis = static_cast<uint32_t>(apis.size());
	if (out_apis)
	{
		memcpy(out_apis, apis.data(), *out_num_apis * sizeof(apis[0]));
	}
	return VG_SUCCESS;
}

VG_API VgResult vgEnumerateAdapters(VgGraphicsApi api, VgSurface surface, uint32_t* out_num_adapters, VgAdapter* out_adapters)
{
	FUNC_DATA(vgEnumerateAdapters);
	CHECK_NOT_NULL_RETURN(surface);
	CHECK_NOT_NULL_RETURN(out_num_adapters);
#if VG_VALIDATION
	VALIDATE_ENUM_RETURN(api, "api");
#endif

	api = SelectAutoGraphicsApi(api);
	if (s_global->unaskedGraphicsApis.contains(api))
	{
		switch (api)
		{
#if VG_D3D12_SUPPORTED
		case VG_GRAPHICS_API_D3D12:
		{
			s_global->adapters[VG_GRAPHICS_API_D3D12] = D3D12Adapter::CollectAdapters();
			std::sort(s_global->adapters[api].begin(), s_global->adapters[api].end(),
				[](const auto& a, const auto& b) { return a->Score() > b->Score(); });
			break;
		}
#endif

#if VG_VULKAN_SUPPORTED
		case VG_GRAPHICS_API_VULKAN:
		{
			if (!s_global->vulkanCore) break;
			s_global->adapters[VG_GRAPHICS_API_VULKAN] = s_global->vulkanCore->CollectAdapters(surface);
			std::sort(s_global->adapters[api].begin(), s_global->adapters[api].end(),
				[](const auto& a, const auto& b) { return a->Score() > b->Score(); });
			break;
		};
#endif
		}

		s_global->unaskedGraphicsApis.erase(api);
	}

	if (!s_global->adapters.contains(api) || s_global->adapters[api].empty())
	{
		return VG_API_UNSUPPORTED;
	}

	auto& apiAdapters = s_global->adapters[api];
	*out_num_adapters = static_cast<uint32_t>(apiAdapters.size());
	if (out_adapters && !apiAdapters.empty())
	{
		memcpy(out_adapters, apiAdapters.data(), apiAdapters.size() * sizeof(VgAdapter));
	}
	return VG_SUCCESS;
}

VG_API VgResult vgAdapterGetProperties(VgAdapter adapter, VgAdapterProperties* out_properties)
{
	FUNC_DATA(vgAdapterGetProperties);
	CHECK_NOT_NULL_RETURN(adapter);
	CHECK_NOT_NULL_RETURN(out_properties);

	*out_properties = adapter->GetProperties();
	return VG_SUCCESS;
}

VG_API VgResult vgAdapterCreateDevice(VgAdapter adapter, VgDevice* out_device)
{
	FUNC_DATA(vgAdapterCreateDevice);
	CHECK_NOT_NULL_RETURN(out_device);

#if VG_FORCE_API == VG_API_D3D12
	try
	{
		*out_device = new(GetAllocator().Allocate<D3D12Device>()) D3D12Device(init_flags);
	}
	catch (VgError& ex)
	{
		LOG(ERROR, "Cannot create D3D12 device: {}", ex.what());
		return ex.result;
	}
	return VG_SUCCESS;
#else
	try
	{
		*out_device = adapter->CreateDevice(init_flags);
	}
	catch (VgError& ex)
	{
		LOG(ERROR, "Cannot create device: {}", ex.what());
		return ex.result;
	}
	return VG_SUCCESS;
#endif
}

VG_API void vgAdapterDestroyDevice(VgAdapter adapter, VgDevice device)
{
	FUNC_DATA(vgAdapterDestroyDevice);
	CHECK_NOT_NULL(device);

	GetAllocator().Delete(device);
}

VgResult vgDeviceGetApiObject(VgDevice device, void** out_obj)
{
	FUNC_DATA(vgDeviceGetApiObject);
	CHECK_NOT_NULL_RETURN(device);
	CHECK_NOT_NULL_RETURN(out_obj);

	*out_obj = device->GetApiObject();
	return VG_SUCCESS;
}

VG_API VgResult vgDeviceGetAdapter(VgDevice device, VgAdapter* out_adapter)
{
	FUNC_DATA(vgDeviceGetAdapter);
	CHECK_NOT_NULL_RETURN(device);
	CHECK_NOT_NULL_RETURN(out_adapter);

	*out_adapter = device->Adapter();
	return VG_SUCCESS;
}

VgResult vgDeviceGetGraphicsApi(VgDevice device, VgGraphicsApi* out_api)
{
	FUNC_DATA(vgDeviceGetGraphicsApi);
	CHECK_NOT_NULL_RETURN(device);
	CHECK_NOT_NULL_RETURN(out_api);

	*out_api = device->Api();
	return VG_SUCCESS;
}

VgResult vgDeviceGetMemoryStatistics(VgDevice device, VgMemoryStatistics* out_memory_stats)
{
	FUNC_DATA(vgDeviceGetMemoryStatistics);
	CHECK_NOT_NULL_RETURN(device);
	CHECK_NOT_NULL_RETURN(out_memory_stats);

	*out_memory_stats = device->GetMemoryStatistics();
	return VG_SUCCESS;
}

void vgDeviceWaitQueueIdle(VgDevice device, VgQueue queue)
{
	FUNC_DATA(vgDeviceWaitQueueIdle);
	CHECK_NOT_NULL(device);

#if VG_VALIDATION
	VALIDATE_ENUM(queue, "queue");
#endif
	device->WaitQueueIdle(queue);
}

void vgDeviceWaitIdle(VgDevice device)
{
	FUNC_DATA(vgDeviceWaitIdle);
	CHECK_NOT_NULL(device);

	device->WaitIdle();
}

VgResult vgDeviceCreateBuffer(VgDevice device, const VgBufferDesc* desc, VgBuffer* out_buffer)
{
	FUNC_DATA(vgDeviceCreateBuffer);
	CHECK_NOT_NULL_RETURN(device);
	CHECK_NOT_NULL_RETURN(desc);
	CHECK_NOT_NULL_RETURN(out_buffer);

#if VG_VALIDATION
	VALIDATE_ENUM_RETURN(desc->usage, "usage");
#endif

	VgBufferDesc newDesc = *desc;
	if (desc->usage == VG_BUFFER_USAGE_CONSTANT && desc->size % 256 != 0)
	{
		newDesc.size = (desc->size + 255) & ~255;
		LOG(DEBUG, "{}(): constant buffer size({}) must be a multiple of 256; overwritten to {}", _func_name_, desc->size, newDesc.size);
	}

	try
	{
		*out_buffer = device->CreateBuffer(newDesc);
		return VG_SUCCESS;
	}
	catch (VgError& ex)
	{
		LOG(ERROR, "Cannot create buffer: {}", ex.what());
		return ex.result;
	}
}

void vgDeviceDestroyBuffer(VgDevice device, VgBuffer buffer)
{
	FUNC_DATA(vgDeviceDestroyBuffer);
	CHECK_NOT_NULL(device);
	CHECK_NOT_NULL(buffer);
	
	device->DestroyBuffer(buffer);
}

VgResult vgDeviceCreateShaderModule(VgDevice device, const void* data, uint64_t size, VgShaderModule* out_module)
{
	FUNC_DATA(vgDeviceCreateShaderModule);
	CHECK_NOT_NULL_RETURN(device);
	CHECK_NOT_NULL_RETURN(data);
	CHECK_NOT_NULL_RETURN(out_module);
	if (size < 1)
	{
		LOG(ERROR, "{}(): size = 0", _func_name_);
		return VG_BAD_ARGUMENT;
	}
	try
	{
		*out_module = device->CreateShaderModule(data, size);
	}
	catch (VgError& ex)
	{
		LOG(ERROR, "Cannot create shader module: {}", ex.what());
		return ex.result;
	}
	return VG_SUCCESS;
}

void vgDeviceDestroyShaderModule(VgDevice device, VgShaderModule shader_module)
{
	FUNC_DATA(vgDeviceDestroyShaderModule);
	CHECK_NOT_NULL(device);
	CHECK_NOT_NULL(shader_module);
	
	device->DestroyShaderModule(shader_module);
}

VgResult vgDeviceCreateGraphicsPipeline(VgDevice device, const VgGraphicsPipelineDesc* desc, VgPipeline* out_pipeline)
{
	FUNC_DATA(vgDeviceCreateGraphicsPipeline);
	CHECK_NOT_NULL_RETURN(device);
	CHECK_NOT_NULL_RETURN(desc);
	CHECK_NOT_NULL_RETURN(out_pipeline);

#if VG_VALIDATION
	VALIDATE_ENUM_RETURN(desc->vertex_pipeline_type, "vertex_pipeline_type");
#endif

	if (desc->vertex_pipeline_type == VG_VERTEX_PIPELINE_FIXED_FUNCTION)
	{
		if (desc->fixed_function.num_vertex_attributes > 0 && !desc->fixed_function.vertex_attributes)
		{
			LOG(ERROR, "{}(): num_vertex_attributes({}) > 0 but vertex_attributes is NULL", _func_name_, desc->fixed_function.num_vertex_attributes);
			return VG_BAD_ARGUMENT;
		}
		if (desc->fixed_function.num_vertex_attributes > vg_num_max_vertex_attributes)
		{
			LOG(ERROR, "{}(): num_vertex_attributes({}) should not exceed vg_num_max_vertex_attributes({})",
				_func_name_, desc->fixed_function.num_vertex_attributes, vg_num_max_vertex_attributes);
			return VG_BAD_ARGUMENT;
		}
#if VG_VALIDATION
		for (uint32_t i = 0; i < desc->fixed_function.num_vertex_attributes; i++)
		{
			VALIDATE_ENUM_RETURN(desc->fixed_function.vertex_attributes[i].format, "vertex_attributes[{}].format", i);
			VALIDATE_ENUM_RETURN(desc->fixed_function.vertex_attributes[i].input_rate, "vertex_attributes[{}].input_rate", i);
			if (desc->fixed_function.vertex_attributes[i].input_rate == VG_ATTRIBUTE_INPUT_RATE_INSTANCE
				&& desc->fixed_function.vertex_attributes[i].instance_step_rate == 0)
			{
				LOG(ERROR, "{}(): vertex_attribute {}: input_rate is {} but instance_step_rate is 0",
					_func_name_, i, magic_enum::enum_name(desc->fixed_function.vertex_attributes[i].input_rate));
				return VG_BAD_ARGUMENT;
			}
		}
#endif

		CHECK_NOT_NULL_RETURN(desc->fixed_function.vertex_shader);
		if (desc->fixed_function.hull_shader) CHECK_NOT_NULL_RETURN(desc->fixed_function.domain_shader);
		else if (desc->fixed_function.domain_shader) CHECK_NOT_NULL_RETURN(desc->fixed_function.hull_shader);
	}
	else if (desc->vertex_pipeline_type == VG_VERTEX_PIPELINE_MESH_SHADER)
	{
		CHECK_NOT_NULL_RETURN(desc->mesh.mesh_shader);
	}

	if (!desc->rasterization_state.rasterization_discard_enable) CHECK_NOT_NULL_RETURN(desc->pixel_shader);
	if (desc->fixed_function.domain_shader && (desc->tesselation_control_points < 1 || desc->tesselation_control_points > 32))
	{
		LOG(ERROR, "{}(): tesselation_control_points({}) should be in range 1..32 when domain_shader and hull_shader are set",
			_func_name_, desc->tesselation_control_points);
		return VG_FAILURE;
	}

#if VG_VALIDATION
	VALIDATE_ENUM_RETURN(desc->primitive_topology, "primitive_topology");

	if (desc->primitive_restart_enable)
	{
		VALIDATE_ENUM_ALLOWED_RETURN((vg::UnorderedSet{VG_PRIMITIVE_TOPOLOGY_LINE_STRIP, VG_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
			VG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, VG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY}), desc->primitive_topology,
			"primitive_topology [with primitive_restart = true]");
	}
	if (!desc->rasterization_state.rasterization_discard_enable)
	{
		VALIDATE_ENUM_RETURN(desc->rasterization_state.fill_mode, "rasterization_state.fill_mode");
		VALIDATE_ENUM_RETURN(desc->rasterization_state.cull_mode, "rasterization_state.cull_mode");
		VALIDATE_ENUM_RETURN(desc->rasterization_state.front_face, "rasterization_state.front_face");
		VALIDATE_ENUM_RETURN(desc->rasterization_state.depth_clip_mode, "rasterization_state.depth_clip_mode");
		if (desc->rasterization_state.conservative_rasterization_enable)
		{
			VALIDATE_ENUM_ALLOWED_RETURN((vg::UnorderedSet{ VG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
				VG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY, VG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY }),
				desc->primitive_topology, "primitive_topology [with rasterization_state.conservative_rasterization_enable = true]");
			VALIDATE_ENUM_ALLOWED_RETURN((vg::UnorderedSet{ VG_FILL_MODE_FILL }), desc->rasterization_state.fill_mode,
				"rasterization_state.fill_mode [with rasterization_state.conservative_rasterization_enable = true]");
		}
	}
	VALIDATE_ENUM_RETURN(desc->multisampling_state.sample_count, "multisampling_state.sample_count");
	if (desc->multisampling_state.alpha_to_coverage)
	{
		VALIDATE_ENUM_ALLOWED_RETURN((vg::UnorderedSet{ VG_SAMPLE_COUNT_2, VG_SAMPLE_COUNT_4, VG_SAMPLE_COUNT_8 }),
			desc->multisampling_state.sample_count, "multisampling_state.sample_count [with multisampling_state.alpha_to_coverage = true]");
	}
	if (desc->depth_stencil_state.depth_test_enable)
	{
		VALIDATE_ENUM_RETURN(desc->depth_stencil_state.depth_compare_op, "depth_stencil_state.depth_compare_op");
	}
	if (desc->depth_stencil_state.stencil_test_enable)
	{
		VALIDATE_ENUM_RETURN(desc->depth_stencil_state.front.fail_op, "depth_stencil_state.front.fail_op");
		VALIDATE_ENUM_RETURN(desc->depth_stencil_state.front.depth_fail_op, "depth_stencil_state.front.depth_fail_op");
		VALIDATE_ENUM_RETURN(desc->depth_stencil_state.front.pass_op, "depth_stencil_state.front.pass_op");
		VALIDATE_ENUM_RETURN(desc->depth_stencil_state.front.compare_op, "depth_stencil_state.front.compare_op");
		VALIDATE_ENUM_RETURN(desc->depth_stencil_state.back.fail_op, "depth_stencil_state.back.fail_op");
		VALIDATE_ENUM_RETURN(desc->depth_stencil_state.back.depth_fail_op, "depth_stencil_state.back.depth_fail_op");
		VALIDATE_ENUM_RETURN(desc->depth_stencil_state.back.pass_op, "depth_stencil_state.back.pass_op");
		VALIDATE_ENUM_RETURN(desc->depth_stencil_state.back.compare_op, "depth_stencil_state.back.compare_op");
	}
	if (desc->depth_stencil_state.depth_bounds_test_enable)
	{
		if (!desc->depth_stencil_state.depth_bounds_test_enable)
		{
			LOG(WARN, "{}(): depth_stencil_state.depth_bounds_test_enable is true but depth_stencil_state.depth_test_enable is false",
				_func_name_);
		}
		else
		{
			if (desc->depth_stencil_state.min_depth_bounds < 0.0f || desc->depth_stencil_state.min_depth_bounds > 1.0f)
			{
				LOG(ERROR, "{}(): depth_stencil_state.min_depth_bounds({}) should be in range 0..1",
					_func_name_, desc->depth_stencil_state.min_depth_bounds);
				return VG_BAD_ARGUMENT;
			}
			if (desc->depth_stencil_state.max_depth_bounds < 0.0f || desc->depth_stencil_state.max_depth_bounds > 1.0f)
			{
				LOG(ERROR, "{}(): depth_stencil_state.max_depth_bounds({}) should be in range 0..1",
					_func_name_, desc->depth_stencil_state.max_depth_bounds);
				return VG_BAD_ARGUMENT;
			}
			if (desc->depth_stencil_state.min_depth_bounds > desc->depth_stencil_state.max_depth_bounds)
			{
				LOG(ERROR, "{}(): depth_stencil_state.min_depth_bounds({}) should be <= depth_stencil_state.max_depth_bounds({})",
					_func_name_, desc->depth_stencil_state.min_depth_bounds, desc->depth_stencil_state.max_depth_bounds);
				return VG_BAD_ARGUMENT;
			}
		}
	}
	for (uint32_t i = 0; i < desc->num_color_attachments; i++)
	{
		VALIDATE_ENUM_RETURN(desc->color_attachment_formats[i], "color_attachment_formats[{}]", i);
	}
	VALIDATE_ENUM_RETURN(desc->depth_stencil_format, "depth_stencil_format");
	if (desc->num_color_attachments == 0 && desc->depth_stencil_format == VG_FORMAT_UNKNOWN)
	{
		LOG(ERROR, "{}(): num_color_attachments = 0 and depth_stencil_format = {}", _func_name_, magic_enum::enum_name(VG_FORMAT_UNKNOWN));
		return VG_BAD_ARGUMENT;
	}
	if (desc->blend_state.logic_op_enable)
	{
		VALIDATE_ENUM_RETURN(desc->blend_state.logic_op, "blend_state.logic_op");
	}
	for (uint32_t i = 0; i < desc->num_color_attachments; i++)
	{
		const auto& attachment = desc->blend_state.attachments[i];
		if (desc->blend_state.logic_op_enable && attachment.blend_enable)
		{
			LOG(ERROR, "{}(): blend_state.attachments[{}].blend_enable should be false when blend_state.logic_op_enable = true",
				_func_name_, i);
			return VG_BAD_ARGUMENT;
		}
		if (attachment.blend_enable)
		{
			VALIDATE_ENUM_RETURN(attachment.src_color, "blend_state.attachments[{}].src_color", i);
			VALIDATE_ENUM_RETURN(attachment.dst_color, "blend_state.attachments[{}].dst_color", i);
			VALIDATE_ENUM_RETURN(attachment.color_op, "blend_state.attachments[{}].color_op", i);
			VALIDATE_ENUM_RETURN(attachment.src_alpha, "blend_state.attachments[{}].src_alpha", i);
			VALIDATE_ENUM_RETURN(attachment.dst_alpha, "blend_state.attachments[{}].dst_alpha", i);
			VALIDATE_ENUM_RETURN(attachment.alpha_op, "blend_state.attachments[{}].alpha_op", i);
		}
		VALIDATE_FLAGS_RETURN(attachment.color_write_mask, "blend_state.attachments[{}].color_write_mask", i);
	}
#endif

	try
	{
		*out_pipeline = device->CreateGraphicsPipeline(*desc);
	}
	catch (VgError& ex)
	{
		LOG(ERROR, "Cannot create graphics pipeline: {}", ex.what());
		return ex.result;
	}
	return VG_SUCCESS;
}

VgResult vgDeviceCreateComputePipeline(VgDevice device, VgShaderModule shader_module, VgPipeline* out_pipeline)
{
	FUNC_DATA(vgDeviceCreateComputePipeline);
	CHECK_NOT_NULL_RETURN(device);
	CHECK_NOT_NULL_RETURN(shader_module);
	CHECK_NOT_NULL_RETURN(out_pipeline);
	try
	{
		*out_pipeline = device->CreateComputePipeline(shader_module);
	}
	catch (VgError& ex)
	{
		LOG(ERROR, "Cannot create compute pipeline: {}", ex.what());
		return ex.result;
	}
	return VG_SUCCESS;
}

void vgDeviceDestroyPipeline(VgDevice device, VgPipeline pipeline)
{
	FUNC_DATA(vgDeviceDestroyPipeline);
	CHECK_NOT_NULL(device);
	CHECK_NOT_NULL(pipeline);

	device->DestroyPipeline(pipeline);
}

VgResult vgDeviceCreateFence(VgDevice device, uint64_t initial_value, VgFence* out_fence)
{
	FUNC_DATA(vgDeviceCreateFence);
	CHECK_NOT_NULL_RETURN(device);
	CHECK_NOT_NULL_RETURN(out_fence);
	try
	{
		*out_fence = device->CreateFence(initial_value);
	}
	catch (VgError& ex)
	{
		LOG(ERROR, "Cannot create fence: {}", ex.what());
		return ex.result;
	}
	return VG_SUCCESS;
}

void vgDeviceDestroyFence(VgDevice device, VgFence fence)
{
	FUNC_DATA(vgDeviceDestroyFence);
	CHECK_NOT_NULL(device);
	CHECK_NOT_NULL(fence);
	
	device->DestroyFence(fence);
}

VgResult vgDeviceCreateCommandPool(VgDevice device, VgCommandPoolFlags flags, VgQueue queue, VgCommandPool* out_pool)
{
	FUNC_DATA(vgDeviceCreateCommandPool);
	CHECK_NOT_NULL_RETURN(device);
	CHECK_NOT_NULL_RETURN(out_pool);

#if VG_VALIDATION
	VALIDATE_FLAGS_RETURN(flags, "flags");
	VALIDATE_ENUM_RETURN(queue, "queue");
#endif
	try
	{
		*out_pool = device->CreateCommandPool(flags, queue);
	}
	catch (VgError& ex)
	{
		LOG(ERROR, "Cannot create command pool: {}", ex.what());
		return ex.result;
	}
	return VG_SUCCESS;
}

void vgDeviceDestroyCommandPool(VgDevice device, VgCommandPool pool)
{
	FUNC_DATA(vgDeviceDestroyCommandPool);
	CHECK_NOT_NULL(device);
	CHECK_NOT_NULL(pool);

	device->DestroyCommandPool(pool);
}

VgResult vgDeviceCreateSampler(VgDevice device, const VgSamplerDesc* desc, VgSampler* out_sampler)
{
	FUNC_DATA(vgDeviceCreateSampler);
	CHECK_NOT_NULL_RETURN(device);
	CHECK_NOT_NULL_RETURN(desc);
	CHECK_NOT_NULL_RETURN(out_sampler);

	auto descCopy = *desc;
#if VG_VALIDATION
	VALIDATE_ENUM_RETURN(descCopy.mag_filter, "mag_filter");
	VALIDATE_ENUM_RETURN(descCopy.min_filter, "min_filter");
	VALIDATE_ENUM_RETURN(descCopy.mipmap_mode, "mipmap_mode");
	VALIDATE_ENUM_RETURN(descCopy.address_u, "address_u");
	VALIDATE_ENUM_RETURN(descCopy.address_v, "address_v");
	VALIDATE_ENUM_RETURN(descCopy.address_w, "address_w");
	VALIDATE_ENUM_RETURN(descCopy.max_anisotropy, "max_anisotropy");
	VALIDATE_ENUM_RETURN(descCopy.reduction_mode, "reduction_mode");

	if (descCopy.reduction_mode != VG_REDUCTION_MODE_DEFAULT)
	{
		if (descCopy.max_anisotropy > VG_ANISOTROPY_1)
		{
			LOG(ERROR, "vgDeviceCreateSampler(): reduction_mode({}) should be {} when max_anisotropy > {}",
				magic_enum::enum_name(desc->max_anisotropy), magic_enum::enum_name(VG_REDUCTION_MODE_DEFAULT),
				magic_enum::enum_name(VG_ANISOTROPY_1));
			return VG_BAD_ARGUMENT;
		}
		if (descCopy.comparison_func != VG_COMPARISON_FUNC_NONE)
		{
			LOG(ERROR, "vgDeviceCreateSampler(): comparison_func({}) should be {} when reduction_mode is not {}",
				magic_enum::enum_name(desc->comparison_func), magic_enum::enum_name(VG_COMPARISON_FUNC_NONE),
				magic_enum::enum_name(VG_REDUCTION_MODE_DEFAULT));
			return VG_BAD_ARGUMENT;
		}
	}
	if (descCopy.max_anisotropy > VG_ANISOTROPY_1 && (descCopy.min_filter != VG_FILTER_LINEAR
		|| descCopy.mag_filter != VG_FILTER_LINEAR || descCopy.mipmap_mode != VG_MIPMAP_MODE_LINEAR))
	{
		LOG(WARN, "vgDeviceCreateSampler(): when max_anisotropy({}) > {}, min_filter({}) and mag_filter({}) will always be {},"
			"mipmap_mode({}) will always be {}", magic_enum::enum_name(descCopy.max_anisotropy),
			magic_enum::enum_name(VG_ANISOTROPY_1), magic_enum::enum_name(descCopy.min_filter),
			magic_enum::enum_name(descCopy.mag_filter), magic_enum::enum_name(VG_FILTER_LINEAR),
			magic_enum::enum_name(descCopy.mipmap_mode), magic_enum::enum_name(VG_MIPMAP_MODE_LINEAR));
		descCopy.min_filter = VG_FILTER_LINEAR;
		descCopy.mag_filter = VG_FILTER_LINEAR;
		descCopy.mipmap_mode = VG_MIPMAP_MODE_LINEAR;
	}
#endif
	try
	{
		*out_sampler = device->CreateSampler(*desc);
	}
	catch (VgError& ex)
	{
		LOG(ERROR, "Cannot create sampler: {}", ex.what());
		return ex.result;
	}
	return VG_SUCCESS;
}

void vgDeviceDestroySampler(VgDevice device, VgSampler sampler)
{
	FUNC_DATA(vgDeviceDestroySampler);
	CHECK_NOT_NULL(device);
	CHECK_NOT_NULL(sampler);

	device->DestroySampler(sampler);
}

VgResult vgDeviceCreateTexture(VgDevice device, const VgTextureDesc* desc, VgTexture* out_texture)
{
	FUNC_DATA(vgDeviceCreateTexture);
	CHECK_NOT_NULL_RETURN(device);
	CHECK_NOT_NULL_RETURN(desc);
	CHECK_NOT_NULL_RETURN(out_texture);

#if VG_VALIDATION
	VALIDATE_ENUM_RETURN(desc->type, "type");
	VALIDATE_ENUM_RETURN(desc->format, "format");
	if (desc->width == 0)
	{
		LOG(ERROR, "{}(): width = 0, but should be > 0", _func_name_);
		return VG_BAD_ARGUMENT;
	}
	if (desc->height == 0)
	{
		LOG(ERROR, "{}(): height = 0, but should be > 0", _func_name_);
		return VG_BAD_ARGUMENT;
	}
	if (desc->depth_or_array_layers == 0)
	{
		LOG(ERROR, "{}(): depth_or_array_layers = 0, but should be > 0", _func_name_);
		return VG_BAD_ARGUMENT;
	}
	if (desc->mip_levels == 0)
	{
		LOG(ERROR, "{}(): mip_levels = 0, but should be > 0", _func_name_);
		return VG_BAD_ARGUMENT;
	}
	VALIDATE_ENUM_RETURN(desc->sample_count, "sample_count");
	VALIDATE_FLAGS_RETURN(desc->usage, "usage");
	VALIDATE_DISALLOWED_FLAGS_RETURN(desc->usage, VG_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT,
		VG_TEXTURE_USAGE_ALLOW_SIMULTANEOUS_ACCESS | VG_TEXTURE_USAGE_COLOR_ATTACHMENT,
		"usage");
	if ((desc->usage & (VG_TEXTURE_USAGE_UNORDERED_ACCESS | VG_TEXTURE_USAGE_ALLOW_SIMULTANEOUS_ACCESS))
		&& desc->sample_count > VG_SAMPLE_COUNT_1)
	{
		LOG(ERROR, "{}(): VG_TEXTURE_USAGE_UNORDERED_ACCESS and VG_TEXTURE_USAGE_UNORDERED_ACCESS usage is illegal when sample_count is not VG_SAMPLE_COUNT_1", _func_name_);
		return VG_BAD_ARGUMENT;
	}
	if (desc->sample_count > 1 && !(desc->usage & (VG_TEXTURE_USAGE_COLOR_ATTACHMENT | VG_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT)))
	{
		LOG(ERROR, R"({}(): sample_count({}) should be VG_SAMPLE_COUNT_1 when neither 
			VG_TEXTURE_USAGE_COLOR_ATTACHMENT or VG_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT usage flags are set)",
			_func_name_, static_cast<uint64_t>(desc->sample_count));
		return VG_BAD_ARGUMENT;
	}
#endif
	try
	{
		*out_texture = device->CreateTexture(*desc);
	}
	catch (VgError& ex)
	{
		LOG(ERROR, "{}() failed: {}", _func_name_ , ex.what());
		return ex.result;
	}
	return VG_SUCCESS;
}

void vgDeviceDestroyTexture(VgDevice device, VgTexture texture)
{
	FUNC_DATA(vgDeviceDestroyTexture);
	CHECK_NOT_NULL(device);
	CHECK_NOT_NULL(texture);
	if (texture->OwnedBySwapChain())
	{
		LOG(ERROR, "{}(): Unable to destroy texture which is owned by swap chain", _func_name_);
		return;
	}

	device->DestroyTexture(texture);
}

void vgDeviceSubmitCommandLists(VgDevice device, uint32_t num_submits, VgSubmitInfo* submits)
{
	FUNC_DATA(vgDeviceSubmitCommandLists);
	CHECK_NOT_NULL(device);
	CHECK_NOT_NULL(submits);

#if VG_VALIDATION
	for (uint32_t i = 0; i < num_submits; i++)
	{
		if (submits[i].num_wait_fences > 0 && submits[i].wait_fences == nullptr)
		{
			LOG(ERROR, "{}(): submit {} num_wait_fences = {} but wait_fences = NULL", _func_name_, i, submits[i].num_wait_fences);
			return;
		}
		if (submits[i].num_command_lists > 0 && submits[i].command_lists == nullptr)
		{
			LOG(ERROR, "{}(): submit {} num_command_lists = {} but command_lists = NULL", _func_name_, i, submits[i].num_command_lists);
			return;
		}
		if (submits[i].num_signal_fences > 0 && submits[i].signal_fences == nullptr)
		{
			LOG(ERROR, "{}(): submit {} num_signal_fences = {} but signal_fences = NULL", _func_name_, i, submits[i].num_signal_fences);
			return;
		}
		for (uint32_t j = 0; j < submits[i].num_wait_fences; j++)
		{
			if (submits[i].wait_fences[j].fence == nullptr)
			{
				LOG(ERROR, "{}(): submit {} wait fence {}: fence = NULL", _func_name_, i, j);
				return;
			}
		}
		for (uint32_t j = 0; j < submits[i].num_command_lists; j++)
		{
			if (submits[i].command_lists[j] == nullptr)
			{
				LOG(ERROR, "{}(): submit {} command list {} = NULL", _func_name_, i, j);
				return;
			}
		}
		for (uint32_t j = 0; j < submits[i].num_signal_fences; j++)
		{
			if (submits[i].signal_fences[j].fence == nullptr)
			{
				LOG(ERROR, "{}(): submit {} signal fence {}: fence = NULL", _func_name_, i, j);
				return;
			}
		}
	}
#endif
	device->SubmitCommandLists(num_submits, submits);
}

VgResult vgDeviceSignalFence(VgDevice device, VgFence fence, uint64_t value)
{
	FUNC_DATA(vgDeviceSignalFence);
	CHECK_NOT_NULL_RETURN(device);
	CHECK_NOT_NULL_RETURN(fence);

	try
	{
		device->SignalFence(fence, value);
	}
	catch (VgError& ex)
	{
		LOG(ERROR, "{}() failed: {}", _func_name_, ex.what());
		return ex.result;
	}
	return VG_SUCCESS;
}

void vgDeviceWaitFence(VgDevice device, VgFence fence, uint64_t value)
{
	FUNC_DATA(vgDeviceWaitFence);
	CHECK_NOT_NULL(device);
	CHECK_NOT_NULL(fence);

	device->WaitFence(fence, value);
}

VgResult vgDeviceGetFenceValue(VgDevice device, VgFence fence, uint64_t* out_value)
{
	FUNC_DATA(vgDeviceGetFenceValue);
	CHECK_NOT_NULL_RETURN(device);
	CHECK_NOT_NULL_RETURN(fence);
	CHECK_NOT_NULL_RETURN(out_value);

	try
	{
		*out_value = device->GetFenceValue(fence);
	}
	catch (VgError& ex)
	{
		LOG(ERROR, "{}() failed: {}", _func_name_, ex.what());
		return ex.result;
	}
	return VG_SUCCESS;
}

VgResult vgDeviceCreateSwapChain(VgDevice device, const VgSwapChainDesc* desc, VgSwapChain* out_swap_chain)
{
	FUNC_DATA(vgDeviceCreateSwapChain);
	CHECK_NOT_NULL_RETURN(device);
	CHECK_NOT_NULL_RETURN(desc);
	CHECK_NOT_NULL_RETURN(out_swap_chain);
	CHECK_NOT_NULL_RETURN(desc->surface);
#if VG_VALIDATION
	// ...
#endif

	try
	{
		*out_swap_chain = device->CreateSwapChain(*desc);
	}
	catch (VgError& ex)
	{
		LOG(ERROR, "Unable to create swap chain: {}", ex.what());
		return ex.result;
	}
	return VG_SUCCESS;
}

void vgDeviceDestroySwapChain(VgDevice device, VgSwapChain swap_chain)
{
	FUNC_DATA(vgDeviceDestroySwapChain);
	CHECK_NOT_NULL(device);
	CHECK_NOT_NULL(swap_chain);

	device->DestroySwapChain(swap_chain);
}

VgResult vgCommandPoolGetApiObject(VgCommandPool pool, void** out_obj)
{
	FUNC_DATA(vgCommandPoolGetApiObject);
	CHECK_NOT_NULL_RETURN(pool);
	CHECK_NOT_NULL_RETURN(out_obj);

	*out_obj = pool->GetApiObject();
	return VG_SUCCESS;
}

void vgCommandPoolSetName(VgCommandPool pool, const char* name)
{
	FUNC_DATA(vgCommandPoolSetName);
	CHECK_NOT_NULL(pool);
	CHECK_NOT_NULL(name);

	pool->SetName(name);
}

VgResult vgCommandPoolGetDevice(VgCommandPool pool, VgDevice* out_device)
{
	FUNC_DATA(vgCommandPoolGetDevice);
	CHECK_NOT_NULL_RETURN(pool);
	CHECK_NOT_NULL_RETURN(out_device);
	
	*out_device = pool->Device();
	return VG_SUCCESS;
}

VgResult vgCommandPoolGetQueue(VgCommandPool pool, VgQueue* out_queue)
{
	FUNC_DATA(vgCommandPoolGetQueue);
	CHECK_NOT_NULL_RETURN(pool);
	CHECK_NOT_NULL_RETURN(out_queue);

	*out_queue = pool->Queue();
	return VG_SUCCESS;
}

VgResult vgCommandPoolAllocateCommandList(VgCommandPool pool, VgCommandList* out_cmd)
{
	FUNC_DATA(vgCommandPoolAllocateCommandList);
	CHECK_NOT_NULL_RETURN(pool);
	CHECK_NOT_NULL_RETURN(out_cmd);

	try
	{
		*out_cmd = pool->AllocateCommandList();
	}
	catch (VgError& ex)
	{
		LOG(ERROR, "{}() failed: {}", _func_name_, ex.what());
		return ex.result;
	}
	return VG_SUCCESS;
}

void vgCommandPoolFreeCommandList(VgCommandPool pool, VgCommandList cmd)
{
	FUNC_DATA(vgCommandPoolFreeCommandList);
	CHECK_NOT_NULL(pool);
	CHECK_NOT_NULL(cmd);

	pool->FreeCommandList(cmd);
}

void vgCommandPoolReset(VgCommandPool pool)
{
	FUNC_DATA(vgCommandPoolReset);
	CHECK_NOT_NULL(pool);

	pool->Reset();
}

VgResult vgCommandListGetApiObject(VgCommandList list, void** out_obj)
{
	FUNC_DATA(vgCommandListGetApiObject);
	CHECK_NOT_NULL_RETURN(list);
	CHECK_NOT_NULL_RETURN(out_obj);

	*out_obj = list->GetApiObject();
	return VG_SUCCESS;
}

void vgCommandListSetName(VgCommandList list, const char* name)
{
	FUNC_DATA(vgCommandListSetName);
	CHECK_NOT_NULL(list);
	CHECK_NOT_NULL(name);

	list->SetName(name);
}

VgResult vgCommandListGetDevice(VgCommandList cmd, VgDevice* out_device)
{
	FUNC_DATA(vgCommandListGetDevice);
	CHECK_NOT_NULL_RETURN(cmd);
	CHECK_NOT_NULL_RETURN(out_device);

	*out_device = cmd->Device();
	return VG_SUCCESS;
}

VgResult vgCommandListGetCommandPool(VgCommandList cmd, VgCommandPool* out_pool)
{
	FUNC_DATA(vgCommandListGetCommandPool);
	CHECK_NOT_NULL_RETURN(cmd);
	CHECK_NOT_NULL_RETURN(out_pool);

	*out_pool = cmd->CommandPool();
	return VG_SUCCESS;
}

VgResult vgCommandListGetQueue(VgCommandList cmd, VgQueue* out_queue)
{
	FUNC_DATA(vgCommandListGetQueue);
	CHECK_NOT_NULL_RETURN(cmd);
	CHECK_NOT_NULL_RETURN(out_queue);

	*out_queue = cmd->CommandPool()->Queue();
	return VG_SUCCESS;
}

void vgCommandListRestoreDescriptorState(VgCommandList cmd)
{
	FUNC_DATA(vgCommandListRestoreDescriptorState);
	CHECK_NOT_NULL(cmd);

	cmd->RestoreDescriptorState();
}

void vgCmdBegin(VgCommandList cmd)
{
	FUNC_DATA(vgCmdBegin);
	CHECK_NOT_NULL(cmd);

	cmd->Begin();
}

void vgCmdEnd(VgCommandList cmd)
{
	FUNC_DATA(vgCmdEnd);
	CHECK_NOT_NULL(cmd);

	cmd->End();
}

void vgCmdSetVertexBuffers(VgCommandList cmd, uint32_t start_slot, uint32_t num_buffers, const VgVertexBufferView* buffers)
{
	FUNC_DATA(vgCmdSetVertexBuffers);
	CHECK_NOT_NULL(cmd);
	if (num_buffers > 0 && !buffers)
	{
		LOG(WARN, "{}(): buffers = NULL", _func_name_);
		return;
	}
	if (num_buffers > vg_num_max_vertex_buffers)
	{
		LOG(WARN, "{}(): num_buffers({}) should not exceed vg_num_max_vertex_buffers({}) -> clamped", _func_name_,
			num_buffers, vg_num_max_vertex_buffers);
		num_buffers = vg_num_max_vertex_buffers;
	}
#if VG_VALIDATION
	if (cmd->CommandPool()->Queue() != VG_QUEUE_GRAPHICS)
	{
		LOG(ERROR, "{}(): command list queue should be VG_QUEUE_GRAPHICS", _func_name_);
		return;
	}
	for (uint32_t i = 0; i < num_buffers; i++)
	{
		if (!buffers[i].buffer)
		{
			LOG(ERROR, "{}(): buffer {} = NULL", _func_name_, i);
			return;
		}
		if (buffers[i].buffer->Desc().usage != VG_BUFFER_USAGE_GENERAL)
		{
			LOG(ERROR, "{}(): buffer {} should be of usage GENERAL", _func_name_, i);
			return;
		}
	}
#endif
	cmd->SetVertexBuffers(start_slot, num_buffers, buffers);
}

void vgCmdSetIndexBuffer(VgCommandList cmd, VgIndexType index_type, uint64_t offset, VgBuffer index_buffer)
{
	FUNC_DATA(vgCmdSetIndexBuffer);
	CHECK_NOT_NULL(cmd);
	CHECK_NOT_NULL(index_buffer);

#if VG_VALIDATION
	if (cmd->CommandPool()->Queue() != VG_QUEUE_GRAPHICS)
	{
		LOG(ERROR, "{}(): command list queue should be VG_QUEUE_GRAPHICS", _func_name_);
		return;
	}
	VALIDATE_ENUM(index_type, "index_type");
#endif
	cmd->SetIndexBuffer(index_type, offset, index_buffer);
}

void vgCmdSetRootConstants(VgCommandList cmd, VgPipelineType pipeline_type,
	uint32_t offset_in_32bit_values, uint32_t num_32bit_values, const void* data)
{
	FUNC_DATA(vgCmdSetRootConstants);
	CHECK_NOT_NULL(cmd);
	if (num_32bit_values > 0 && !data)
	{
		LOG(WARN, "{}(): data = NULL but num_32bit_values > 0", _func_name_);
		return;
	}
	if (offset_in_32bit_values + num_32bit_values > vg_num_allowed_root_constants)
	{
		LOG(WARN, "{}(): offset {} + count {} > allowed {}; truncating",
			_func_name_, offset_in_32bit_values, num_32bit_values, vg_num_allowed_root_constants);
		if (offset_in_32bit_values >= vg_num_allowed_root_constants) return;
		num_32bit_values = vg_num_allowed_root_constants - offset_in_32bit_values;
	}
#if VG_VALIDATION
	if (cmd->CommandPool()->Queue() == VG_QUEUE_TRANSFER)
	{
		LOG(ERROR, "{}(): command list queue should be VG_QUEUE_GRAPHICS or VG_QUEUE_COMPUTE", _func_name_);
		return;
	}
	VALIDATE_ENUM(pipeline_type, "pipeline_type");
#endif
	cmd->SetRootConstants(pipeline_type, offset_in_32bit_values, num_32bit_values, data);
}

void vgCmdSetPipeline(VgCommandList cmd, VgPipeline pipeline)
{
	FUNC_DATA(vgCmdSetPipeline);
	CHECK_NOT_NULL(cmd);
	CHECK_NOT_NULL(pipeline);

#if VG_VALIDATION
	if (cmd->CommandPool()->Queue() == VG_QUEUE_TRANSFER)
	{
		LOG(ERROR, "{}(): command list queue is VG_QUEUE_TRANSFER -> pipelines are not supported", _func_name_);
		return;
	}
	if (cmd->CommandPool()->Queue() == VG_QUEUE_COMPUTE && pipeline->Type() == VG_PIPELINE_TYPE_GRAPHICS)
	{
		LOG(ERROR, "{}(): command list queue is VG_QUEUE_COMPUTE but pipeline type is VG_PIPELINE_TYPE_GRAPHICS", _func_name_);
		return;
	}
#endif

	cmd->SetPipeline(pipeline);
}

void vgCmdBarrier(VgCommandList cmd, const VgDependencyInfo* dependency_info)
{
	FUNC_DATA(vgCmdBarrier);
	CHECK_NOT_NULL(cmd);
	CHECK_NOT_NULL(dependency_info);

#if VG_VALIDATION
	for (uint32_t i = 0; i < dependency_info->num_memory_barriers; i++)
	{
		VALIDATE_FLAGS(dependency_info->memory_barriers[i].src_stage,
			"memory barrier {} src_stage", i);
		VALIDATE_FLAGS(dependency_info->memory_barriers[i].src_access,
			"memory barrier {} src_access", i);
		VALIDATE_FLAGS(dependency_info->memory_barriers[i].dst_stage,
			"memory barrier {} dst_stage", i);
		VALIDATE_FLAGS(dependency_info->memory_barriers[i].dst_access,
			"memory barrier {} dst_access", i);
	}
	for (uint32_t i = 0; i < dependency_info->num_buffer_barriers; i++)
	{
		VALIDATE_FLAGS(dependency_info->buffer_barriers[i].src_stage,
			"buffer barrier {} src_stage", i);
		VALIDATE_FLAGS(dependency_info->buffer_barriers[i].src_access,
			"buffer barrier {} src_access", i);
		VALIDATE_FLAGS(dependency_info->buffer_barriers[i].dst_stage,
			"buffer barrier {} dst_stage", i);
		VALIDATE_FLAGS(dependency_info->buffer_barriers[i].dst_access,
			"buffer barrier {} dst_access", i);
		if (dependency_info->buffer_barriers[i].buffer == nullptr)
		{
			LOG(ERROR, "{}(): buffer barrier {}: buffer = NULL", _func_name_, i);
			return;
		}
		switch (dependency_info->buffer_barriers[i].buffer->Desc().heap_type)
		{
		case VG_HEAP_TYPE_UPLOAD:
			VALIDATE_FLAGS_ALLOWED(VG_ACCESS_VERTEX_ATTRIBUTE_READ | VG_ACCESS_UNIFORM_READ
				| VG_ACCESS_INDEX_READ | VG_ACCESS_SHADER_READ | VG_ACCESS_SHADER_SAMPLED_READ
				| VG_ACCESS_SHADER_STORAGE_READ | VG_ACCESS_INDIRECT_COMMAND_READ
				| VG_ACCESS_TRANSFER_READ | VG_ACCESS_MEMORY_READ,
				dependency_info->buffer_barriers[i].dst_access, "buffer barrier {} dst access", i);
			break;
		case VG_HEAP_TYPE_READBACK:
			VALIDATE_FLAGS_ALLOWED(VG_ACCESS_TRANSFER_WRITE | VG_ACCESS_MEMORY_WRITE,
				dependency_info->buffer_barriers[i].dst_access, "buffer barrier {} dst access", i);
			break;
		}
	}
	for (uint32_t i = 0; i < dependency_info->num_texture_barriers; i++)
	{
		VALIDATE_FLAGS(dependency_info->texture_barriers[i].src_stage,
			"texture barrier {} src_stage", i);
		VALIDATE_FLAGS(dependency_info->texture_barriers[i].src_access,
			"texture barrier {} src_access", i);
		VALIDATE_FLAGS(dependency_info->texture_barriers[i].dst_stage,
			"texture barrier {} dst_stage", i);
		VALIDATE_FLAGS(dependency_info->texture_barriers[i].dst_access,
			"texture barrier {} dst_access", i);
		VALIDATE_ENUM(dependency_info->texture_barriers[i].old_layout,
			"texture barrier {} old_layout", i);
		VALIDATE_ENUM(dependency_info->texture_barriers[i].new_layout,
			"texture barrier {} new_layout", i);
		/*if (dependency_info->texture_barriers[i].old_queue != VG_QUEUE_IGNORE)
		{
			VALIDATE_FLAGS(dependency_info->texture_barriers[i].old_queue,
				"texture barrier {} old_queue", i);
		}
		if (dependency_info->texture_barriers[i].new_queue != VG_QUEUE_IGNORE)
		{
			VALIDATE_FLAGS(dependency_info->texture_barriers[i].new_queue,
				"texture barrier {} new_queue", i);
		}
		if (dependency_info->texture_barriers[i].old_queue != VG_QUEUE_IGNORE
			&& dependency_info->texture_barriers[i].new_queue != VG_QUEUE_IGNORE
			&& cmd->CommandPool()->Queue() != dependency_info->texture_barriers[i].old_queue
			&& cmd->CommandPool()->Queue() != dependency_info->texture_barriers[i].new_queue)
		{
			LOG(ERROR, "{}(): texture barrier {} has both queues specified, and cmd queue({}) does not equal any of them",
				_func_name_, i, magic_enum::enum_name(cmd->CommandPool()->Queue()));
			return;
		}*/
		if (dependency_info->texture_barriers[i].texture == nullptr)
		{
			LOG(ERROR, "{}(): texture barrier {}: texture = NULL", _func_name_, i);
			return;
		}
		switch (dependency_info->texture_barriers[i].texture->Desc().heap_type)
		{
		case VG_HEAP_TYPE_UPLOAD:
			VALIDATE_FLAGS_ALLOWED(VG_ACCESS_SHADER_READ | VG_ACCESS_SHADER_SAMPLED_READ
				| VG_ACCESS_SHADER_STORAGE_READ | VG_ACCESS_INDIRECT_COMMAND_READ | VG_ACCESS_TRANSFER_READ
				| VG_ACCESS_MEMORY_READ,
				dependency_info->texture_barriers[i].dst_access, "texture barrier {} dst access", i);
			break;
		case VG_HEAP_TYPE_READBACK:
			VALIDATE_FLAGS_ALLOWED(VG_ACCESS_TRANSFER_WRITE | VG_ACCESS_MEMORY_WRITE,
				dependency_info->buffer_barriers[i].dst_access, "texture barrier {} dst access", i);
			break;
		}
		switch (cmd->CommandPool()->Queue())
		{
		case VG_QUEUE_COMPUTE:
		{
			const auto allowed = { VG_TEXTURE_LAYOUT_GENERAL, VG_TEXTURE_LAYOUT_SHADER_RESOURCE,
				VG_TEXTURE_LAYOUT_TRANSFER_SOURCE, VG_TEXTURE_LAYOUT_TRANSFER_DEST,
				VG_TEXTURE_LAYOUT_READ_ONLY };
			VALIDATE_ENUM_ALLOWED(allowed, dependency_info->texture_barriers[i].old_layout,
				"texture barrier {} old layout on queue {}", i, magic_enum::enum_name(VG_QUEUE_COMPUTE));
			VALIDATE_ENUM_ALLOWED(allowed, dependency_info->texture_barriers[i].new_layout,
				"texture barrier {} new layout on queue {}", i, magic_enum::enum_name(VG_QUEUE_COMPUTE));
			break;
		}
		case VG_QUEUE_TRANSFER:
		{
			const auto allowed = { VG_TEXTURE_LAYOUT_GENERAL,
				VG_TEXTURE_LAYOUT_TRANSFER_SOURCE, VG_TEXTURE_LAYOUT_TRANSFER_DEST,
				VG_TEXTURE_LAYOUT_READ_ONLY };
			VALIDATE_ENUM_ALLOWED(allowed, dependency_info->texture_barriers[i].old_layout,
				"texture barrier {} old layout on queue {}", i, magic_enum::enum_name(VG_QUEUE_TRANSFER));
			VALIDATE_ENUM_ALLOWED(allowed, dependency_info->texture_barriers[i].new_layout,
				"texture barrier {} new layout on queue {}", i, magic_enum::enum_name(VG_QUEUE_TRANSFER));
			break;
		}
		}
	}
#endif

	cmd->Barrier(*dependency_info);
}

VgResult vgBufferGetApiObject(VgBuffer buffer, void** out_obj)
{
	FUNC_DATA(vgBufferGetApiObject);
	CHECK_NOT_NULL_RETURN(buffer);
	CHECK_NOT_NULL_RETURN(out_obj);

	*out_obj = buffer->GetApiObject();
	return VG_SUCCESS;
}

void vgBufferSetName(VgBuffer buffer, const char* name)
{
	FUNC_DATA(vgBufferSetName);
	CHECK_NOT_NULL(buffer);
	CHECK_NOT_NULL(name);

	buffer->SetName(name);
}

VgResult vgBufferGetDevice(VgBuffer buffer, VgDevice* out_device)
{
	FUNC_DATA(vgBufferGetDevice);
	CHECK_NOT_NULL_RETURN(buffer);
	CHECK_NOT_NULL_RETURN(out_device);

	*out_device = buffer->Device();
	return VG_SUCCESS;
}

VgResult vgBufferGetDesc(VgBuffer buffer, VgBufferDesc* out_desc)
{
	FUNC_DATA(vgBufferGetDesc);
	CHECK_NOT_NULL_RETURN(buffer);
	CHECK_NOT_NULL_RETURN(out_desc);

	*out_desc = buffer->Desc();
	return VG_SUCCESS;
}

VgResult vgBufferCreateView(VgBuffer buffer, const VgBufferViewDesc* desc, VgView* out_descriptor)
{
	FUNC_DATA(vgBufferCreateView);
	CHECK_NOT_NULL_RETURN(buffer);
	CHECK_NOT_NULL_RETURN(desc);
	CHECK_NOT_NULL_RETURN(out_descriptor);

	auto descCopy = *desc;
#if VG_VALIDATION
	VALIDATE_ENUM_RETURN(descCopy.descriptor_type, "descriptor_type");
	VALIDATE_ENUM_RETURN(descCopy.view_type, "view_type");
	if (descCopy.descriptor_type == VG_BUFFER_DESCRIPTOR_TYPE_UAV && buffer->Desc().heap_type != VG_HEAP_TYPE_GPU)
	{
		LOG(ERROR, "{}(): desc->descriptor_type({}) can only be set when buffer is on heap {}({}) (buffer heap: {}({}))", _func_name_,
			static_cast<int>(desc->descriptor_type), magic_enum::enum_name(VG_HEAP_TYPE_GPU), static_cast<int>(VG_HEAP_TYPE_GPU),
			magic_enum::enum_name(buffer->Desc().heap_type), static_cast<int>(buffer->Desc().heap_type));
		return VG_ILLEGAL_OPERATION;
	}
	if (descCopy.descriptor_type == VG_BUFFER_DESCRIPTOR_TYPE_CBV)
	{
		// size must be multiple of 256
	}
	// TODO: ...
#endif
	if (descCopy.size == VG_WHOLE_SIZE)
	{
		descCopy.size = buffer->Desc().size - desc->offset;
	}
	*out_descriptor = buffer->CreateView(descCopy);
	return VG_SUCCESS;
}

void vgBufferDestroyViews(VgBuffer buffer)
{
	FUNC_DATA(vgBufferDestroyViews);
	CHECK_NOT_NULL(buffer);

	buffer->DestroyViews();
}

VgResult vgBufferMap(VgBuffer buffer, void** out_data)
{
	FUNC_DATA(vgBufferMap);
	CHECK_NOT_NULL_RETURN(buffer);
	CHECK_NOT_NULL_RETURN(out_data);

	try
	{
		*out_data = buffer->Map();
	}
	catch (VgError& ex)
	{
		LOG(ERROR, "{}() failed: {}", _func_name_, ex.what());
		return ex.result;
	}
	return VG_SUCCESS;
}

void vgBufferUnmap(VgBuffer buffer)
{
	FUNC_DATA(vgBufferUnmap);
	CHECK_NOT_NULL(buffer);

	buffer->Unmap();
}

VgResult vgPipelineGetApiObject(VgPipeline pipeline, void** out_obj)
{
	FUNC_DATA(vgPipelineGetApiObject);
	CHECK_NOT_NULL_RETURN(pipeline);
	CHECK_NOT_NULL_RETURN(out_obj);

	*out_obj = pipeline->GetApiObject();
	return VG_SUCCESS;
}

void vgPipelineSetName(VgPipeline pipeline, const char* name)
{
	FUNC_DATA(vgPipelineSetName);
	CHECK_NOT_NULL(pipeline);
	CHECK_NOT_NULL(name);

	pipeline->SetName(name);
}

VgResult vgPipelineGetDevice(VgPipeline pipeline, VgDevice* out_device)
{
	FUNC_DATA(vgPipelineGetDevice);
	CHECK_NOT_NULL_RETURN(pipeline);
	CHECK_NOT_NULL_RETURN(out_device);

	*out_device = pipeline->Device();
	return VG_SUCCESS;
}

VgResult vgPipelineGetType(VgPipeline pipeline, VgPipelineType* out_type)
{
	FUNC_DATA(vgPipelineGetType);
	CHECK_NOT_NULL_RETURN(pipeline);
	CHECK_NOT_NULL_RETURN(out_type);
	
	*out_type = pipeline->Type();
	return VG_SUCCESS;
}

VgResult vgCmdBeginRendering(VgCommandList cmd, const VgRenderingInfo* info)
{
	FUNC_DATA(vgCmdBeginRendering);
	CHECK_NOT_NULL_RETURN(cmd);
	if (cmd->CommandPool()->Queue() != VG_QUEUE_GRAPHICS)
	{
		LOG(ERROR, "(){}: only allowed on {} but cmd is on queue {}", _func_name_,
			magic_enum::enum_name(VG_QUEUE_GRAPHICS), magic_enum::enum_name(cmd->CommandPool()->Queue()));
		return VG_ILLEGAL_OPERATION;
	}
	CHECK_NOT_NULL_RETURN(info);
	if (info->num_color_attachments > 0 && !info->color_attachments)
	{
		LOG(ERROR, "{}(): num_color_attachments({}) > 0, but color_attachments is NULL", _func_name_, info->num_color_attachments);
		return VG_BAD_ARGUMENT;
	}
	if (!info->color_attachments && info->depth_stencil_attachment.view == VG_NO_VIEW)
	{
		LOG(ERROR, "{}(): color_attachments is NULL and depth_stencil_attachment.view is VG_NO_VIEW", _func_name_);
		return VG_ILLEGAL_OPERATION;
	}

#if VG_VALIDATION
	for (uint32_t i = 0; i < info->num_color_attachments; i++)
	{
		if (info->color_attachments[i].view == VG_NO_VIEW)
		{
			LOG(ERROR, "{}(): color_attachments[{}].view is VG_NO_VIEW", _func_name_, i);
			return VG_BAD_ARGUMENT;
		}
		VALIDATE_ENUM_RETURN(info->color_attachments[i].view_layout, "color_attachments[{}].view_layout", i);
		if (info->color_attachments[i].resolve_view != VG_NO_VIEW)
		{
			VALIDATE_ENUM_RETURN(info->color_attachments[i].resolve_mode, "color_attachments[{}].resolve_mode", i);
			VALIDATE_ENUM_RETURN(info->color_attachments[i].resolve_view_layout, "color_attachments[{}].resolve_view_layout", i);
		}
		VALIDATE_ENUM_RETURN(info->color_attachments[i].load_op, "color_attachments[{}].load_op", i);
		VALIDATE_ENUM_RETURN(info->color_attachments[i].store_op, "color_attachments[{}].store_op", i);
	}
	if (info->depth_stencil_attachment.view != VG_NO_VIEW)
	{
		VALIDATE_ENUM_RETURN(info->depth_stencil_attachment.view_layout, "depth_stencil_attachment.view_layout");
		if (info->depth_stencil_attachment.resolve_view != VG_NO_VIEW)
		{
			VALIDATE_ENUM_RETURN(info->depth_stencil_attachment.resolve_mode, "depth_stencil_attachment.resolve_mode");
			VALIDATE_ENUM_RETURN(info->depth_stencil_attachment.resolve_view_layout, "depth_stencil_attachment.resolve_view_layout");
		}
		VALIDATE_ENUM_RETURN(info->depth_stencil_attachment.load_op, "depth_stencil_attachment.load_op");
		VALIDATE_ENUM_RETURN(info->depth_stencil_attachment.store_op, "depth_stencil_attachment.store_op");
	}
#endif

	cmd->BeginRendering(*info);
	return VG_SUCCESS;
}

void vgCmdEndRendering(VgCommandList cmd)
{
	FUNC_DATA(vgCmdEndRendering);
	CHECK_NOT_NULL(cmd);
	if (cmd->CommandPool()->Queue() != VG_QUEUE_GRAPHICS)
	{
		LOG(ERROR, "(){}: only allowed on {} but cmd is on queue {}", _func_name_,
			magic_enum::enum_name(VG_QUEUE_GRAPHICS), magic_enum::enum_name(cmd->CommandPool()->Queue()));
		return;
	}
	cmd->EndRendering();
}

void vgCmdSetViewport(VgCommandList cmd, uint32_t first_viewport, uint32_t num_viewports, VgViewport* viewports)
{
	FUNC_DATA(vgCmdSetViewport);
	CHECK_NOT_NULL(cmd);
	if (cmd->CommandPool()->Queue() != VG_QUEUE_GRAPHICS)
	{
		LOG(ERROR, "(){}: only allowed on {} but cmd is on queue {}", _func_name_,
			magic_enum::enum_name(VG_QUEUE_GRAPHICS), magic_enum::enum_name(cmd->CommandPool()->Queue()));
		return;
	}
	if (num_viewports == 0) return;
	CHECK_NOT_NULL(viewports);

	if (first_viewport >= vg_num_max_viewports_and_scissors)
	{
		LOG(ERROR, "{}(): first_viewport({}) should be < vg_num_max_viewports_and_scissors({})",
			_func_name_, first_viewport, vg_num_max_viewports_and_scissors);
		return;
	}
	if (first_viewport + num_viewports > vg_num_max_viewports_and_scissors)
	{
		LOG(WARN, "{}(): first_viewport({}) + num_viewports({}) should be <= vg_num_max_viewports_and_scissors({})",
			_func_name_, first_viewport, num_viewports, vg_num_max_viewports_and_scissors);
		num_viewports = vg_num_max_viewports_and_scissors - first_viewport;
	}

	cmd->SetViewport(first_viewport, num_viewports, viewports);
}

void vgCmdSetScissor(VgCommandList cmd, uint32_t first_scissor, uint32_t num_scissors, VgScissor* scissors)
{
	FUNC_DATA(vgCmdSetScissor);
	CHECK_NOT_NULL(cmd);
	if (cmd->CommandPool()->Queue() != VG_QUEUE_GRAPHICS)
	{
		LOG(ERROR, "(){}: only allowed on {} but cmd is on queue {}", _func_name_,
			magic_enum::enum_name(VG_QUEUE_GRAPHICS), magic_enum::enum_name(cmd->CommandPool()->Queue()));
		return;
	}
	if (num_scissors == 0) return;
	CHECK_NOT_NULL(scissors);

	if (first_scissor >= vg_num_max_viewports_and_scissors)
	{
		LOG(ERROR, "{}(): first_scissor({}) should be < vg_num_max_viewports_and_scissors({})",
			_func_name_, first_scissor, vg_num_max_viewports_and_scissors);
		return;
	}
	if (first_scissor + num_scissors > vg_num_max_viewports_and_scissors)
	{
		LOG(WARN, "{}(): first_scissor({}) + num_viewports({}) should be <= vg_num_max_viewports_and_scissors({})",
			_func_name_, first_scissor, num_scissors, vg_num_max_viewports_and_scissors);
		num_scissors = vg_num_max_viewports_and_scissors - first_scissor;
	}

	cmd->SetScissor(first_scissor, num_scissors, scissors);
}

void vgCmdDraw(VgCommandList cmd, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
	FUNC_DATA(vgCmdDraw);
	CHECK_NOT_NULL(cmd);
	if (cmd->CommandPool()->Queue() != VG_QUEUE_GRAPHICS)
	{
		LOG(ERROR, "(){}: only allowed on {} but cmd is on queue {}", _func_name_,
			magic_enum::enum_name(VG_QUEUE_GRAPHICS), magic_enum::enum_name(cmd->CommandPool()->Queue()));
		return;
	}

#if VG_VALIDATION
	if (!(cmd->GetState() & VgCommandList_t::STATE_RENDERING))
	{
		LOG(ERROR, "{}(): command list should be in state of rendering", _func_name_);
		return;
	}
	if (vertex_count == 0)
	{
		LOG(WARN, "{}(): called with vertex_count = 0", _func_name_);
		return;
	}
	if (instance_count == 0)
	{
		LOG(WARN, "{}(): called with instance_count = 0", _func_name_);
		return;
	}
#endif

	cmd->Draw(vertex_count, instance_count, first_vertex, first_instance);
}

void vgCmdDrawIndexed(VgCommandList cmd, uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance)
{
	FUNC_DATA(vgCmdDraw);
	CHECK_NOT_NULL(cmd);
	if (cmd->CommandPool()->Queue() != VG_QUEUE_GRAPHICS)
	{
		LOG(ERROR, "(){}: only allowed on {} but cmd is on queue {}", _func_name_,
			magic_enum::enum_name(VG_QUEUE_GRAPHICS), magic_enum::enum_name(cmd->CommandPool()->Queue()));
		return;
	}

#if VG_VALIDATION
	if (!(cmd->GetState() & VgCommandList_t::STATE_RENDERING))
	{
		LOG(ERROR, "{}(): command list should be in state of rendering", _func_name_);
		return;
	}
	if (index_count == 0)
	{
		LOG(WARN, "{}(): called with index_count = 0", _func_name_);
		return;
	}
	if (instance_count == 0)
	{
		LOG(WARN, "{}(): called with instance_count = 0", _func_name_);
		return;
	}
#endif

	cmd->DrawIndexed(index_count, instance_count, first_index, vertex_offset, first_instance);
}

void vgCmdDispatch(VgCommandList cmd, uint32_t groups_x, uint32_t groups_y, uint32_t groups_z)
{
	FUNC_DATA(vgCmdDispatch);
	CHECK_NOT_NULL(cmd);
	if (cmd->CommandPool()->Queue() == VG_QUEUE_TRANSFER)
	{
		LOG(ERROR, "(){}: not allowed on {}", _func_name_,
			magic_enum::enum_name(VG_QUEUE_TRANSFER));
		return;
	}
	if (groups_x == 0)
	{
		LOG(WARN, "{}(): groups_x = 0", _func_name_);
		return;
	}
	if (groups_y == 0)
	{
		LOG(WARN, "{}(): groups_y = 0", _func_name_);
		return;
	}
	if (groups_z == 0)
	{
		LOG(WARN, "{}(): groups_z = 0", _func_name_);
		return;
	}
	cmd->Dispatch(groups_x, groups_y, groups_z);
}

void vgCmdDrawIndirect(VgCommandList cmd, VgBuffer buffer, uint64_t offset, uint32_t draw_count, uint32_t stride)
{
	FUNC_DATA(vgCmdDrawIndirect);
	CHECK_NOT_NULL(cmd);
	CHECK_NOT_NULL(buffer);
	if (cmd->CommandPool()->Queue() != VG_QUEUE_GRAPHICS)
	{
		LOG(ERROR, "(){}: only allowed on {} but cmd is on queue {}", _func_name_,
			magic_enum::enum_name(VG_QUEUE_GRAPHICS), magic_enum::enum_name(cmd->CommandPool()->Queue()));
		return;
	}
	if (stride < sizeof(VgDrawIndirectCommand))
	{
		LOG(ERROR, "{}(): stride({}) should be >= sizeof(VgDrawIndirectCommand) ({})", _func_name_, stride, sizeof(VgDrawIndirectCommand));
		return;
	}
#if VG_VALIDATION
	if (!(cmd->GetState() & VgCommandList_t::STATE_RENDERING))
	{
		LOG(ERROR, "{}(): command list should be in state of rendering", _func_name_);
		return;
	}
	VALIDATE_ENUM_ALLOWED(vg::UnorderedSet{ VG_BUFFER_USAGE_GENERAL }, buffer->Desc().usage, "buffer usage");
	if (offset + draw_count * stride > buffer->Desc().size)
	{
		LOG(ERROR, "{}(): offset({}) + draw_count({}) * stride({}) ({}) should be <= buffer size {}", _func_name_, offset,
			draw_count, stride, offset + draw_count * stride, buffer->Desc().size);
		return;
	}
#endif
	cmd->DrawIndirect(buffer, offset, draw_count, stride);
}

void vgCmdDrawIndirectCount(VgCommandList cmd, VgBuffer buffer, uint64_t offset, VgBuffer count_buffer, uint64_t count_buffer_offset, uint32_t max_draw_count, uint32_t stride)
{
	FUNC_DATA(vgCmdDrawIndirectCount);
	CHECK_NOT_NULL(cmd);
	CHECK_NOT_NULL(buffer);
	CHECK_NOT_NULL(count_buffer);
	if (cmd->CommandPool()->Queue() != VG_QUEUE_GRAPHICS)
	{
		LOG(ERROR, "(){}: only allowed on {} but cmd is on queue {}", _func_name_,
			magic_enum::enum_name(VG_QUEUE_GRAPHICS), magic_enum::enum_name(cmd->CommandPool()->Queue()));
		return;
	}
	if (stride < sizeof(VgDrawIndirectCommand))
	{
		LOG(ERROR, "{}(): stride({}) should be >= sizeof(VgDrawIndirectCommand) ({})", _func_name_, stride, sizeof(VgDrawIndirectCommand));
		return;
	}
#if VG_VALIDATION
	if (!(cmd->GetState() & VgCommandList_t::STATE_RENDERING))
	{
		LOG(ERROR, "{}(): command list should be in state of rendering", _func_name_);
		return;
	}
	VALIDATE_ENUM_ALLOWED(vg::UnorderedSet{ VG_BUFFER_USAGE_GENERAL }, buffer->Desc().usage, "buffer usage");
	VALIDATE_ENUM_ALLOWED(vg::UnorderedSet{ VG_BUFFER_USAGE_GENERAL }, count_buffer->Desc().usage, "count buffer usage");
	if (offset + max_draw_count * stride > buffer->Desc().size)
	{
		LOG(ERROR, "{}(): offset({}) + max_draw_count({}) * stride({}) ({}) should be <= buffer size {}", _func_name_, offset,
			max_draw_count, stride, offset + max_draw_count * stride, buffer->Desc().size);
		return;
	}
#endif
	cmd->DrawIndirectCount(buffer, offset, count_buffer, count_buffer_offset, max_draw_count, stride);
}

void vgCmdDrawIndexedIndirect(VgCommandList cmd, VgBuffer buffer, uint64_t offset, uint32_t draw_count, uint32_t stride)
{
	FUNC_DATA(vgCmdDrawIndexedIndirect);
	CHECK_NOT_NULL(cmd);
	CHECK_NOT_NULL(buffer);
	if (cmd->CommandPool()->Queue() != VG_QUEUE_GRAPHICS)
	{
		LOG(ERROR, "(){}: only allowed on {} but cmd is on queue {}", _func_name_,
			magic_enum::enum_name(VG_QUEUE_GRAPHICS), magic_enum::enum_name(cmd->CommandPool()->Queue()));
		return;
	}
	if (stride < sizeof(VgDrawIndexedIndirectCommand))
	{
		LOG(ERROR, "{}(): stride({}) should be >= sizeof(VgDrawIndexedIndirectCommand) ({})", _func_name_, stride, sizeof(VgDrawIndexedIndirectCommand));
		return;
	}
#if VG_VALIDATION
	if (!(cmd->GetState() & VgCommandList_t::STATE_RENDERING))
	{
		LOG(ERROR, "{}(): command list should be in state of rendering", _func_name_);
		return;
	}
	VALIDATE_ENUM_ALLOWED(vg::UnorderedSet{ VG_BUFFER_USAGE_GENERAL }, buffer->Desc().usage, "buffer usage");
	if (offset + draw_count * stride > buffer->Desc().size)
	{
		LOG(ERROR, "{}(): offset({}) + draw_count({}) * stride({}) ({}) should be <= buffer size {}", _func_name_, offset,
			draw_count, stride, offset + draw_count * stride, buffer->Desc().size);
		return;
	}
#endif
	cmd->DrawIndexedIndirect(buffer, offset, draw_count, stride);
}

void vgCmdDrawIndexedIndirectCount(VgCommandList cmd, VgBuffer buffer, uint64_t offset, VgBuffer count_buffer, uint64_t count_buffer_offset, uint32_t max_draw_count, uint32_t stride)
{
	FUNC_DATA(vgCmdDrawIndexedIndirectCount);
	CHECK_NOT_NULL(cmd);
	CHECK_NOT_NULL(buffer);
	CHECK_NOT_NULL(count_buffer);
	if (cmd->CommandPool()->Queue() != VG_QUEUE_GRAPHICS)
	{
		LOG(ERROR, "(){}: only allowed on {} but cmd is on queue {}", _func_name_,
			magic_enum::enum_name(VG_QUEUE_GRAPHICS), magic_enum::enum_name(cmd->CommandPool()->Queue()));
		return;
	}
	if (stride < sizeof(VgDrawIndexedIndirectCommand))
	{
		LOG(ERROR, "{}(): stride({}) should be >= sizeof(VgDrawIndexedIndirectCommand) ({})", _func_name_, stride, sizeof(VgDrawIndexedIndirectCommand));
		return;
	}
#if VG_VALIDATION
	if (!(cmd->GetState() & VgCommandList_t::STATE_RENDERING))
	{
		LOG(ERROR, "{}(): command list should be in state of rendering", _func_name_);
		return;
	}
	VALIDATE_ENUM_ALLOWED(vg::UnorderedSet{ VG_BUFFER_USAGE_GENERAL }, buffer->Desc().usage, "buffer usage");
	VALIDATE_ENUM_ALLOWED(vg::UnorderedSet{ VG_BUFFER_USAGE_GENERAL }, count_buffer->Desc().usage, "count buffer usage");
	if (offset + max_draw_count * stride > buffer->Desc().size)
	{
		LOG(ERROR, "{}(): offset({}) + max_draw_count({}) * stride({}) ({}) should be <= buffer size {}", _func_name_, offset,
			max_draw_count, stride, offset + max_draw_count * stride, buffer->Desc().size);
		return;
	}
#endif
	cmd->DrawIndexedIndirectCount(buffer, offset, count_buffer, count_buffer_offset, max_draw_count, stride);
}

void vgCmdDispatchIndirect(VgCommandList cmd, VgBuffer buffer, uint64_t offset)
{
	FUNC_DATA(vgCmdDrawIndexedIndirectCount);
	CHECK_NOT_NULL(cmd);
	CHECK_NOT_NULL(buffer);
	if (cmd->CommandPool()->Queue() == VG_QUEUE_TRANSFER)
	{
		LOG(ERROR, "(){}: not allowed on {}", _func_name_,
			magic_enum::enum_name(VG_QUEUE_TRANSFER));
		return;
	}
#if VG_VALIDATION
	VALIDATE_ENUM_ALLOWED(vg::UnorderedSet{ VG_BUFFER_USAGE_GENERAL }, buffer->Desc().usage, "buffer usage");
	if (offset + sizeof(VgDispatchIndirectCommand) > buffer->Desc().size)
	{
		LOG(ERROR, "{}(): offset({}) + sizeof(VgDispatchIndirectCommand) ({}) should be <= buffer size {}", _func_name_, offset,
			offset + sizeof(VgDispatchIndirectCommand), buffer->Desc().size);
		return;
	}
#endif
	cmd->DispatchIndirect(buffer, offset);
}

// Do not allow src to be on READBACK heap
void vgCmdCopyBufferToBuffer(VgCommandList cmd, VgBuffer dst, uint64_t dst_offset, VgBuffer src, uint64_t src_offset, uint64_t size)
{
	FUNC_DATA(vgCmdCopyBufferToBuffer);
	CHECK_NOT_NULL(cmd);
	CHECK_NOT_NULL(dst);
	CHECK_NOT_NULL(src);
#if VG_VALIDATION
	// ...
#endif
	if (size == VG_WHOLE_SIZE)
	{
		size = src->Desc().size;
	}
	cmd->CopyBufferToBuffer(dst, dst_offset, src, src_offset, size);

}

void vgCmdCopyBufferToTexture(VgCommandList cmd, VgTexture dst, const VgRegion* dst_region, VgBuffer src, uint64_t src_offset)
{
	FUNC_DATA(vgCmdCopyBufferToTexture);
	CHECK_NOT_NULL(cmd);
	CHECK_NOT_NULL(dst);
	CHECK_NOT_NULL(dst_region);
	CHECK_NOT_NULL(src);
#if VG_VALIDATION
	// ...
#endif
	cmd->CopyBufferToTexture(dst, *dst_region, src, src_offset);
}

void vgCmdCopyTextureToBuffer(VgCommandList cmd, VgBuffer dst, uint64_t dst_offset, VgTexture src, const VgRegion* src_region)
{
	FUNC_DATA(vgCmdCopyTextureToBuffer);
	CHECK_NOT_NULL(cmd);
	CHECK_NOT_NULL(dst);
	CHECK_NOT_NULL(src);
	CHECK_NOT_NULL(src_region);
#if VG_VALIDATION
	// ...
#endif
	cmd->CopyTextureToBuffer(dst, dst_offset, src, *src_region);
}

void vgCmdCopyTextureToTexture(VgCommandList cmd, VgTexture dst, const VgRegion* dst_region, VgTexture src, const VgRegion* src_region)
{
	FUNC_DATA(vgCmdCopyTextureToBuffer);
	CHECK_NOT_NULL(cmd);
	CHECK_NOT_NULL(dst);
	CHECK_NOT_NULL(dst_region);
	CHECK_NOT_NULL(src);
	CHECK_NOT_NULL(src_region);
#if VG_VALIDATION
	// ...
#endif
	cmd->CopyTextureToTexture(dst, *dst_region, src, *src_region);
}

void vgCmdBeginMarker(VgCommandList cmd, const char* name, float color[3])
{
	FUNC_DATA(vgCmdBeginMarker);
	CHECK_NOT_NULL(cmd);
	CHECK_NOT_NULL(name);
	
	static float defaultColor[] = { 1.0f, 1.0f, 1.0f };
	cmd->BeginMarker(name, color ? color : defaultColor);
}

void vgCmdEndMarker(VgCommandList cmd)
{
	FUNC_DATA(vgCmdEndMarker);
	CHECK_NOT_NULL(cmd);
	cmd->EndMarker();
}

VgResult vgDeviceGetSamplerIndex(VgDevice device, VgSampler sampler, uint32_t* out_index)
{
	FUNC_DATA(vgDeviceGetSamplerIndex);
	CHECK_NOT_NULL_RETURN(device);
	CHECK_NOT_NULL_RETURN(sampler);
	CHECK_NOT_NULL_RETURN(out_index);

	*out_index = device->GetSamplerIndex(sampler);
	return VG_SUCCESS;
}

VgResult vgTextureGetApiObject(VgTexture texture, void** out_obj)
{
	FUNC_DATA(vgTextureGetApiObject);
	CHECK_NOT_NULL_RETURN(texture);
	CHECK_NOT_NULL_RETURN(out_obj);

	*out_obj = texture->GetApiObject();
	return VG_SUCCESS;
}

void vgTextureSetName(VgTexture texture, const char* name)
{
	FUNC_DATA(vgTextureSetName);
	CHECK_NOT_NULL(texture);
	CHECK_NOT_NULL(name);

	texture->SetName(name);
}

VgResult vgTextureGetDevice(VgTexture texture, VgDevice* out_device)
{
	FUNC_DATA(vgTextureGetDevice);
	CHECK_NOT_NULL_RETURN(texture);
	CHECK_NOT_NULL_RETURN(out_device);

	*out_device = texture->Device();
	return VG_SUCCESS;
}

VgResult vgTextureGetDesc(VgTexture texture, VgTextureDesc* out_desc)
{
	FUNC_DATA(vgTextureGetDevice);
	CHECK_NOT_NULL_RETURN(texture);
	CHECK_NOT_NULL_RETURN(out_desc);

	*out_desc = texture->Desc();
	return VG_SUCCESS;
}

VgResult vgTextureCreateAttachmentView(VgTexture texture, const VgAttachmentViewDesc* desc, VgAttachmentView* out_descriptor)
{
	FUNC_DATA(vgTextureCreateAttachmentView);
	CHECK_NOT_NULL_RETURN(texture);
	CHECK_NOT_NULL_RETURN(desc);
	CHECK_NOT_NULL_RETURN(out_descriptor);

#if VG_VALIDATION
	VALIDATE_ENUM_RETURN(desc->format, "format");
	VALIDATE_ENUM_RETURN(desc->type, "type");

	const vg::UnorderedSet arrayViews = { VG_TEXTURE_ATTACHMENT_VIEW_TYPE_1D_ARRAY,
		VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_ARRAY, VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_MS_ARRAY };
	if (arrayViews.contains(desc->type))
	{
		LOG(ERROR, "{}(): array_layers({}) should be > 0 for {}", _func_name_,
			desc->base_array_layer, magic_enum::enum_name(desc->type));
		return VG_BAD_ARGUMENT;
	}

	const auto& tdesc = texture->Desc();
	if (tdesc.type == VG_TEXTURE_TYPE_1D)
	{
		const vg::UnorderedSet allowed_views = { VG_TEXTURE_ATTACHMENT_VIEW_TYPE_1D,
			VG_TEXTURE_ATTACHMENT_VIEW_TYPE_1D_ARRAY };
		if (!allowed_views.contains(desc->type))
		{
			LOG(ERROR, "{}(): type({}) is not one of allowed views: {}", _func_name_, magic_enum::enum_name(desc->type),
				join(allowed_views));
			return VG_BAD_ARGUMENT;
		}
		if (desc->base_array_layer + desc->array_layers > tdesc.depth_or_array_layers)
		{
			LOG(ERROR, "{}(): base_array_layer({}) + array_layers({}) should be <= {}", _func_name_,
				desc->base_array_layer, desc->array_layers, tdesc.depth_or_array_layers);
			return VG_BAD_ARGUMENT;
		}
	}
	else if (tdesc.type == VG_TEXTURE_TYPE_2D)
	{
		if (tdesc.sample_count > VG_SAMPLE_COUNT_1)
		{
			const vg::UnorderedSet allowed_multisampled_views = { VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_MS,
				VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_MS_ARRAY };
			if (!allowed_multisampled_views.contains(desc->type))
			{
				LOG(ERROR, "{}(): type({}) is not one of allowed views: {}", _func_name_, magic_enum::enum_name(desc->type),
					join(allowed_multisampled_views));
				return VG_BAD_ARGUMENT;
			}
		}
		else
		{
			const vg::UnorderedSet allowed_views = { VG_TEXTURE_ATTACHMENT_VIEW_TYPE_1D_ARRAY,
				VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D, VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_ARRAY,
				VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_MS, VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_MS_ARRAY };
			if (!allowed_views.contains(desc->type))
			{
				LOG(ERROR, "{}(): type({}) is not one of allowed views: {}", _func_name_, magic_enum::enum_name(desc->type),
					join(allowed_views));
				return VG_BAD_ARGUMENT;
			}
		}
	}
	else if (tdesc.type == VG_TEXTURE_TYPE_3D)
	{
		const vg::UnorderedSet allowed_views = { VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_ARRAY,
			VG_TEXTURE_ATTACHMENT_VIEW_TYPE_3D };
		if (!allowed_views.contains(desc->type))
		{
			LOG(ERROR, "{}(): type({}) is not one of allowed views: {}", _func_name_, magic_enum::enum_name(desc->type),
				join(allowed_views));
			return VG_BAD_ARGUMENT;
		}
		if (FormatIsDepthStencil(desc->format))
		{
			LOG(ERROR, "{}(): unable to create attachment view of a {} texture with format {}", _func_name_,
				magic_enum::enum_name(VG_TEXTURE_TYPE_3D), magic_enum::enum_name(desc->format));
			return VG_ILLEGAL_OPERATION;
		}
		if (desc->type == VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_ARRAY && desc->base_array_layer + desc->array_layers > tdesc.depth_or_array_layers)
		{
			LOG(ERROR, "{}(): base_array_layer({}) + array_layers({}) should be <= {}", _func_name_,
				desc->base_array_layer, desc->array_layers, tdesc.depth_or_array_layers);
			return VG_BAD_ARGUMENT;
		}
		else if (desc->type == VG_TEXTURE_ATTACHMENT_VIEW_TYPE_3D && desc->base_array_layer != 0)
		{
			LOG(ERROR, "{}(): base_array_layer({}) should be 0 for {}", _func_name_,
				desc->base_array_layer, magic_enum::enum_name(desc->type));
			return VG_BAD_ARGUMENT;
		}
	}

	const vg::UnorderedSet array_types = { VG_TEXTURE_ATTACHMENT_VIEW_TYPE_1D_ARRAY,
		VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_ARRAY, VG_TEXTURE_ATTACHMENT_VIEW_TYPE_3D };
	if (array_types.contains(desc->type) && desc->array_layers == 0)
	{
		LOG(ERROR, "{}(): type({}) should have array_layers set > 0", _func_name_, magic_enum::enum_name(desc->type));
		return VG_BAD_ARGUMENT;
	}
#endif

	*out_descriptor = texture->CreateAttachmentView(*desc);
	return VG_SUCCESS;
}

VgResult vgTextureCreateView(VgTexture texture, const VgTextureViewDesc* desc, VgView* out_descriptor)
{
	FUNC_DATA(vgTextureCreateView);
	CHECK_NOT_NULL_RETURN(texture);
	CHECK_NOT_NULL_RETURN(desc);
	CHECK_NOT_NULL_RETURN(out_descriptor);

#if VG_VALIDATION
	VALIDATE_ENUM_RETURN(desc->format, "format");
	VALIDATE_ENUM_RETURN(desc->type, "type");
	VALIDATE_ENUM_RETURN(desc->descriptor_type, "descriptor_type");
	VALIDATE_ENUM_RETURN(desc->components.r, "components.r");
	VALIDATE_ENUM_RETURN(desc->components.g, "components.g");
	VALIDATE_ENUM_RETURN(desc->components.b, "components.b");
	VALIDATE_ENUM_RETURN(desc->components.a, "components.a");

	if (desc->descriptor_type == VG_TEXTURE_DESCRIPTOR_TYPE_UAV && desc->array_layers != 1)
	{
		LOG(WARN, "{}(): array_layers will be forced to 1 when descriptor_type is {}", _func_name_,
			magic_enum::enum_name(desc->descriptor_type));
	}
	const vg::UnorderedSet cubeViews = { VG_TEXTURE_VIEW_TYPE_CUBE, VG_TEXTURE_VIEW_TYPE_CUBE_ARRAY };
	if (cubeViews.contains(desc->type) && desc->descriptor_type == VG_TEXTURE_DESCRIPTOR_TYPE_UAV)
	{
		LOG(ERROR, "{}(): {} is not supported for {}", _func_name_, magic_enum::enum_name(desc->descriptor_type),
			join(cubeViews));
		return VG_BAD_ARGUMENT;
	}

	const vg::UnorderedSet arrayViews = { VG_TEXTURE_VIEW_TYPE_1D_ARRAY,
		VG_TEXTURE_VIEW_TYPE_2D_ARRAY, VG_TEXTURE_VIEW_TYPE_2D_MS_ARRAY,
		VG_TEXTURE_VIEW_TYPE_CUBE_ARRAY };
	if (arrayViews.contains(desc->type))
	{
		LOG(ERROR, "{}(): array_layers({}) should be > 0 for {}", _func_name_,
			desc->base_array_layer, magic_enum::enum_name(desc->type));
		return VG_BAD_ARGUMENT;
	}

	const auto& tdesc = texture->Desc();
	if (tdesc.type == VG_TEXTURE_TYPE_1D)
	{
		const vg::UnorderedSet allowed_views = { VG_TEXTURE_VIEW_TYPE_1D,
			VG_TEXTURE_VIEW_TYPE_1D_ARRAY };
		if (!allowed_views.contains(desc->type))
		{
			LOG(ERROR, "{}(): type({}) is not one of allowed views: {}", _func_name_, magic_enum::enum_name(desc->type),
				join(allowed_views));
			return VG_BAD_ARGUMENT;
		}
		if (desc->base_array_layer + desc->array_layers > tdesc.depth_or_array_layers)
		{
			LOG(ERROR, "{}(): base_array_layer({}) + array_layers({}) should be <= {}", _func_name_,
				desc->base_array_layer, desc->array_layers, tdesc.depth_or_array_layers);
			return VG_BAD_ARGUMENT;
		}
	}
	else if (tdesc.type == VG_TEXTURE_TYPE_2D)
	{
		if (tdesc.sample_count > VG_SAMPLE_COUNT_1)
		{
			const vg::UnorderedSet allowed_multisampled_views = { VG_TEXTURE_VIEW_TYPE_2D_MS,
				VG_TEXTURE_VIEW_TYPE_2D_MS_ARRAY };
			if (!allowed_multisampled_views.contains(desc->type))
			{
				LOG(ERROR, "{}(): type({}) is not one of allowed views: {}", _func_name_, magic_enum::enum_name(desc->type),
					join(allowed_multisampled_views));
				return VG_BAD_ARGUMENT;
			}
		}
		else
		{
			const vg::UnorderedSet allowed_views = { VG_TEXTURE_VIEW_TYPE_1D_ARRAY,
				VG_TEXTURE_VIEW_TYPE_2D, VG_TEXTURE_VIEW_TYPE_2D_ARRAY,
				VG_TEXTURE_VIEW_TYPE_CUBE, VG_TEXTURE_VIEW_TYPE_CUBE_ARRAY };
			if (!allowed_views.contains(desc->type))
			{
				LOG(ERROR, "{}(): type({}) is not one of allowed views: {}", _func_name_, magic_enum::enum_name(desc->type),
					join(allowed_views));
				return VG_BAD_ARGUMENT;
			}

			if ((desc->type == VG_TEXTURE_VIEW_TYPE_CUBE || desc->type == VG_TEXTURE_VIEW_TYPE_CUBE_ARRAY))
			{
				if ((desc->array_layers % 6) != 0)
				{
					LOG(ERROR, "{}(): array_layers({}) should be a multiple of 6 for {} view", _func_name_,
						desc->array_layers, magic_enum::enum_name(desc->type));
					return VG_BAD_ARGUMENT;
				}
				if (desc->base_array_layer + desc->array_layers > tdesc.depth_or_array_layers)
				{
					LOG(ERROR, "{}(): desc->base_array_layer({}) + desc->array_layers({}) should be"
						"<= depth_or_array_layers({}) of specified texture for {} view",
						_func_name_, desc->base_array_layer, desc->array_layers, tdesc.depth_or_array_layers,
						magic_enum::enum_name(desc->type));
					return VG_BAD_ARGUMENT;
				}
			}
		}
		if (desc->base_array_layer + desc->array_layers > tdesc.depth_or_array_layers)
		{
			LOG(ERROR, "{}(): base_array_layer({}) + array_layers({}) should be <= {}", _func_name_,
				desc->base_array_layer, desc->array_layers, tdesc.depth_or_array_layers);
			return VG_BAD_ARGUMENT;
		}
	}
	else if (tdesc.type == VG_TEXTURE_TYPE_3D)
	{
		const vg::UnorderedSet allowed_views = { VG_TEXTURE_VIEW_TYPE_2D_ARRAY,
			VG_TEXTURE_VIEW_TYPE_3D };
		if (!allowed_views.contains(desc->type))
		{
			LOG(ERROR, "{}(): type({}) is not one of allowed views: {}", _func_name_, magic_enum::enum_name(desc->type),
				join(allowed_views));
			return VG_BAD_ARGUMENT;
		}
		if (desc->type == VG_TEXTURE_VIEW_TYPE_2D_ARRAY && desc->base_array_layer + desc->array_layers > tdesc.depth_or_array_layers)
		{
			LOG(ERROR, "{}(): base_array_layer({}) + array_layers({}) should be <= {}", _func_name_,
				desc->base_array_layer, desc->array_layers, tdesc.depth_or_array_layers);
			return VG_BAD_ARGUMENT;
		}
		else if (desc->type == VG_TEXTURE_VIEW_TYPE_3D && desc->base_array_layer != 0)
		{
			LOG(ERROR, "{}(): base_array_layer({}) should be 0 for {}", _func_name_,
				desc->base_array_layer, magic_enum::enum_name(desc->type));
			return VG_BAD_ARGUMENT;
		}
	}
#endif

	*out_descriptor = texture->CreateView(*desc);
	return VG_SUCCESS;
}

void vgTextureDestroyViews(VgTexture texture)
{
	FUNC_DATA(vgTextureDestroyViews);
	CHECK_NOT_NULL(texture);

	texture->DestroyViews();
}

VgResult vgGetVulkanObjects(VgVulkanObjects* out_vulkan_objects)
{
	FUNC_DATA(vgGetVulkanObjects);
	CHECK_NOT_NULL_RETURN(out_vulkan_objects);

#if VG_VULKAN_SUPPORTED
	if (!s_global->vulkanCore) return VG_FAILURE;
	s_global->vulkanCore->Initialize();
	*out_vulkan_objects = s_global->vulkanCore->GetVulkanObjects();
	return VG_SUCCESS;
#else
	return VG_API_UNSUPPORTED;
#endif
}

VgResult vgCreateSurfaceD3D12(void* hwnd, VgSurface* out_surface)
{
	FUNC_DATA(vgCreateSurfaceD3D12);
	CHECK_NOT_NULL_RETURN(hwnd);
	CHECK_NOT_NULL_RETURN(out_surface);
#if VG_D3D12_SUPPORTED	
	*out_surface = hwnd;
	return VG_SUCCESS;
#else
	return VG_API_UNSUPPORTED;
#endif
}

VgResult vgCreateSurfaceVulkan(struct VkSurfaceKHR_T* vk_surface, VgSurface* out_surface)
{
	FUNC_DATA(vgCreateSurfaceVulkan);
	CHECK_NOT_NULL_RETURN(vk_surface);
	CHECK_NOT_NULL_RETURN(out_surface);

#if VG_VULKAN_SUPPORTED
	if (!s_global->vulkanCore) return VG_API_UNSUPPORTED;
	*out_surface = vk_surface;
	return VG_SUCCESS;
#else
	return VG_API_UNSUPPORTED;
#endif
}

void vgDestroySurfaceVulkan(VgSurface surface)
{
	FUNC_DATA(vgDestroySurfaceVulkan);
	CHECK_NOT_NULL(surface);

#if VG_VULKAN_SUPPORTED
	if (s_global->vulkanCore)
		s_global->vulkanCore->DestroySurface(static_cast<VkSurfaceKHR>(surface));
#endif
}

VgResult vgSwapChainGetApiObject(VgSwapChain swap_chain, void** out_obj)
{
	FUNC_DATA(vgSwapChainGetApiObject);
	CHECK_NOT_NULL_RETURN(swap_chain);
	CHECK_NOT_NULL_RETURN(out_obj);

	*out_obj = swap_chain->GetApiObject();
	return VG_SUCCESS;
}

VgResult vgSwapChainGetDevice(VgSwapChain swap_chain, VgDevice* out_device)
{
	FUNC_DATA(vgSwapChainGetDevice);
	CHECK_NOT_NULL_RETURN(swap_chain);
	CHECK_NOT_NULL_RETURN(out_device);

	*out_device = swap_chain->Device();
	return VG_SUCCESS;
}

VgResult vgSwapChainGetDesc(VgSwapChain swap_chain, VgSwapChainDesc* out_desc)
{
	FUNC_DATA(vgSwapChainGetDesc);
	CHECK_NOT_NULL_RETURN(swap_chain);
	CHECK_NOT_NULL_RETURN(out_desc);

	*out_desc = swap_chain->Desc();
	return VG_SUCCESS;
}

VgResult vgSwapChainAcquireNextImage(VgSwapChain swap_chain, uint32_t* out_image_index)
{
	FUNC_DATA(vgSwapChainAcquireNextImage);
	CHECK_NOT_NULL_RETURN(swap_chain);
	CHECK_NOT_NULL_RETURN(out_image_index);

	try
	{
		*out_image_index = swap_chain->AcquireNextImage();
	}
	catch (VgError& ex)
	{
		return ex.result;
	}
	return VG_SUCCESS;
}

VgResult vgSwapChainGetBackBuffer(VgSwapChain swap_chain, uint32_t index, VgTexture* out_back_buffer)
{
	FUNC_DATA(vgSwapChainGetBackBuffer);
	CHECK_NOT_NULL_RETURN(swap_chain);
	CHECK_NOT_NULL_RETURN(out_back_buffer);

	try
	{
		*out_back_buffer = swap_chain->GetBackBuffer(index);
	}
	catch (VgError& ex)
	{
		return ex.result;
	}
	return VG_SUCCESS;
}

VgResult vgSwapChainPresent(VgSwapChain swap_chain, uint32_t num_wait_fences, VgFenceOperation* wait_fences)
{
	FUNC_DATA(vgSwapChainPresent);
	CHECK_NOT_NULL_RETURN(swap_chain);
	if (num_wait_fences > 0)
	{
		CHECK_NOT_NULL_RETURN(wait_fences);
	}

	try
	{
		swap_chain->Present(num_wait_fences, wait_fences);
	}
	catch (VgError& ex)
	{
		return ex.result;
	}
	return VG_SUCCESS;
}
