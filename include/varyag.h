#pragma once

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// 
// Maybe when VG_VALIDATION is defined, add validation to API-specific objects?
// Like manage object allocations in Device and then check if any objects were
// not deleted or something
// 
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#ifdef VG_EXPORT
#define VG_API __declspec(dllexport)
#else
#define VG_API __declspec(dllimport)
#endif
#else
#define VG_API
#endif

#ifndef VG_VALIDATION
#define VG_VALIDATION 1
#endif

#ifdef __cplusplus
#define VG_OBJECT_DEF class
#else
#define VG_OBJECT_DEF struct
#endif

#define VG_API_D3D12 100

//#define VG_FORCE_API VG_API_D3D12

#if VG_FORCE_API == VG_API_D3D12
#define VG_DECLARE_HANDLE(name, d3d12name, vulkanname) \
VG_OBJECT_DEF d3d12name; \
typedef d3d12name* name
#else
#define VG_DECLARE_HANDLE(name, d3d12name, vulkanname) \
struct name##_t; \
typedef name##_t* name
#endif

#define VG_DECLARE_OPAQUE_HANDLE(name) \
typedef void name##_t; \
typedef name##_t* name

#ifdef __cplusplus
#include <utility>
#include <type_traits>
#define VG_ENUM_FLAGS(e) \
} \
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
extern "C" {
#else
#define VG_ENUM_FLAGS(e)
#endif

VG_DECLARE_HANDLE(VgAdapter, D3D12Adapter, VulkanAdapter);
VG_DECLARE_HANDLE(VgDevice, D3D12Device, VulkanDevice);
VG_DECLARE_HANDLE(VgCommandPool, D3D12CommandPool, VulkanCommandPool);
VG_DECLARE_HANDLE(VgCommandList, D3D12CommandList, VulkanCommandList);
VG_DECLARE_HANDLE(VgBuffer, D3D12Buffer, VulkanBuffer);
VG_DECLARE_HANDLE(VgShaderModule, D3D12ShaderModule, VulkanShaderModule);
VG_DECLARE_HANDLE(VgPipeline, D3D12Pipeline, VulkanPipeline);
VG_DECLARE_HANDLE(VgTexture, D3D12Texture, VulkanTexture);
VG_DECLARE_HANDLE(VgSwapChain, D3D12SwapChain, VulkanSwapChain);

VG_DECLARE_OPAQUE_HANDLE(VgFence);
VG_DECLARE_OPAQUE_HANDLE(VgSampler);
VG_DECLARE_OPAQUE_HANDLE(VgSurface);

typedef uint32_t VgView;
typedef uint32_t VgAttachmentView;

#undef VG_OBJECT_DEF

#include <stdint.h>

#define VG_INVALID_INDEX ((uint32_t)0xFFFFFFFF)
#define VG_WHOLE_SIZE ((uint64_t)-1)
#define VG_QUEUE_IGNORE ((VgQueue)-1)
#define VG_REMAINING_MIP_LAYERS (~0U)
#define VG_NO_VIEW (VG_INVALID_INDEX)
#if !defined(__cplusplus)
#define VG_DEFAULT_COMPONENT_SWIZZLE ((ComponentSwizzle){ VG_COMPONENT_MAPPING_IDENTITY, \
	VG_COMPONENT_MAPPING_IDENTITY, VG_COMPONENT_MAPPING_IDENTITY,VG_COMPONENT_MAPPING_IDENTITY })
#else
#define VG_DEFAULT_COMPONENT_SWIZZLE (VgComponentSwizzle { VG_COMPONENT_MAPPING_IDENTITY, \
	VG_COMPONENT_MAPPING_IDENTITY, VG_COMPONENT_MAPPING_IDENTITY,VG_COMPONENT_MAPPING_IDENTITY })
#endif
#define VG_COLOR_COMPONENT_ALL (VG_COLOR_COMPONENT_R | VG_COLOR_COMPONENT_G | VG_COLOR_COMPONENT_B | VG_COLOR_COMPONENT_A)

#define VG_NUM_ALLOWED_ROOT_CONSTANTS 32u
#define VG_NUM_MAX_VIEWPORTS_AND_SCISSORS 16u
#define VG_NUM_MAX_COLOR_ATTACHMENTS 8u
#define VG_NUM_MAX_ADAPTER_NAME_LENGTH 256u
#define VG_NUM_MAX_VERTEX_BUFFERS 16u
#define VG_NUM_MAX_VERTEX_ATTRIBUTES 16u

#ifdef __cplusplus
extern "C" {
#endif

	static const uint32_t vg_num_allowed_root_constants = VG_NUM_ALLOWED_ROOT_CONSTANTS;
	static const uint32_t vg_num_max_viewports_and_scissors = VG_NUM_MAX_VIEWPORTS_AND_SCISSORS;
	static const uint32_t vg_num_max_color_attachments = VG_NUM_MAX_COLOR_ATTACHMENTS;
	static const uint32_t vg_num_max_adapter_name_length = VG_NUM_MAX_ADAPTER_NAME_LENGTH;
	static const uint32_t vg_num_max_vertex_buffers = VG_NUM_MAX_VERTEX_BUFFERS;
	static const uint32_t vg_num_max_vertex_attributes = VG_NUM_MAX_VERTEX_ATTRIBUTES;

	typedef enum VgGraphicsApi : uint64_t
	{
		VG_GRAPHICS_API_AUTO = 0,
		VG_GRAPHICS_API_D3D12 = 1,
		/* TODO */
		VG_GRAPHICS_API_VULKAN = 2
	} VgGraphicsApi;

	// =========================================================
	// Required Vulkan extensions:
	// ---------------------------------------------------------
	// VK_EXT_conservative_rasterization
	// VK_KHR_multiview
	// =========================================================

	// =========================================================
	// Required Vulkan physical device features:
	// ---------------------------------------------------------
	// independentBlend
	// dualSrcBlend
	// core 1.1: shaderDrawParameters
	// =========================================================

	typedef enum VgResult : uint64_t
	{
		VG_SUCCESS = 0,
		VG_API_UNSUPPORTED = 1,
		VG_BAD_ARGUMENT = 2,
		VG_ALREADY_INITIALIZED = 3,
		VG_FAILURE = 4,
		VG_ILLEGAL_OPERATION = 5,
		VG_DEVICE_LOST = 6
	} VgResult;

	typedef enum VgMessageSeverity : uint64_t
	{
		VG_MESSAGE_SEVERITY_INFO = 0,
		VG_MESSAGE_SEVERITY_DEBUG = 1,
		VG_MESSAGE_SEVERITY_WARN = 2,
		VG_MESSAGE_SEVERITY_ERROR = 3
	} VgMessageSeverity;

	typedef enum VgAdapterType : uint64_t
	{
		VG_ADAPTER_TYPE_DISCRETE = 0,
		VG_ADAPTER_TYPE_INTEGRATED = 1,
		VG_ADAPTER_TYPE_SOFTWARE = 2
	} VgAdapterType;

	typedef enum VgQueue : uint64_t
	{
		VG_QUEUE_GRAPHICS = 0,
		VG_QUEUE_COMPUTE = 1,
		VG_QUEUE_TRANSFER = 2
	} VgQueue;

	typedef enum VgFormat : uint64_t
	{
		VG_FORMAT_UNKNOWN = 0,
		VG_FORMAT_R32G32B32A32_TYPELESS = 1,
		VG_FORMAT_R32G32B32A32_FLOAT = 2,
		VG_FORMAT_R32G32B32A32_UINT = 3,
		VG_FORMAT_R32G32B32A32_SINT = 4,
		VG_FORMAT_R32G32B32_TYPELESS = 5,
		VG_FORMAT_R32G32B32_FLOAT = 6,
		VG_FORMAT_R32G32B32_UINT = 7,
		VG_FORMAT_R32G32B32_SINT = 8,
		VG_FORMAT_R16G16B16A16_TYPELESS = 9,
		VG_FORMAT_R16G16B16A16_FLOAT = 10,
		VG_FORMAT_R16G16B16A16_UNORM = 11,
		VG_FORMAT_R16G16B16A16_UINT = 12,
		VG_FORMAT_R16G16B16A16_SNORM = 13,
		VG_FORMAT_R16G16B16A16_SINT = 14,
		VG_FORMAT_R32G32_TYPELESS = 15,
		VG_FORMAT_R32G32_FLOAT = 16,
		VG_FORMAT_R32G32_UINT = 17,
		VG_FORMAT_R32G32_SINT = 18,
		VG_FORMAT_R32G8X24_TYPELESS = 19,
		VG_FORMAT_D32_FLOAT_S8X24_UINT = 20,
		VG_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
		VG_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
		VG_FORMAT_R10G10B10A2_TYPELESS = 23,
		VG_FORMAT_R10G10B10A2_UNORM = 24,
		VG_FORMAT_R10G10B10A2_UINT = 25,
		VG_FORMAT_R11G11B10_FLOAT = 26,
		VG_FORMAT_R8G8B8A8_TYPELESS = 27,
		VG_FORMAT_R8G8B8A8_UNORM = 28,
		VG_FORMAT_R8G8B8A8_SRGB = 29,
		VG_FORMAT_R8G8B8A8_UINT = 30,
		VG_FORMAT_R8G8B8A8_SNORM = 31,
		VG_FORMAT_R8G8B8A8_SINT = 32,
		VG_FORMAT_R16G16_TYPELESS = 33,
		VG_FORMAT_R16G16_FLOAT = 34,
		VG_FORMAT_R16G16_UNORM = 35,
		VG_FORMAT_R16G16_UINT = 36,
		VG_FORMAT_R16G16_SNORM = 37,
		VG_FORMAT_R16G16_SINT = 38,
		VG_FORMAT_R32_TYPELESS = 39,
		VG_FORMAT_D32_FLOAT = 40,
		VG_FORMAT_R32_FLOAT = 41,
		VG_FORMAT_R32_UINT = 42,
		VG_FORMAT_R32_SINT = 43,
		VG_FORMAT_R24G8_TYPELESS = 44,
		VG_FORMAT_D24_UNORM_S8_UINT = 45,
		VG_FORMAT_R24_UNORM_X8_TYPELESS = 46,
		VG_FORMAT_X24_TYPELESS_G8_UINT = 47,
		VG_FORMAT_R8G8_TYPELESS = 48,
		VG_FORMAT_R8G8_UNORM = 49,
		VG_FORMAT_R8G8_UINT = 50,
		VG_FORMAT_R8G8_SNORM = 51,
		VG_FORMAT_R8G8_SINT = 52,
		VG_FORMAT_R16_TYPELESS = 53,
		VG_FORMAT_R16_FLOAT = 54,
		VG_FORMAT_D16_UNORM = 55,
		VG_FORMAT_R16_UNORM = 56,
		VG_FORMAT_R16_UINT = 57,
		VG_FORMAT_R16_SNORM = 58,
		VG_FORMAT_R16_SINT = 59,
		VG_FORMAT_R8_TYPELESS = 60,
		VG_FORMAT_R8_UNORM = 61,
		VG_FORMAT_R8_UINT = 62,
		VG_FORMAT_R8_SNORM = 63,
		VG_FORMAT_R8_SINT = 64,
		VG_FORMAT_A8_UNORM = 65,
		VG_FORMAT_R1_UNORM = 66,
		VG_FORMAT_R9G9B9E5_SHAREDEXP = 67,
		VG_FORMAT_R8G8_B8G8_UNORM = 68,
		VG_FORMAT_G8R8_G8B8_UNORM = 69,
		VG_FORMAT_BC1_TYPELESS = 70,
		VG_FORMAT_BC1_UNORM = 71,
		VG_FORMAT_BC1_SRGB = 72,
		VG_FORMAT_BC2_TYPELESS = 73,
		VG_FORMAT_BC2_UNORM = 74,
		VG_FORMAT_BC2_SRGB = 75,
		VG_FORMAT_BC3_TYPELESS = 76,
		VG_FORMAT_BC3_UNORM = 77,
		VG_FORMAT_BC3_SRGB = 78,
		VG_FORMAT_BC4_TYPELESS = 79,
		VG_FORMAT_BC4_UNORM = 80,
		VG_FORMAT_BC4_SNORM = 81,
		VG_FORMAT_BC5_TYPELESS = 82,
		VG_FORMAT_BC5_UNORM = 83,
		VG_FORMAT_BC5_SNORM = 84,
		VG_FORMAT_B5G6R5_UNORM = 85,
		VG_FORMAT_B5G5R5A1_UNORM = 86,
		VG_FORMAT_B8G8R8A8_UNORM = 87,
		VG_FORMAT_B8G8R8X8_UNORM = 88,
		VG_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
		VG_FORMAT_B8G8R8A8_TYPELESS = 90,
		VG_FORMAT_B8G8R8A8_SRGB = 91,
		VG_FORMAT_B8G8R8X8_TYPELESS = 92,
		VG_FORMAT_B8G8R8X8_SRGB = 93,
		VG_FORMAT_BC6H_TYPELESS = 94,
		VG_FORMAT_BC6H_UF16 = 95,
		VG_FORMAT_BC6H_SF16 = 96,
		VG_FORMAT_BC7_TYPELESS = 97,
		VG_FORMAT_BC7_UNORM = 98,
		VG_FORMAT_BC7_SRGB = 99,
	} VgFormat;

	typedef enum VgHeapType : uint64_t
	{
		VG_HEAP_TYPE_GPU = 1,
		VG_HEAP_TYPE_UPLOAD = 2,
		VG_HEAP_TYPE_READBACK = 3
	} VgHeapType;

	typedef enum VgBufferUsage : uint64_t
	{
		VG_BUFFER_USAGE_GENERAL = 0,
		VG_BUFFER_USAGE_CONSTANT = 1
	} VgBufferUsage;

	typedef enum VgInitFlags : uint64_t
	{
		VG_INIT_NONE = 0,
		VG_INIT_DEBUG = 1,
		VG_INIT_ENABLE_MESSAGE_CALLBACK = 2,
		VG_INIT_USE_PROVIDED_ALLOCATOR = 4
	} VgInitFlags;
	VG_ENUM_FLAGS(VgInitFlags);

	typedef enum VgIndexType : uint64_t
	{
		VG_INDEX_TYPE_UINT16 = 0,
		VG_INDEX_TYPE_UINT32 = 1
	} VgIndexType;

	typedef enum VgPipelineType : uint64_t
	{
		VG_PIPELINE_TYPE_GRAPHICS = 0,
		VG_PIPELINE_TYPE_COMPUTE = 1
	} VgPipelineType;

	typedef enum VgBufferDescriptorType : uint64_t
	{
		VG_BUFFER_DESCRIPTOR_TYPE_SRV = 0,
		VG_BUFFER_DESCRIPTOR_TYPE_UAV = 1,
		VG_BUFFER_DESCRIPTOR_TYPE_CBV = 2,
	} VgBufferDescriptorType;

	typedef enum VgBufferViewType : uint64_t
	{
		VG_BUFFER_VIEW_TYPE_BUFFER = 0,
		VG_BUFFER_VIEW_TYPE_STRUCTURED_BUFFER = 1,
		VG_BUFFER_VIEW_TYPE_BYTE_ADDRESS_BUFFER = 2
	} VgBufferViewType;

	typedef enum VgCommandPoolFlags : uint64_t
	{
		VG_COMMAND_POOL_FLAG_NONE = 0,
		VG_COMMAND_POOL_FLAG_TRANSIENT = 1,
	} VgCommandPoolFlags;
	VG_ENUM_FLAGS(VgCommandPoolFlags);

	typedef enum VgPipelineStageFlags : uint64_t
	{
		VG_PIPELINE_STAGE_NONE = 0ULL,
		VG_PIPELINE_STAGE_TOP_OF_PIPE = 0x00000001ULL,
		VG_PIPELINE_STAGE_DRAW_INDIRECT = 0x00000002ULL,
		VG_PIPELINE_STAGE_VERTEX_INPUT = 0x00000004ULL,
		VG_PIPELINE_STAGE_VERTEX_SHADER = 0x00000008ULL,
		VG_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER = 0x00000010ULL,
		VG_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER = 0x00000020ULL,
		VG_PIPELINE_STAGE_GEOMETRY_SHADER = 0x00000040ULL,
		VG_PIPELINE_STAGE_FRAGMENT_SHADER = 0x00000080ULL,
		VG_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS = 0x00000100ULL,
		VG_PIPELINE_STAGE_LATE_FRAGMENT_TESTS = 0x00000200ULL,
		VG_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT = 0x00000400ULL,
		VG_PIPELINE_STAGE_COMPUTE_SHADER = 0x00000800ULL,
		VG_PIPELINE_STAGE_ALL_TRANSFER = 0x00001000ULL,
		VG_PIPELINE_STAGE_TRANSFER = 0x00001000ULL,
		VG_PIPELINE_STAGE_BOTTOM_OF_PIPE = 0x00002000ULL,
		VG_PIPELINE_STAGE_ALL_GRAPHICS = 0x00008000ULL,
		VG_PIPELINE_STAGE_ALL_COMMANDS = 0x00010000ULL,
		VG_PIPELINE_STAGE_RESOLVE = 0x200000000ULL,
		VG_PIPELINE_STAGE_BLIT = 0x400000000ULL,
		VG_PIPELINE_STAGE_CLEAR = 0x800000000ULL,
		VG_PIPELINE_STAGE_INDEX_INPUT = 0x1000000000ULL,
		VG_PIPELINE_STAGE_VERTEX_ATTRIBUTE_INPUT = 0x2000000000ULL,
		VG_PIPELINE_STAGE_PRE_RASTERIZATION_SHADERS = 0x4000000000ULL,
		// Uncomment in d3d12device.h
		//VG_PIPELINE_STAGE_VIDEO_DECODE = 0x04000000ULL,
		//VG_PIPELINE_STAGE_VIDEO_ENCODE = 0x08000000ULL,
		VG_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD = 0x02000000ULL,
		VG_PIPELINE_STAGE_ACCELERATION_STRUCTURE_COPY = 0x10000000ULL,
		VG_PIPELINE_STAGE_RAY_TRACING_SHADER = 0x00200000ULL,
		VG_PIPELINE_STAGE_TASK_SHADER = 0x00080000ULL,
		VG_PIPELINE_STAGE_MESH_SHADER = 0x00100000ULL
	} VgPipelineStageFlags;
	VG_ENUM_FLAGS(VgPipelineStageFlags);

	typedef enum VgAccessFlags : uint64_t
	{
		VG_ACCESS_NONE = 0ULL,
		VG_ACCESS_INDIRECT_COMMAND_READ = 0x00000001ULL,
		VG_ACCESS_INDEX_READ = 0x00000002ULL,
		VG_ACCESS_VERTEX_ATTRIBUTE_READ = 0x00000004ULL,
		VG_ACCESS_UNIFORM_READ = 0x00000008ULL,
		VG_ACCESS_INPUT_ATTACHMENT_READ = 0x00000010ULL,
		VG_ACCESS_SHADER_READ = 0x00000020ULL,
		VG_ACCESS_SHADER_WRITE = 0x00000040ULL,
		VG_ACCESS_COLOR_ATTACHMENT_READ = 0x00000080ULL,
		VG_ACCESS_COLOR_ATTACHMENT_WRITE = 0x00000100ULL,
		VG_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ = 0x00000200ULL,
		VG_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE = 0x00000400ULL,
		VG_ACCESS_TRANSFER_READ = 0x00000800ULL,
		VG_ACCESS_TRANSFER_WRITE = 0x00001000ULL,
		VG_ACCESS_MEMORY_READ = 0x00008000ULL,
		VG_ACCESS_MEMORY_WRITE = 0x00010000ULL,
		VG_ACCESS_SHADER_SAMPLED_READ = 0x100000000ULL,
		VG_ACCESS_SHADER_STORAGE_READ = 0x200000000ULL,
		VG_ACCESS_SHADER_STORAGE_WRITE = 0x400000000ULL,
		// Uncomment in d3d12device.h
		//VG_ACCESS_VIDEO_DECODE_READ = 0x800000000ULL,
		//VG_ACCESS_VIDEO_DECODE_WRITE = 0x1000000000ULL,
		//VG_ACCESS_VIDEO_ENCODE_READ = 0x2000000000ULL,
		//VG_ACCESS_VIDEO_ENCODE_WRITE = 0x4000000000ULL,
		VG_ACCESS_ACCELERATION_STRUCTURE_READ = 0x00200000ULL,
		VG_ACCESS_ACCELERATION_STRUCTURE_WRITE = 0x00400000ULL,
		VG_ACCESS_SHADER_BINDING_TABLE_READ = 0x10000000000ULL,
		//VG_ACCESS_COMMON = 0x20000000000ULL
	} VgAccessFlags;
	VG_ENUM_FLAGS(VgAccessFlags);

	typedef enum VgFilter : uint64_t
	{
		VG_FILTER_NEAREST = 0,
		VG_FILTER_LINEAR = 1
	} VgFilter;

	typedef enum VgMipmapMode : uint64_t
	{
		VG_MIPMAP_MODE_NEAREST = 0,
		VG_MIPMAP_MODE_LINEAR = 1
	} VgMipmapMode;

	typedef enum VgAddressMode : uint64_t
	{
		VG_ADDRESS_MODE_REPEAT = 0,
		VG_ADDRESS_MODE_MIRRORED_REPEAT = 1,
		VG_ADDRESS_MODE_CLAMP_TO_EDGE = 2,
		VG_ADDRESS_MODE_CLAMP_TO_BORDER = 3,
		VG_ADDRESS_MODE_MIRROR_ONCE = 4
	} VgAddressMode;

	typedef enum VgAnisotropy : uint64_t
	{
		VG_ANISOTROPY_1 = 0,
		VG_ANISOTROPY_2 = 1,
		VG_ANISOTROPY_4 = 2,
		VG_ANISOTROPY_8 = 3,
		VG_ANISOTROPY_16 = 4,
	} VgAnisotropy;

	typedef enum VgComparisonFunc : uint64_t
	{
		VG_COMPARISON_FUNC_NONE = 0,
		VG_COMPARISON_FUNC_NEVER = 1,
		VG_COMPARISON_FUNC_LESS = 2,
		VG_COMPARISON_FUNC_EQUAL = 3,
		VG_COMPARISON_FUNC_LESS_EQUAL = 4,
		VG_COMPARISON_FUNC_GREATER = 5,
		VG_COMPARISON_FUNC_NOT_EQUAL = 6,
		VG_COMPARISON_FUNC_GREATER_EQUAL = 7,
		VG_COMPARISON_FUNC_ALWAYS = 8
	} VgComparisonFunc;

	typedef enum VgReductionMode : uint64_t
	{
		VG_REDUCTION_MODE_DEFAULT = 0,
		VG_REDUCTION_MODE_MIN = 1,
		VG_REDUCTION_MODE_MAX = 2
	} VgReductionMode;

	typedef enum VgTextureLayout : uint64_t
	{
		VG_TEXTURE_LAYOUT_UNDEFINED = 0,
		VG_TEXTURE_LAYOUT_GENERAL = 1,
		VG_TEXTURE_LAYOUT_COLOR_ATTACHMENT = 2,
		VG_TEXTURE_LAYOUT_DEPTH_STENCIL = 3,
		VG_TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY = 4,
		VG_TEXTURE_LAYOUT_SHADER_RESOURCE = 5,
		VG_TEXTURE_LAYOUT_TRANSFER_SOURCE = 6,
		VG_TEXTURE_LAYOUT_TRANSFER_DEST = 7,
		VG_TEXTURE_LAYOUT_RESOLVE_SOURCE = 8,
		VG_TEXTURE_LAYOUT_RESOLVE_DEST = 9,
		VG_TEXTURE_LAYOUT_PRESENT = 10,
		// SHADER_RESOURCE + TRANSFER_SOURCE
		VG_TEXTURE_LAYOUT_READ_ONLY = 11,
		VG_TEXTURE_LAYOUT_UNORDERED_ACCESS = 12
	} VgTextureLayout;

	typedef enum VgTextureType : uint64_t
	{
		VG_TEXTURE_TYPE_1D = 0,
		VG_TEXTURE_TYPE_2D = 1,
		VG_TEXTURE_TYPE_3D = 2
	} VgTextureType;

	typedef enum VgSampleCount : uint64_t
	{
		VG_SAMPLE_COUNT_1 = 0,
		VG_SAMPLE_COUNT_2 = 1,
		VG_SAMPLE_COUNT_4 = 2,
		VG_SAMPLE_COUNT_8 = 3
	} VgSampleCount;

	typedef enum VgTextureUsageFlags : uint64_t
	{
		VG_TEXTURE_USAGE_SHADER_RESOURCE = 1,
		VG_TEXTURE_USAGE_UNORDERED_ACCESS = 2,
		VG_TEXTURE_USAGE_COLOR_ATTACHMENT = 4,
		VG_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT = 8,
		VG_TEXTURE_USAGE_ALLOW_SIMULTANEOUS_ACCESS = 16
	} VgTextureUsageFlags;
	VG_ENUM_FLAGS(VgTextureUsageFlags);

	typedef enum VgTextureTiling : uint64_t
	{
		VG_TEXTURE_TILING_OPTIMAL = 0,
		VG_TEXTURE_TILING_LINEAR = 1
	} VgTextureTiling;

	typedef enum VgTextureAttachmentViewType : uint64_t
	{
		VG_TEXTURE_ATTACHMENT_VIEW_TYPE_1D = 0,
		VG_TEXTURE_ATTACHMENT_VIEW_TYPE_1D_ARRAY = 1,
		VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D = 2,
		VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_ARRAY = 3,
		VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_MS = 4,
		VG_TEXTURE_ATTACHMENT_VIEW_TYPE_2D_MS_ARRAY = 5,
		VG_TEXTURE_ATTACHMENT_VIEW_TYPE_3D = 6
	} VgTextureAttachmentViewType;

	typedef enum VgTextureViewType : uint64_t
	{
		VG_TEXTURE_VIEW_TYPE_1D = 0,
		VG_TEXTURE_VIEW_TYPE_1D_ARRAY = 1,
		VG_TEXTURE_VIEW_TYPE_2D = 3,
		VG_TEXTURE_VIEW_TYPE_2D_ARRAY = 4,
		VG_TEXTURE_VIEW_TYPE_2D_MS = 5,
		VG_TEXTURE_VIEW_TYPE_2D_MS_ARRAY = 6,
		VG_TEXTURE_VIEW_TYPE_3D = 7,
		VG_TEXTURE_VIEW_TYPE_CUBE = 8,
		VG_TEXTURE_VIEW_TYPE_CUBE_ARRAY = 9,
	} VgTextureViewType;

	typedef enum VgTextureAspect : uint64_t
	{
		VG_TEXTURE_ASPECT_COLOR = 0,
		VG_TEXTURE_ASPECT_DEPTH = 1,
		VG_TEXTURE_ASPECT_STENCIL = 2
	} VgTextureAspect;

	typedef enum VgResolveMode : uint64_t
	{
		VG_RESOLVE_MODE_AVERAGE = 0,
		VG_RESOLVE_MODE_MIN = 1,
		VG_RESOLVE_MODE_MAX = 2
	} VgResolveMode;

	typedef enum VgAttachmentOp : uint64_t
	{
		VG_ATTACHMENT_OP_DEFAULT = 0,
		VG_ATTACHMENT_OP_CLEAR = 1,
		VG_ATTACHMENT_OP_DONT_CARE = 2
	} VgAttachmentOp;

	typedef enum VgTextureDescriptorType : uint64_t
	{
		VG_TEXTURE_DESCRIPTOR_TYPE_SRV = 0,
		VG_TEXTURE_DESCRIPTOR_TYPE_UAV = 1
	} VgTextureDescriptorType;

	typedef enum VgComponentMapping : uint64_t
	{
		VG_COMPONENT_MAPPING_IDENTITY = 0,
		VG_COMPONENT_MAPPING_ZERO = 1,
		VG_COMPONENT_MAPPING_ONE = 2,
		VG_COMPONENT_MAPPING_R = 3,
		VG_COMPONENT_MAPPING_G = 4,
		VG_COMPONENT_MAPPING_B = 5,
		VG_COMPONENT_MAPPING_A = 6,
	} VgComponentMapping;

	typedef enum VgVertexPipeline : uint64_t
	{
		VG_VERTEX_PIPELINE_FIXED_FUNCTION = 0,
		VG_VERTEX_PIPELINE_MESH_SHADER = 1
	} VgVertexPipeline;

	typedef enum VgAttributeInputRate : uint64_t
	{
		VG_ATTRIBUTE_INPUT_RATE_VERTEX = 0,
		VG_ATTRIBUTE_INPUT_RATE_INSTANCE = 1,
	} VgAttributeInputRate;

	typedef enum VgPrimitiveTopology : uint64_t
	{
		VG_PRIMITIVE_TOPOLOGY_POINT_LIST = 0,
		VG_PRIMITIVE_TOPOLOGY_LINE_LIST = 1,
		VG_PRIMITIVE_TOPOLOGY_LINE_STRIP = 2,
		VG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3,
		VG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4,
		VG_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY = 6,
		VG_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY = 7,
		VG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY = 8,
		VG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY = 9,
		VG_PRIMITIVE_TOPOLOGY_PATCH_LIST = 10
	} VgPrimitiveTopology;

	typedef enum VgFillMode : uint64_t
	{
		VG_FILL_MODE_FILL = 0,
		VG_FILL_MODE_WIREFRAME = 1
	} VgFillMode;

	typedef enum VgFrontFace : uint64_t
	{
		VG_FRONT_FACE_CLOCKWISE = 0,
		VG_FRONT_FACE_COUNTER_CLOCKWISE = 1
	} VgFrontFace;

	typedef enum VgCullMode : uint64_t
	{
		VG_CULL_MODE_NONE = 0,
		VG_CULL_MODE_FRONT = 1,
		VG_CULL_MODE_BACK = 2
	} VgCullMode;

	typedef enum VgDepthClipMode : uint64_t
	{
		VG_DEPTH_CLIP_MODE_CLIP = 0,
		VG_DEPTH_CLIP_MODE_CLAMP = 1
	} VgDepthClipMode;

	typedef enum VgCompareOp : uint64_t
	{
		VG_COMPARE_OP_NEVER = 0,
		VG_COMPARE_OP_LESS = 1,
		VG_COMPARE_OP_EQUAL = 2,
		VG_COMPARE_OP_LESS_OR_EQUAL = 3,
		VG_COMPARE_OP_GREATER = 4,
		VG_COMPARE_OP_NOT_EQUAL = 5,
		VG_COMPARE_OP_GREATER_OR_EQUAL = 6,
		VG_COMPARE_OP_ALWAYS = 7
	} VgCompareOp;

	typedef enum VgStencilOp : uint64_t
	{
		VG_STENCIL_OP_KEEP = 0,
		VG_STENCIL_OP_ZERO = 1,
		VG_STENCIL_OP_REPLACE = 2,
		VG_STENCIL_OP_INCREMENT_AND_CLAMP = 3,
		VG_STENCIL_OP_DECREMENT_AND_CLAMP = 4,
		VG_STENCIL_OP_INVERT = 5,
		VG_STENCIL_OP_INCREMENT_AND_WRAP = 6,
		VG_STENCIL_OP_DECREMENT_AND_WRAP = 7
	} VgStencilOp;

	typedef enum VgLogicOp : uint64_t
	{
		VG_LOGIC_OP_CLEAR = 0,
		VG_LOGIC_OP_AND = 1,
		VG_LOGIC_OP_AND_REVERSE = 2,
		VG_LOGIC_OP_COPY = 3,
		VG_LOGIC_OP_AND_INVERTED = 4,
		VG_LOGIC_OP_NO_OP = 5,
		VG_LOGIC_OP_XOR = 6,
		VG_LOGIC_OP_OR = 7,
		VG_LOGIC_OP_NOR = 8,
		VG_LOGIC_OP_EQUIVALENT = 9,
		VG_LOGIC_OP_INVERT = 10,
		VG_LOGIC_OP_OR_REVERSE = 11,
		VG_LOGIC_OP_COPY_INVERTED = 12,
		VG_LOGIC_OP_OR_INVERTED = 13,
		VG_LOGIC_OP_NAND = 14,
		VG_LOGIC_OP_SET = 15
	} VgLogicOp;

	typedef enum VgBlendFactor : uint64_t
	{
		VG_BLEND_FACTOR_ZERO = 0,
		VG_BLEND_FACTOR_ONE = 1,
		VG_BLEND_FACTOR_SRC_COLOR = 2,
		VG_BLEND_FACTOR_ONE_MINUS_SRC_COLOR = 3,
		VG_BLEND_FACTOR_DST_COLOR = 4,
		VG_BLEND_FACTOR_ONE_MINUS_DST_COLOR = 5,
		VG_BLEND_FACTOR_SRC_ALPHA = 6,
		VG_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 7,
		VG_BLEND_FACTOR_DST_ALPHA = 8,
		VG_BLEND_FACTOR_ONE_MINUS_DST_ALPHA = 9,
		VG_BLEND_FACTOR_CONSTANT_COLOR = 10,
		VG_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR = 11,
		VG_BLEND_FACTOR_CONSTANT_ALPHA = 12,
		VG_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA = 13,
		VG_BLEND_FACTOR_SRC_ALPHA_SATURATE = 14,
		VG_BLEND_FACTOR_SRC1_COLOR = 15,
		VG_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR = 16,
		VG_BLEND_FACTOR_SRC1_ALPHA = 17,
		VG_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA = 18
	} VgBlendFactor;

	typedef enum VgBlendOp : uint64_t
	{
		VG_BLEND_OP_ADD = 0,
		VG_BLEND_OP_SUBTRACT = 1,
		VG_BLEND_OP_REVERSE_SUBTRACT = 2,
		VG_BLEND_OP_MIN = 3,
		VG_BLEND_OP_MAX = 4
	} VgBlendOp;

	typedef enum VgColorComponentFlags : uint64_t
	{
		VG_COLOR_COMPONENT_R = 1,
		VG_COLOR_COMPONENT_G = 2,
		VG_COLOR_COMPONENT_B = 4,
		VG_COLOR_COMPONENT_A = 8
	} VgColorComponentFlags;
	VG_ENUM_FLAGS(VgColorComponentFlags);

	typedef void* (*VgAllocPFN)(void* user_data, size_t size, size_t alignment);
	typedef void* (*VgReallocPFN)(void* user_data, void* original, size_t size, size_t alignment);
	typedef void(*VgFreePFN)(void* user_data, void* memory);
	typedef void(*VgMessageCallbackPFN)(VgMessageSeverity severity, const char* msg);

	typedef struct VgAllocator
	{
		void* user_data;
		VgAllocPFN alloc;
		VgReallocPFN realloc;
		VgFreePFN free;
	} VgAllocator;

	typedef struct VgConfig
	{
		VgInitFlags flags;
		VgMessageCallbackPFN message_callback;
		VgAllocator allocator;
	} VgConfig;

	typedef struct VgAdapterProperties
	{
		VgAdapterType type;
		char name[VG_NUM_MAX_ADAPTER_NAME_LENGTH];
		uint64_t dedicated_vram;
		uint64_t dedicated_ram;
		uint64_t shared_ram;
		bool mesh_shaders;
		bool hardware_ray_tracing;
	} VgAdapterProperties;

	typedef struct VgBufferDesc
	{
		uint64_t size;
		VgBufferUsage usage;
		VgHeapType heap_type;
	} VgBufferDesc;

	typedef struct VgVertexBufferView
	{
		VgBuffer buffer;
		uint64_t offset;
		uint32_t stride_in_bytes;
	} VgVertexBufferView;

	typedef struct VgBufferViewDesc
	{
		VgBufferDescriptorType descriptor_type;
		VgBufferViewType view_type;
		VgFormat format;
		uint64_t offset;
		uint64_t size;
		uint64_t element_size;
	} VgBufferViewDesc;

	typedef struct VgFenceOperation
	{
		VgFence fence;
		uint64_t value;
	} VgFenceOperation;

	typedef struct VgSubmitInfo
	{
		uint32_t num_wait_fences;
		VgFenceOperation* wait_fences;
		uint32_t num_signal_fences;
		VgFenceOperation* signal_fences;
		uint32_t num_command_lists;
		VgCommandList* command_lists;
	} VgSubmitInfo;

	typedef struct VgTextureSubresourceRange
	{
		uint32_t base_mip_level;
		uint32_t mip_levels;
		uint32_t base_array_layer;
		uint32_t array_layers;
	} VgTextureSubresourceRange;

	typedef struct VgMemoryBarrier
	{
		VgPipelineStageFlags src_stage;
		VgAccessFlags src_access;
		VgPipelineStageFlags dst_stage;
		VgAccessFlags dst_access;
	} VgMemoryBarrier;

	// TODO: Look into queue ownership transfers
	typedef struct VgBufferBarrier
	{
		VgPipelineStageFlags src_stage;
		VgAccessFlags src_access;
		VgPipelineStageFlags dst_stage;
		VgAccessFlags dst_access;
		VgBuffer buffer;
	} VgBufferBarrier;

	typedef struct VgTextureBarrier
	{
		VgPipelineStageFlags src_stage;
		VgAccessFlags src_access;
		VgPipelineStageFlags dst_stage;
		VgAccessFlags dst_access;
		VgTextureLayout old_layout;
		VgTextureLayout new_layout;
		VgTexture texture;
		VgTextureSubresourceRange subresource_range;
	} VgTextureBarrier;

	typedef struct VgDependencyInfo
	{
		uint32_t num_memory_barriers;
		VgMemoryBarrier* memory_barriers;
		uint32_t num_buffer_barriers;
		VgBufferBarrier* buffer_barriers;
		uint32_t num_texture_barriers;
		VgTextureBarrier* texture_barriers;
	} VgDependencyInfo;

	typedef struct VgSamplerDesc
	{
		VgFilter mag_filter;
		VgFilter min_filter;
		VgMipmapMode mipmap_mode;
		VgAddressMode address_u;
		VgAddressMode address_v;
		VgAddressMode address_w;
		float mip_lod_bias;
		VgAnisotropy max_anisotropy;
		VgComparisonFunc comparison_func;
		float border_color[4];
		float min_lod;
		float max_lod;
		VgReductionMode reduction_mode;
	} VgSamplerDesc;

	typedef struct VgTextureDesc
	{
		VgTextureType type;
		VgFormat format;
		uint32_t width;
		uint32_t height;
		uint32_t depth_or_array_layers;
		uint32_t mip_levels;
		VgSampleCount sample_count;
		VgTextureUsageFlags usage;
		VgTextureTiling tiling;
		VgTextureLayout initial_layout;
		VgHeapType heap_type;
	} VgTextureDesc;

	typedef struct VgAttachmentViewDesc
	{
		VgFormat format;
		VgTextureAttachmentViewType type;
		uint32_t mip;
		uint32_t base_array_layer;
		uint32_t array_layers;
	} VgAttachmentViewDesc;
	
	typedef struct VgClearValue
	{
		union
		{
			float color[4];
		};
		union
		{
			float depth;
		};
		union
		{
			uint32_t stencil;
		};
	} VgClearValue;

	typedef struct VgAttachmentInfo
	{
		VgAttachmentView view;
		VgTextureLayout view_layout;
		uint32_t resolve_view;
		VgResolveMode resolve_mode;
		VgTextureLayout resolve_view_layout;
		VgAttachmentOp load_op;
		VgAttachmentOp store_op;
		VgClearValue clear;
	} VgAttachmentInfo;

	typedef struct VgRenderingInfo
	{
		uint32_t num_color_attachments;
		VgAttachmentInfo* color_attachments;
		VgAttachmentInfo depth_stencil_attachment;
	} VgRenderingInfo;

	typedef struct VgComponentSwizzle
	{
		VgComponentMapping r;
		VgComponentMapping g;
		VgComponentMapping b;
		VgComponentMapping a;
	} VgComponentSwizzle;

	typedef struct VgTextureViewDesc
	{
		VgFormat format;
		VgTextureViewType type;
		VgTextureDescriptorType descriptor_type;
		VgComponentSwizzle components;
		uint32_t base_mip_level;
		uint32_t mip_levels;
		uint32_t base_array_layer;
		uint32_t array_layers;
	} VgTextureViewDesc;

	typedef struct VgOffset
	{
		uint32_t x;
		uint32_t y;
		uint32_t z;
	} VgOffset;

	typedef struct VgRegion
	{
		uint32_t mip;
		uint32_t base_array_layer;
		uint32_t array_layers;
		VgOffset offset;
		uint32_t width;
		uint32_t height;
		uint32_t depth;
	} VgRegion;

	typedef struct VgSwapChainDesc
	{
		uint32_t width;
		uint32_t height;
		VgFormat format;
		uint32_t buffer_count;
		// TODO: Swap effect
		VgSurface surface;
	} VgSwapChainDesc;

	typedef struct VgVertexAttribute
	{
		VgFormat format;
		uint32_t offset;
		uint32_t vertex_buffer_index;
		VgAttributeInputRate input_rate;
		uint32_t instance_step_rate;
	} VgVertexAttribute;

	typedef struct VgFixedFunctionState
	{
		uint32_t num_vertex_attributes;
		VgVertexAttribute* vertex_attributes;
		VgShaderModule vertex_shader;
		VgShaderModule hull_shader;
		VgShaderModule domain_shader;
		VgShaderModule geometry_shader;
	} VgFixedFunctionState;

	typedef struct VgMeshShaderState
	{
		VgShaderModule amplification_shader;
		VgShaderModule mesh_shader;
	} VgMeshShaderState;

	typedef struct VgRasterizationState
	{
		VgFillMode fill_mode;
		VgCullMode cull_mode;
		VgFrontFace front_face;
		VgDepthClipMode depth_clip_mode;
		int32_t depth_bias;
		float depth_bias_clamp;
		float depth_bias_slope_factor;
		bool conservative_rasterization_enable;
		bool rasterization_discard_enable;
	} VgRasterizationState;

	typedef struct VgStencilState
	{
		VgStencilOp fail_op;
		VgStencilOp depth_fail_op;
		VgStencilOp pass_op;
		VgCompareOp compare_op;
	} VgStencilState;

	typedef struct VgDepthStencilState
	{
		bool depth_test_enable;
		bool depth_write_enable;
		VgCompareOp depth_compare_op;
		bool stencil_test_enable;
		VgStencilState front;
		VgStencilState back;
		uint32_t stencil_read_mask;
		uint32_t stencil_write_mask;
		uint32_t stencil_reference;
		bool depth_bounds_test_enable;
		float min_depth_bounds;
		float max_depth_bounds;
	} VgDepthStencilState;

	typedef struct VgMultisamplingState
	{
		VgSampleCount sample_count;
		bool alpha_to_coverage;
	} VgMultisamplingState;

	typedef struct VgAttachmentBlendState
	{
		bool blend_enable;
		VgBlendFactor src_color;
		VgBlendFactor dst_color;
		VgBlendOp color_op;
		VgBlendFactor src_alpha;
		VgBlendFactor dst_alpha;
		VgBlendOp alpha_op;
		VgColorComponentFlags color_write_mask;
	} VgAttachmentBlendState;

	typedef struct VgBlendState
	{
		bool logic_op_enable;
		VgLogicOp logic_op;
		VgAttachmentBlendState attachments[vg_num_max_color_attachments];
		float blend_constants[4];
	} VgBlendState;

	typedef struct VgGraphicsPipelineDesc
	{
		VgVertexPipeline vertex_pipeline_type;
		VgFixedFunctionState fixed_function;
		VgMeshShaderState mesh;
		VgShaderModule pixel_shader;
		VgPrimitiveTopology primitive_topology;
		bool primitive_restart_enable;
		uint32_t tesselation_control_points;
		VgRasterizationState rasterization_state;
		VgMultisamplingState multisampling_state;
		VgDepthStencilState depth_stencil_state;
		uint32_t num_color_attachments;
		VgFormat color_attachment_formats[vg_num_max_color_attachments];
		VgFormat depth_stencil_format;
		VgBlendState blend_state;
	} VgGraphicsPipelineDesc;

	typedef struct VgViewport
	{
		float x;
		float y;
		float width;
		float height;
		float min_depth;
		float max_depth;
	} VgViewport;

	typedef struct VgScissor
	{
		uint32_t x;
		uint32_t y;
		uint32_t width;
		uint32_t height;
	} VgScissor;

	typedef struct VgMemoryStatistics
	{
		uint64_t num_buffers;
		uint64_t num_textures;
		uint64_t num_pipelines;
		uint64_t used_vram;
	} VgMemoryStatistics;

	typedef struct VgDrawIndirectCommand
	{
		uint32_t vertex_count;
		uint32_t instance_count;
		uint32_t fisrt_vertex;
		uint32_t first_instance;
	} VgDrawIndirectCommand;

	typedef struct VgDrawIndexedIndirectCommand
	{
		uint32_t index_count;
		uint32_t instance_count;
		uint32_t first_index;
		uint32_t vertex_offset;
		uint32_t first_instace;
	} VgDrawIndexedIndirectCommand;

	typedef struct VgDispatchIndirectCommand
	{
		uint32_t groups_x;
		uint32_t groups_y;
		uint32_t groups_z;
	} VgDispatchIndirectCommand;

	VG_API VgResult vgInit(const VgConfig* cfg);
	VG_API void vgShutdown();
	VG_API VgResult vgEnumerateApis(uint32_t* out_num_apis, VgGraphicsApi* out_apis);
	VG_API VgResult vgEnumerateAdapters(VgGraphicsApi api, uint32_t* out_num_adapters, VgAdapter* out_adapters);
	
	VG_API VgResult vgAdapterGetProperties(VgAdapter adapter, VgAdapterProperties* out_properties);
	VG_API VgResult vgAdapterCreateDevice(VgAdapter adapter, VgDevice* out_device);
	VG_API void vgAdapterDestroyDevice(VgAdapter adapter, VgDevice device);

	VG_API VgResult vgDeviceGetApiObject(VgDevice device, void** out_obj);
	VG_API VgResult vgDeviceGetAdapter(VgDevice device, VgAdapter* out_adapter);
	VG_API VgResult vgDeviceGetGraphicsApi(VgDevice device, VgGraphicsApi* out_api);
	VG_API VgResult vgDeviceGetMemoryStatistics(VgDevice device, VgMemoryStatistics* out_memory_stats);
	VG_API void vgDeviceWaitQueueIdle(VgDevice device, VgQueue queue);
	VG_API void vgDeviceWaitIdle(VgDevice device);
	VG_API VgResult vgDeviceCreateBuffer(VgDevice device, const VgBufferDesc* desc, VgBuffer* out_buffer);
	VG_API void vgDeviceDestroyBuffer(VgDevice device, VgBuffer buffer);
	VG_API VgResult vgDeviceCreateShaderModule(VgDevice device, const void* data, uint64_t size, VgShaderModule* out_module);
	VG_API void vgDeviceDestroyShaderModule(VgDevice device, VgShaderModule shader_module);
	VG_API VgResult vgDeviceCreateGraphicsPipeline(VgDevice device, const VgGraphicsPipelineDesc* desc, VgPipeline* out_pipeline);
	VG_API VgResult vgDeviceCreateComputePipeline(VgDevice device, VgShaderModule shader_module, VgPipeline* out_pipeline);
	VG_API void vgDeviceDestroyPipeline(VgDevice device, VgPipeline pipeline);
	VG_API VgResult vgDeviceCreateFence(VgDevice device, uint64_t initial_value, VgFence* out_fence);
	VG_API void vgDeviceDestroyFence(VgDevice device, VgFence fence);
	VG_API VgResult vgDeviceCreateCommandPool(VgDevice device, VgCommandPoolFlags flags, VgQueue queue, VgCommandPool* out_pool);
	VG_API void vgDeviceDestroyCommandPool(VgDevice device, VgCommandPool pool);
	VG_API VgResult vgDeviceCreateSampler(VgDevice device, const VgSamplerDesc* desc, VgSampler* out_sampler);
	VG_API void vgDeviceDestroySampler(VgDevice device, VgSampler sampler);
	VG_API VgResult vgDeviceCreateTexture(VgDevice device, const VgTextureDesc* desc, VgTexture* out_texture);
	VG_API void vgDeviceDestroyTexture(VgDevice device, VgTexture texture);
	VG_API void vgDeviceSubmitCommandLists(VgDevice device, uint32_t num_submits, VgSubmitInfo* submits);
	VG_API VgResult vgDeviceSignalFence(VgDevice device, VgFence fence, uint64_t value);
	VG_API void vgDeviceWaitFence(VgDevice device, VgFence fence, uint64_t value);
	VG_API VgResult vgDeviceGetFenceValue(VgDevice device, VgFence fence, uint64_t* out_value);
	VG_API VgResult vgDeviceCreateSwapChain(VgDevice device, const VgSwapChainDesc* desc, VgSwapChain* out_swap_chain);
	VG_API void vgDeviceDestroySwapChain(VgDevice device, VgSwapChain swap_chain);

	VG_API VgResult vgCommandPoolGetApiObject(VgCommandPool pool, void** out_obj);
	VG_API void vgCommandPoolSetName(VgCommandPool pool, const char* name);
	VG_API VgResult vgCommandPoolGetDevice(VgCommandPool pool, VgDevice* out_device);
	VG_API VgResult vgCommandPoolGetQueue(VgCommandPool pool, VgQueue* out_queue);
	VG_API VgResult vgCommandPoolAllocateCommandList(VgCommandPool pool, VgCommandList* out_cmd);
	VG_API void vgCommandPoolFreeCommandList(VgCommandPool pool, VgCommandList cmd);
	VG_API void vgCommandPoolReset(VgCommandPool pool);


	VG_API VgResult vgCommandListGetApiObject(VgCommandList list, void** out_obj);
	VG_API void vgCommandListSetName(VgCommandList list, const char* name);
	VG_API VgResult vgCommandListGetDevice(VgCommandList cmd, VgDevice* out_device);
	VG_API VgResult vgCommandListGetCommandPool(VgCommandList cmd, VgCommandPool* out_pool);
	VG_API VgResult vgCommandListGetQueue(VgCommandList cmd, VgQueue* out_queue);
	VG_API void vgCommandListRestoreDescriptorState(VgCommandList cmd);

	VG_API void vgCmdBegin(VgCommandList cmd);
	VG_API void vgCmdEnd(VgCommandList cmd);
	VG_API void vgCmdSetVertexBuffers(VgCommandList cmd, uint32_t start_slot, uint32_t num_buffers, const VgVertexBufferView* buffers);
	VG_API void vgCmdSetIndexBuffer(VgCommandList cmd, VgIndexType index_type, uint64_t offset, VgBuffer index_buffer);
	VG_API void vgCmdSetRootConstants(VgCommandList cmd, VgPipelineType pipeline_type, uint32_t offset_in_32bit_values, uint32_t num_32bit_values, const void* data);
	VG_API void vgCmdSetPipeline(VgCommandList cmd, VgPipeline pipeline);
	VG_API void vgCmdBarrier(VgCommandList cmd, const VgDependencyInfo* dependency_info);
	VG_API VgResult vgCmdBeginRendering(VgCommandList cmd, const VgRenderingInfo* info);
	VG_API void vgCmdEndRendering(VgCommandList cmd);
	VG_API void vgCmdSetViewport(VgCommandList cmd, uint32_t first_viewport, uint32_t num_viewports, VgViewport* viewports);
	VG_API void vgCmdSetScissor(VgCommandList cmd, uint32_t first_scissor, uint32_t num_scissors, VgScissor* scissors);

	VG_API void vgCmdDraw(VgCommandList cmd, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
	VG_API void vgCmdDrawIndexed(VgCommandList cmd, uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance);
	VG_API void vgCmdDispatch(VgCommandList cmd, uint32_t groups_x, uint32_t groups_y, uint32_t groups_z);
	VG_API void vgCmdDrawIndirect(VgCommandList cmd, VgBuffer buffer, uint64_t offset, uint32_t draw_count, uint32_t stride);
	VG_API void vgCmdDrawIndirectCount(VgCommandList cmd, VgBuffer buffer, uint64_t offset, VgBuffer count_buffer, uint64_t count_buffer_offset, uint32_t max_draw_count, uint32_t stride);
	VG_API void vgCmdDrawIndexedIndirect(VgCommandList cmd, VgBuffer buffer, uint64_t offset, uint32_t draw_count, uint32_t stride);
	VG_API void vgCmdDrawIndexedIndirectCount(VgCommandList cmd, VgBuffer buffer, uint64_t offset, VgBuffer count_buffer, uint64_t count_buffer_offset, uint32_t max_draw_count, uint32_t stride);
	VG_API void vgCmdDispatchIndirect(VgCommandList cmd, VgBuffer buffer, uint64_t offset);

	VG_API void vgCmdCopyBufferToBuffer(VgCommandList cmd, VgBuffer dst, uint64_t dst_offset, VgBuffer src, uint64_t src_offset, uint64_t size);
	VG_API void vgCmdCopyBufferToTexture(VgCommandList cmd, VgTexture dst, const VgRegion* dst_region, VgBuffer src, uint64_t src_offset);
	VG_API void vgCmdCopyTextureToBuffer(VgCommandList cmd, VgBuffer dst, uint64_t dst_offset, VgTexture src, const VgRegion* src_region);
	VG_API void vgCmdCopyTextureToTexture(VgCommandList cmd, VgTexture dst, const VgRegion* dst_region, VgTexture src, const VgRegion* src_region);

	VG_API void vgCmdBeginMarker(VgCommandList cmd, const char* name, float color[3]);
	VG_API void vgCmdEndMarker(VgCommandList cmd);


	VG_API VgResult vgBufferGetApiObject(VgBuffer buffer, void** out_obj);
	VG_API void vgBufferSetName(VgBuffer buffer, const char* name);
	VG_API VgResult vgBufferGetDevice(VgBuffer buffer, VgDevice* out_device);
	VG_API VgResult vgBufferGetDesc(VgBuffer buffer, VgBufferDesc* out_desc);
	VG_API VgResult vgBufferCreateView(VgBuffer buffer, const VgBufferViewDesc* desc, VgView* out_descriptor);
	VG_API void vgBufferDestroyViews(VgBuffer buffer);
	VG_API VgResult vgBufferMap(VgBuffer buffer, void** out_data);
	VG_API void vgBufferUnmap(VgBuffer buffer);

	VG_API VgResult vgPipelineGetApiObject(VgPipeline pipeline, void** out_obj);
	VG_API void vgPipelineSetName(VgPipeline pipeline, const char* name);
	VG_API VgResult vgPipelineGetDevice(VgPipeline pipeline, VgDevice* out_device);
	VG_API VgResult vgPipelineGetType(VgPipeline pipeline, VgPipelineType* out_type);

	VG_API VgResult vgDeviceGetSamplerIndex(VgDevice device, VgSampler sampler, uint32_t* out_index);

	VG_API VgResult vgTextureGetApiObject(VgTexture texture, void** out_obj);
	VG_API void vgTextureSetName(VgTexture texture, const char* name);
	VG_API VgResult vgTextureGetDevice(VgTexture texture, VgDevice* out_device);
	VG_API VgResult vgTextureGetDesc(VgTexture texture, VgTextureDesc* out_desc);
	VG_API VgResult vgTextureCreateAttachmentView(VgTexture texture, const VgAttachmentViewDesc* desc, VgAttachmentView* out_descriptor);
	VG_API VgResult vgTextureCreateView(VgTexture texture, const VgTextureViewDesc* desc, VgView* out_descriptor);
	VG_API void vgTextureDestroyViews(VgTexture texture);

	VG_API VgResult vgCreateSurfaceD3D12(void* hwnd, VgSurface* out_surface);
	VG_API VgResult vgCreateSurfaceVulkan(void* vk_surface, VgSurface* out_surface);
	VG_API void vgDestroySurfaceVulkan(VgSurface surface);

	VG_API VgResult vgSwapChainGetApiObject(VgSwapChain swap_chain, void** out_obj);
	VG_API VgResult vgSwapChainGetDevice(VgSwapChain swap_chain, VgDevice* out_device);
	VG_API VgResult vgSwapChainGetDesc(VgSwapChain swap_chain, VgSwapChainDesc* out_desc);
	VG_API VgResult vgSwapChainAcquireNextImage(VgSwapChain swap_chain, uint32_t* out_image_index);
	VG_API VgResult vgSwapChainGetBackBuffer(VgSwapChain swap_chain, uint32_t index, VgTexture* out_back_buffer);
	VG_API VgResult vgSwapChainPresent(VgSwapChain swap_chain, uint32_t num_wait_fences, VgFenceOperation* wait_fences);

#ifdef __cplusplus
}
#endif

#undef VG_DECLARE_HANDLE
#undef VG_DECLARE_OPAQUE_HANDLE
#undef VG_ENUM_FLAGS