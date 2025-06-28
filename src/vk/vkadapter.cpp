#include "vkadapter.h"
#include "vkdevice.h"

#if VG_VULKAN_SUPPORTED

constexpr VgAdapterType PhysicalDeviceTypeToAdapterType(VkPhysicalDeviceType type)
{
	switch (type)
	{
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return VG_ADAPTER_TYPE_INTEGRATED;
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return VG_ADAPTER_TYPE_DISCRETE;
	default: return VG_ADAPTER_TYPE_SOFTWARE;
	}
}

constexpr void ComputeMemorySizes(VgAdapterProperties& properties, VkPhysicalDeviceMemoryProperties memoryProperties)
{
	for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i)
	{
		VkMemoryHeap heap = memoryProperties.memoryHeaps[i];
		bool isDeviceLocal = heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
		bool isHostVisible = false;

		for (uint32_t j = 0; j < memoryProperties.memoryTypeCount; ++j)
		{
			if (memoryProperties.memoryTypes[j].heapIndex == i)
			{
				if (memoryProperties.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
					isHostVisible = true;
					break;
				}
			}
		}

		if (isDeviceLocal && !isHostVisible) properties.dedicated_vram += heap.size;
		else if (isDeviceLocal && isHostVisible) properties.shared_ram += heap.size;
		else if (!isDeviceLocal) properties.dedicated_ram += heap.size;
	}
}

VulkanAdapter::VulkanAdapter(VulkanCore& core, vkb::PhysicalDevice physicalDevice)
	: _core(&core), _adapter(physicalDevice)
{
	_properties = {
		PhysicalDeviceTypeToAdapterType(physicalDevice.properties.deviceType),
		"", 0, 0, 0, physicalDevice.enable_extension_if_present(VK_EXT_MESH_SHADER_EXTENSION_NAME),
		physicalDevice.enable_extension_if_present(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
		&& physicalDevice.enable_extension_if_present(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
		&& physicalDevice.enable_extension_if_present(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME)
		&& physicalDevice.enable_extension_if_present(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
		&& physicalDevice.enable_extension_if_present(VK_KHR_RAY_QUERY_EXTENSION_NAME)
	};
	memcpy(_properties.name, physicalDevice.properties.deviceName, sizeof(_properties.name));
	ComputeMemorySizes(_properties, physicalDevice.memory_properties);

	_properties.mesh_shaders &= physicalDevice.enable_extension_features_if_present(VulkanCore::meshShaderFeatures);

	_properties.hardware_ray_tracing &= physicalDevice.enable_extension_features_if_present(VulkanCore::accelerationStructureFeatures)
		&& physicalDevice.enable_extension_features_if_present(VulkanCore::rayTracingPipelineFeatures)
		&& physicalDevice.enable_extension_features_if_present(VulkanCore::rayQueryFeatures);

	_extensions = {
		.MutableDescriptors = (physicalDevice.enable_extension_if_present(VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME)
			&& physicalDevice.enable_extension_features_if_present(VulkanCore::mutableDescriptorTypeFeatures)) && false
	};
}

VulkanAdapter::~VulkanAdapter()
{
}

VgDevice_t* VulkanAdapter::CreateDevice(VgInitFlags initFlags)
{
	return new (GetAllocator().Allocate<VulkanDevice>()) VulkanDevice(*this, initFlags);
}

#endif