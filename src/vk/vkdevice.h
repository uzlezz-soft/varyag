#pragma once

#include "vkcore.h"
#include "vkadapter.h"

#if VG_VULKAN_SUPPORTED

constexpr void SamplerDescToVk(const VgSamplerDesc& desc, VkSamplerCreateInfo& info,
	VkSamplerCustomBorderColorCreateInfoEXT& customBorderColor, VkSamplerReductionModeCreateInfo& reductionMode)
{
	const auto convertAddressMode = [](VgAddressMode mode)
	{
		switch (mode)
		{
		case VG_ADDRESS_MODE_REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case VG_ADDRESS_MODE_MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case VG_ADDRESS_MODE_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case VG_ADDRESS_MODE_CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case VG_ADDRESS_MODE_MIRROR_ONCE: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
		}
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	};

	reductionMode = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO,
		.pNext = nullptr,
		.reductionMode = desc.reduction_mode == VG_REDUCTION_MODE_DEFAULT ? VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE
		: (desc.reduction_mode == VG_REDUCTION_MODE_MAX ? VK_SAMPLER_REDUCTION_MODE_MAX : VK_SAMPLER_REDUCTION_MODE_MIN)
	};

	customBorderColor = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT,
		.pNext = &reductionMode,
		.customBorderColor = VkClearColorValue {
			.float32 = {desc.border_color[0], desc.border_color[1], desc.border_color[2], desc.border_color[3]}
		},
		.format = VK_FORMAT_UNDEFINED
	};

	info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = &customBorderColor,
		.flags = 0,
		.magFilter = desc.mag_filter == VG_FILTER_LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
		.minFilter = desc.min_filter == VG_FILTER_LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
		.mipmapMode = desc.mipmap_mode == VG_MIPMAP_MODE_LINEAR ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = convertAddressMode(desc.address_u),
		.addressModeV = convertAddressMode(desc.address_v),
		.addressModeW = convertAddressMode(desc.address_w),
		.mipLodBias = desc.mip_lod_bias,
		.anisotropyEnable = desc.max_anisotropy > VG_ANISOTROPY_1,
		.maxAnisotropy = static_cast<float>(1u << desc.max_anisotropy),
		.minLod = desc.min_lod,
		.maxLod = desc.max_lod,
		.borderColor = VK_BORDER_COLOR_FLOAT_CUSTOM_EXT,
		.unnormalizedCoordinates = false
	};
}

class VulkanDescriptorManager;
class VulkanDevice final : public VgDevice_t
{
public:
	VulkanDevice(VulkanAdapter& adapter, VgInitFlags initFlags);
	~VulkanDevice();

	void* GetApiObject() const override { return _device; }
	VgGraphicsApi Api() const override { return VG_GRAPHICS_API_VULKAN; }
	const VgMemoryStatistics& GetMemoryStatistics() const override { return _memStats; }
	VgMemoryStatistics& GetMemoryStatistics() { return _memStats; }

	VulkanAdapter* Adapter() const override { return _adapter; }
	VkDevice Device() const { return _device; }
	VmaAllocator Allocator() const { return _allocator; }
	VulkanCore& Core() const { return *_adapter->Core(); }
	const VkAllocationCallbacks* AllocationCallbacks() const { return Core().Allocator(); }
	const VolkDeviceTable& Functions() const { return _functions; }

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

	void SignalFence(VgFence_t* fence, uint64_t value) override;
	void WaitFence(VgFence_t* fence, uint64_t value) override;
	uint64_t GetFenceValue(VgFence_t* fence) override;

	void SubmitCommandLists(uint32_t numSubmits, const VgSubmitInfo* submits) override;

	uint32_t GetSamplerIndex(VgSampler_t* sampler) override;

private:
	VulkanAdapter* _adapter;
	vkb::Device _device;
	VmaAllocator _allocator;

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;
	VkQueue _computeQueue;
	uint32_t _computeQueueFamily;
	VkQueue _transferQueue;
	uint32_t _transferQueueFamily;

	VulkanDescriptorManager* _descriptorManager;

	VolkDeviceTable _functions;
	VgMemoryStatistics _memStats;
};

#endif