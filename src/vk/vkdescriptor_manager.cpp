#include "vkdescriptor_manager.h"

#if VG_VULKAN_SUPPORTED


VulkanDescriptorManager::VulkanDescriptorManager(VulkanDevice& device)
	: _device(&device)
{
	CreateResourceDescriptorSet();
	CreateImmutableSamplersSet();
}

VulkanDescriptorManager::~VulkanDescriptorManager()
{
	auto& fn = _device->Functions();

	fn.vkDestroyDescriptorPool(_device->Device(), _resourcesPool, _device->AllocationCallbacks());
	fn.vkDestroyDescriptorPool(_device->Device(), _immutableSamplersPool, _device->AllocationCallbacks());

	fn.vkDestroyDescriptorSetLayout(_device->Device(), _resourcesLayout, _device->AllocationCallbacks());
	fn.vkDestroyDescriptorSetLayout(_device->Device(), _immutableSamplersLayout, _device->AllocationCallbacks());

	for (auto sampler : _immutableSamplers)
	{
		fn.vkDestroySampler(_device->Device(), sampler, _device->AllocationCallbacks());
	}
}

void VulkanDescriptorManager::CreateResourceDescriptorSet()
{
	auto& fn = _device->Functions();

	std::array<VkDescriptorSetLayoutBinding, 3> bindings = {
		VkDescriptorSetLayoutBinding {
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_MUTABLE_EXT,
			.descriptorCount = NumResourceDescriptors,
			.stageFlags = VK_SHADER_STAGE_ALL,
			.pImmutableSamplers = nullptr
		},
		VkDescriptorSetLayoutBinding {
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = NumResourceDescriptors,
			.stageFlags = VK_SHADER_STAGE_ALL,
			.pImmutableSamplers = nullptr
		},
		VkDescriptorSetLayoutBinding {
			.binding = 2,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
			.descriptorCount = NumSamplerDescriptors,
			.stageFlags = VK_SHADER_STAGE_ALL,
			.pImmutableSamplers = nullptr
		}
	};

	vg::Vector<VkDescriptorType> resourceDescriptorTypes = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		  VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE };
	if (_device->Adapter()->GetProperties().hardware_ray_tracing)
	{
		//resourceDescriptorTypes.push_back(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
	}

	VkMutableDescriptorTypeListEXT typeList{
		.descriptorTypeCount = static_cast<uint32_t>(resourceDescriptorTypes.size()),
		.pDescriptorTypes = resourceDescriptorTypes.data()
	};
	VkMutableDescriptorTypeCreateInfoEXT mutableInfo{
		.sType = VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT,
		.pNext = nullptr,
		.mutableDescriptorTypeListCount = 1,
		.pMutableDescriptorTypeLists = &typeList
	};

	std::array<VkDescriptorBindingFlags, bindings.size()> flags;
	for (uint64_t i = 0; i < flags.size(); i++)
	{
		flags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
	}

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO, &mutableInfo,
		static_cast<uint32_t>(flags.size()), flags.data()
	};
	VkDescriptorSetLayoutCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = &bindingFlags,
		.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	VkThrowOnError(fn.vkCreateDescriptorSetLayout(_device->Device(), &createInfo, _device->AllocationCallbacks(), &_resourcesLayout));

	constexpr std::array poolSizes = {
		VkDescriptorPoolSize{
			.type = VK_DESCRIPTOR_TYPE_MUTABLE_EXT,
			.descriptorCount = NumResourceDescriptors
		},
		VkDescriptorPoolSize{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = NumResourceDescriptors
		},
		VkDescriptorPoolSize{
			.type = VK_DESCRIPTOR_TYPE_SAMPLER,
			.descriptorCount = NumSamplerDescriptors
		}
	};
	VkDescriptorPoolCreateInfo poolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
		.maxSets = 1,
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data()
	};
	VkThrowOnError(fn.vkCreateDescriptorPool(_device->Device(), &poolCreateInfo, _device->AllocationCallbacks(), &_resourcesPool));

	VkDescriptorSetAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = _resourcesPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &_resourcesLayout
	};
	VkThrowOnError(fn.vkAllocateDescriptorSets(_device->Device(), &allocateInfo, &_resourcesSet));
}

void VulkanDescriptorManager::CreateImmutableSamplersSet()
{
	auto& fn = _device->Functions();

	std::array<VkDescriptorSetLayoutBinding, static_samplers.size()> bindings;

	VkSamplerReductionModeCreateInfo reductionMode;
	VkSamplerCustomBorderColorCreateInfoEXT customBorderColor;
	VkSamplerCreateInfo samplerCreateInfo;
	for (uint64_t i = 0; i < static_samplers.size(); i++)
	{
		SamplerDescToVk(static_samplers[i], samplerCreateInfo, customBorderColor, reductionMode);

		// Immutable samplers do not support custom border
		samplerCreateInfo.pNext = &reductionMode;
		reductionMode.pNext = nullptr;

		constexpr float borderColorFloatTransparentBlack[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		constexpr float borderColorFloatOpaqueBlack[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		constexpr float borderColorFloatOpaqueWhite[] = { 1.0f, 1.0f, 1.0f, 1.0f };

		if (memcmp(customBorderColor.customBorderColor.float32, borderColorFloatTransparentBlack, sizeof(float) * 4) == 0)
			samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		else if (memcmp(customBorderColor.customBorderColor.float32, borderColorFloatOpaqueBlack, sizeof(float) * 4) == 0)
			samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		else if (memcmp(customBorderColor.customBorderColor.float32, borderColorFloatOpaqueWhite, sizeof(float) * 4) == 0)
			samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		else
			samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

		VkThrowOnError(fn.vkCreateSampler(_device->Device(), &samplerCreateInfo, _device->AllocationCallbacks(), &_immutableSamplers[i]));

		bindings[i] = {
			.binding = static_cast<uint32_t>(i),
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_ALL,
			.pImmutableSamplers = &_immutableSamplers[i]
		};
	}

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};
	VkThrowOnError(fn.vkCreateDescriptorSetLayout(_device->Device(), &layoutCreateInfo, _device->AllocationCallbacks(), &_immutableSamplersLayout));
	
	const VkDescriptorPoolSize poolSize = {
		.type = VK_DESCRIPTOR_TYPE_SAMPLER,
		.descriptorCount = static_cast<uint32_t>(_immutableSamplers.size())
	};
	VkDescriptorPoolCreateInfo poolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = 1,
		.poolSizeCount = 1,
		.pPoolSizes = &poolSize
	};
	VkThrowOnError(fn.vkCreateDescriptorPool(_device->Device(), &poolCreateInfo, _device->AllocationCallbacks(), &_immutableSamplersPool));

	VkDescriptorSetAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = _immutableSamplersPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &_immutableSamplersLayout
	};
	VkThrowOnError(fn.vkAllocateDescriptorSets(_device->Device(), &allocateInfo, &_immutableSamplersSet));
}

#endif