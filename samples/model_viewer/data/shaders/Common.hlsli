#ifndef ROOT_SIGNATURE_HLSLI
#define ROOT_SIGNATURE_HLSLI

#ifdef __spirv__
#define PushConstants(Type, Name) [[vk::push_constant]] ConstantBuffer<Type> Name
#define VkLocation(b, set) [[vk::binding(b, set)]]

#if __SHADER_TARGET_STAGE == __SHADER_STAGE_VERTEX
#  define RHI_VERTEX_DATA , [[vk::builtin("DrawIndex")]] uint _internalVulkan_gl_DrawID : GLDRAWID
#  define GetDrawId() _internalVulkan_gl_DrawID
#endif

#define IS_VULKAN 1
#else
#define PushConstants(Type, Name) ConstantBuffer<Type> Name : register(b0)
#define VkLocation(binding, set)

#if __SHADER_TARGET_STAGE == __SHADER_STAGE_VERTEX
#  define RHI_VERTEX_DATA 
#  define GetDrawId() _internalIndirectDrawData.drawID
#endif

#define IS_VULKAN 0
#endif

#if __SHADER_TARGET_STAGE != __SHADER_STAGE_VERTEX
#define RHI_VERTEX_DATA
#define GetDrawId() 0
#endif

#define RS \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | SAMPLER_HEAP_DIRECTLY_INDEXED)," \
	"RootConstants(b0, num32BitConstants = 32)," \
	"RootConstants(b1, num32BitConstants = 1, visibility = SHADER_VISIBILITY_VERTEX)," \
	"StaticSampler(" \
		"s0," \
		"filter = FILTER_MIN_MAG_MIP_POINT," \
		"addressU = TEXTURE_ADDRESS_CLAMP," \
		"addressV = TEXTURE_ADDRESS_CLAMP," \
		"addressW = TEXTURE_ADDRESS_CLAMP," \
		"mipLODBias = 0.f," \
		"minLOD = 0.f," \
		"maxLOD = 100.f," \
		"visibility = SHADER_VISIBILITY_ALL)," \
	"StaticSampler(" \
		"s1," \
		"filter = FILTER_MIN_MAG_MIP_POINT," \
		"addressU = TEXTURE_ADDRESS_WRAP," \
		"addressV = TEXTURE_ADDRESS_WRAP," \
		"addressW = TEXTURE_ADDRESS_WRAP," \
		"mipLODBias = 0.f," \
		"minLOD = 0.f," \
		"maxLOD = 100.f," \
		"visibility = SHADER_VISIBILITY_ALL)," \
	"StaticSampler(" \
		"s2," \
		"filter = FILTER_MIN_MAG_MIP_LINEAR," \
		"addressU = TEXTURE_ADDRESS_CLAMP," \
		"addressV = TEXTURE_ADDRESS_CLAMP," \
		"addressW = TEXTURE_ADDRESS_CLAMP," \
		"mipLODBias = 0.f," \
		"minLOD = 0.f," \
		"maxLOD = 100.f," \
		"visibility = SHADER_VISIBILITY_ALL)," \
	"StaticSampler(" \
		"s3," \
		"filter = FILTER_MIN_MAG_MIP_LINEAR," \
		"addressU = TEXTURE_ADDRESS_WRAP," \
		"addressV = TEXTURE_ADDRESS_WRAP," \
		"addressW = TEXTURE_ADDRESS_WRAP," \
		"mipLODBias = 0.f," \
		"minLOD = 0.f," \
		"maxLOD = 100.f," \
		"visibility = SHADER_VISIBILITY_ALL)," \
	"StaticSampler(" \
		"s4," \
		"filter = FILTER_ANISOTROPIC," \
		"addressU = TEXTURE_ADDRESS_CLAMP," \
		"addressV = TEXTURE_ADDRESS_CLAMP," \
		"addressW = TEXTURE_ADDRESS_CLAMP," \
		"mipLODBias = 0.f," \
		"maxAnisotropy = 16," \
		"minLOD = 0.f," \
		"maxLOD = 100.f," \
		"visibility = SHADER_VISIBILITY_ALL)," \
	"StaticSampler(" \
		"s5," \
		"filter = FILTER_ANISOTROPIC," \
		"addressU = TEXTURE_ADDRESS_WRAP," \
		"addressV = TEXTURE_ADDRESS_WRAP," \
		"addressW = TEXTURE_ADDRESS_WRAP," \
		"mipLODBias = 0.f," \
		"maxAnisotropy = 16," \
		"minLOD = 0.f," \
		"maxLOD = 100.f," \
		"visibility = SHADER_VISIBILITY_ALL)," \
	"StaticSampler(" \
		"s6," \
		"filter = FILTER_MIN_MAG_LINEAR_MIP_POINT," \
		"addressU = TEXTURE_ADDRESS_CLAMP," \
		"addressV = TEXTURE_ADDRESS_CLAMP," \
		"addressW = TEXTURE_ADDRESS_CLAMP," \
		"mipLODBias = 0.f," \
		"minLOD = 0.f," \
		"maxLOD = 100.f," \
		"visibility = SHADER_VISIBILITY_ALL)," \
	"StaticSampler(" \
		"s7," \
		"filter = FILTER_MIN_MAG_LINEAR_MIP_POINT," \
		"addressU = TEXTURE_ADDRESS_WRAP," \
		"addressV = TEXTURE_ADDRESS_WRAP," \
		"addressW = TEXTURE_ADDRESS_WRAP," \
		"mipLODBias = 0.f," \
		"minLOD = 0.f," \
		"maxLOD = 100.f," \
		"visibility = SHADER_VISIBILITY_ALL)," \
	"StaticSampler(" \
		"s8," \
		"filter = FILTER_MIN_MAG_LINEAR_MIP_POINT," \
		"addressU = TEXTURE_ADDRESS_BORDER," \
		"addressV = TEXTURE_ADDRESS_BORDER," \
		"addressW = TEXTURE_ADDRESS_BORDER," \
		"mipLODBias = 0.f," \
		"minLOD = 0.f," \
		"maxLOD = 100.f," \
		"borderColor = STATIC_BORDER_COLOR_OPAQUE_BLACK," \
		"visibility = SHADER_VISIBILITY_ALL)," \
	"StaticSampler(" \
		"s9," \
		"filter = FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT," \
		"addressU = TEXTURE_ADDRESS_CLAMP," \
		"addressV = TEXTURE_ADDRESS_CLAMP," \
		"addressW = TEXTURE_ADDRESS_CLAMP," \
		"mipLODBias = 0.f," \
		"minLOD = 0.f," \
		"maxLOD = 100.f," \
		"visibility = SHADER_VISIBILITY_ALL)," \
	"StaticSampler(" \
		"s10," \
		"filter = FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT," \
		"addressU = TEXTURE_ADDRESS_CLAMP," \
		"addressV = TEXTURE_ADDRESS_CLAMP," \
		"addressW = TEXTURE_ADDRESS_CLAMP," \
		"mipLODBias = 0.f," \
		"minLOD = 0.f," \
		"maxLOD = 100.f," \
		"visibility = SHADER_VISIBILITY_ALL)," \
	"StaticSampler(" \
		"s11," \
		"filter = FILTER_MIN_MAG_LINEAR_MIP_POINT," \
		"addressU = TEXTURE_ADDRESS_BORDER," \
		"addressV = TEXTURE_ADDRESS_BORDER," \
		"addressW = TEXTURE_ADDRESS_BORDER," \
		"mipLODBias = 0.f," \
		"minLOD = 0.f," \
		"maxLOD = 100.f," \
		"borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK," \
		"visibility = SHADER_VISIBILITY_ALL)" \

VkLocation(0, 1) SamplerState nearestClamp: register(s0);
VkLocation(1, 1) SamplerState nearestWrap: register(s1);
VkLocation(2, 1) SamplerState linearClamp : register(s2);
VkLocation(3, 1) SamplerState linearWrap : register(s3);
VkLocation(4, 1) SamplerState anisotropicClamp : register(s4);
VkLocation(5, 1) SamplerState anisotropicWrap : register(s5);
VkLocation(6, 1) SamplerState linearMipNearestClamp : register(s6);
VkLocation(7, 1) SamplerState linearMipNearestWrap : register(s7);
VkLocation(8, 1) SamplerState downsampleBorder : register(s8);  // Black border color, see: https://www.froyok.fr/blog/2021-12-ue4-custom-bloom/

VkLocation(9, 1) SamplerState linearMipNearestClampMinimum : register(s9);
VkLocation(10, 1) SamplerState linearMipNearestClampMaximum : register(s10);
VkLocation(11, 1) SamplerState linearMipNearestTransparentBorder : register(s11);  // Border with alpha=0.

uint UnpackTexture(uint index) {
    return index & 0x00FFFFFF;
}

uint UnpackSampler(uint index) {
    return (index & 0xFF000000) >> 24;
}

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

template <typename TFormat, typename TUv>
TFormat SampleTexture2D(uint index, TUv uv) {
	Texture2D<TFormat> tex = ResourceDescriptorHeap[NonUniformResourceIndex(UnpackTexture(index))];
	SamplerState smp = SamplerDescriptorHeap[NonUniformResourceIndex(UnpackSampler(index))];
    return tex.Sample(smp, uv);
}

template <typename TFormat, typename TUv>
TFormat SampleTexture2DLevel(uint index, TUv uv, float lod) {
	Texture2D<TFormat> tex = ResourceDescriptorHeap[NonUniformResourceIndex(UnpackTexture(index))];
    SamplerState smp = SamplerDescriptorHeap[NonUniformResourceIndex(UnpackSampler(index))];
    return tex.SampleLevel(smp, uv, lod);
}

#if !IS_VULKAN && __SHADER_TARGET_STAGE == __SHADER_STAGE_VERTEX
struct InternalIndirectDrawData
{
    uint drawID;
};

ConstantBuffer<InternalIndirectDrawData> _internalIndirectDrawData : register(b1);
#endif

#endif