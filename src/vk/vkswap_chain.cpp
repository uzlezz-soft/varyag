#include "vkswap_chain.h"

#if VG_VULKAN_SUPPORTED

VulkanSwapChain::~VulkanSwapChain()
{
}

uint32_t VulkanSwapChain::AcquireNextImage()
{
	return 0;
}

VgTexture_t* VulkanSwapChain::GetBackBuffer(uint32_t index)
{
	return _backBuffers[index];
}

void VulkanSwapChain::Present(uint32_t numWaitFences, VgFenceOperation* waitFences)
{
}

// TODO
VulkanSwapChain::VulkanSwapChain(VulkanDevice& device, const VgSwapChainDesc& desc)
{
	auto& vk = device.Functions();

	VkSwapchainCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.surface = static_cast<VkSurfaceKHR>(desc.surface),
		.minImageCount = desc.buffer_count,
		//.imageFormat = desc.height
	};
	vk.vkCreateSwapchainKHR(device.Device(), &createInfo, device.AllocationCallbacks(), &_swapChain);
}

#endif