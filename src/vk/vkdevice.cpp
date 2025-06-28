#include "vkdevice.h"
#include "vkdescriptor_manager.h"

#if VG_VULKAN_SUPPORTED

VulkanDevice::VulkanDevice(VulkanAdapter& adapter, VgInitFlags initFlags)
	: _adapter(&adapter)
{
	vkb::DeviceBuilder deviceBuilder(adapter.PhysicalDevice());

	auto deviceRet = deviceBuilder
		.set_allocation_callbacks(const_cast<VkAllocationCallbacks*>(adapter.Core()->Allocator()))
		.build();
	if (!deviceRet.has_value())
		throw VgFailure(std::format("Failed to create device from adapter {}", adapter.GetProperties().name));
	_device = deviceRet.value();

	volkLoadDeviceTable(&_functions, _device);

	VmaVulkanFunctions vulkanFunctions = {
		.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
		.vkGetDeviceProcAddr = vkGetDeviceProcAddr,
		.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
		.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
		.vkAllocateMemory = _functions.vkAllocateMemory,
		.vkFreeMemory = _functions.vkFreeMemory,
		.vkMapMemory = _functions.vkMapMemory,
		.vkUnmapMemory = _functions.vkUnmapMemory,
		.vkFlushMappedMemoryRanges = _functions.vkFlushMappedMemoryRanges,
		.vkInvalidateMappedMemoryRanges = _functions.vkInvalidateMappedMemoryRanges,
		.vkBindBufferMemory = _functions.vkBindBufferMemory,
		.vkBindImageMemory = _functions.vkBindImageMemory,
		.vkGetBufferMemoryRequirements = _functions.vkGetBufferMemoryRequirements,
		.vkGetImageMemoryRequirements = _functions.vkGetImageMemoryRequirements,
		.vkCreateBuffer = _functions.vkCreateBuffer,
		.vkDestroyBuffer = _functions.vkDestroyBuffer,
		.vkCreateImage = _functions.vkCreateImage,
		.vkDestroyImage = _functions.vkDestroyImage,
		.vkCmdCopyBuffer = _functions.vkCmdCopyBuffer,
		.vkGetBufferMemoryRequirements2KHR = _functions.vkGetBufferMemoryRequirements2,
		.vkGetImageMemoryRequirements2KHR = _functions.vkGetImageMemoryRequirements2,
		.vkBindBufferMemory2KHR = _functions.vkBindBufferMemory2,
		.vkBindImageMemory2KHR = _functions.vkBindImageMemory2,
		.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2,
		.vkGetDeviceBufferMemoryRequirements = _functions.vkGetDeviceBufferMemoryRequirements,
		.vkGetDeviceImageMemoryRequirements = _functions.vkGetDeviceImageMemoryRequirements,
		.vkGetMemoryWin32HandleKHR = nullptr
	};

	VmaAllocatorCreateInfo allocatorCreateInfo = {
		.flags = 0,
		.physicalDevice = adapter.PhysicalDevice(),
		.device = _device,
		.preferredLargeHeapBlockSize = 0,
		.pAllocationCallbacks = AllocationCallbacks(),
		.pDeviceMemoryCallbacks = nullptr,
		.pHeapSizeLimit = nullptr,
		.pVulkanFunctions = &vulkanFunctions,
		.instance = Core().Instance(),
		.vulkanApiVersion = VK_API_VERSION_1_3,
		.pTypeExternalMemoryHandleTypes = nullptr
	};

	VmaAllocator allocator;
	VkThrowOnError(vmaCreateAllocator(&allocatorCreateInfo, &allocator));

	if (auto queue = _device.get_queue(vkb::QueueType::graphics); queue.has_value())
	{
		_graphicsQueue = queue.value();
		_graphicsQueueFamily = _device.get_queue_index(vkb::QueueType::graphics).value();
	}
	else throw VgFailure(std::format("Unable to get graphics queue"));

	if (auto queue = _device.get_queue(vkb::QueueType::compute); queue.has_value())
	{
		_computeQueue = queue.value();
		_computeQueueFamily = _device.get_queue_index(vkb::QueueType::compute).value();
	}
	else throw VgFailure(std::format("Unable to get compute queue"));

	if (auto queue = _device.get_queue(vkb::QueueType::transfer); queue.has_value())
	{
		_transferQueue = queue.value();
		_transferQueueFamily = _device.get_queue_index(vkb::QueueType::transfer).value();
	}
	else throw VgFailure(std::format("Unable to get transfer queue"));

	_descriptorManager = new (GetAllocator().Allocate<VulkanDescriptorManager>()) VulkanDescriptorManager(*this);
}

VulkanDevice::~VulkanDevice()
{
	auto& fn = _functions;

	GetAllocator().Delete(_descriptorManager);
	vmaDestroyAllocator(_allocator);
	vkb::destroy_device(_device);
}

VgCommandPool VulkanDevice::CreateCommandPool(VgCommandPoolFlags flags, VgQueue queue)
{
	return VgCommandPool();
}

void VulkanDevice::DestroyCommandPool(VgCommandPool pool)
{
}

VgBuffer VulkanDevice::CreateBuffer(const VgBufferDesc& desc)
{
	return VgBuffer();
}

void VulkanDevice::DestroyBuffer(VgBuffer buffer)
{
}

VgShaderModule VulkanDevice::CreateShaderModule(const void* data, uint64_t size)
{
	return VgShaderModule();
}

void VulkanDevice::DestroyShaderModule(VgShaderModule module)
{
}

VgPipeline VulkanDevice::CreateGraphicsPipeline(const VgGraphicsPipelineDesc& desc)
{
	return VgPipeline();
}

VgPipeline VulkanDevice::CreateComputePipeline(VgShaderModule shaderModule)
{
	return VgPipeline();
}

void VulkanDevice::DestroyPipeline(VgPipeline pipeline)
{
}

VgFence VulkanDevice::CreateFence(uint64_t initialValue)
{
	return VgFence();
}

void VulkanDevice::DestroyFence(VgFence fence)
{
}

VgSampler VulkanDevice::CreateSampler(const VgSamplerDesc& desc)
{
	return VgSampler();
}

void VulkanDevice::DestroySampler(VgSampler sampler)
{
}

VgTexture VulkanDevice::CreateTexture(const VgTextureDesc& desc)
{
	return VgTexture();
}

void VulkanDevice::DestroyTexture(VgTexture texture)
{
}

VgSwapChain VulkanDevice::CreateSwapChain(const VgSwapChainDesc& desc)
{
	return VgSwapChain();
}

void VulkanDevice::DestroySwapChain(VgSwapChain swapChain)
{
}

void VulkanDevice::WaitQueueIdle(VgQueue queue)
{
}

void VulkanDevice::WaitIdle()
{
}

void VulkanDevice::SignalFence(VgFence_t* fence, uint64_t value)
{
}

void VulkanDevice::WaitFence(VgFence_t* fence, uint64_t value)
{
}

uint64_t VulkanDevice::GetFenceValue(VgFence_t* fence)
{
	return 0;
}

void VulkanDevice::SubmitCommandLists(uint32_t numSubmits, const VgSubmitInfo* submits)
{
}

uint32_t VulkanDevice::GetSamplerIndex(VgSampler_t* sampler)
{
	return 0;
}

#endif