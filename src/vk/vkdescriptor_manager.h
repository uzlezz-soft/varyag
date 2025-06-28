#pragma once

#include "vkdevice.h"

#if VG_VULKAN_SUPPORTED

class VulkanDescriptorManager
{
public:
	inline static constexpr uint32_t NumResourceDescriptors = 250'000;
	inline static constexpr uint32_t NumSamplerDescriptors = 2'048;

	VulkanDescriptorManager(VulkanDevice& device);
	~VulkanDescriptorManager();

private:
	VulkanDevice* _device;

	VkDescriptorSetLayout _resourcesLayout;
	VkDescriptorSetLayout _immutableSamplersLayout;
	std::array<VkSampler, static_samplers.size()> _immutableSamplers;

	VkDescriptorPool _resourcesPool;
	VkDescriptorPool _immutableSamplersPool;

	VkDescriptorSet _resourcesSet;
	VkDescriptorSet _immutableSamplersSet;

	void CreateResourceDescriptorSet();
	void CreateImmutableSamplersSet();
};

#endif