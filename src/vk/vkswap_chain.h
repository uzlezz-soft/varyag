#pragma once

#include "vkdevice.h"

#if VG_VULKAN_SUPPORTED

// TODO
class VulkanSwapChain final : public VgSwapChain_t
{
public:
	~VulkanSwapChain();

	void* GetApiObject() const override { return _swapChain; }
	VulkanDevice* Device() const override { return _device; }
	const VgSwapChainDesc& Desc() const override { return _desc; }

	uint32_t AcquireNextImage() override;
	VgTexture_t* GetBackBuffer(uint32_t index) override;
	void Present(uint32_t numWaitFences, VgFenceOperation* waitFences) override;

private:
	VulkanDevice* _device;
	VgSwapChainDesc _desc;
	VkSwapchainKHR _swapChain;

	vg::Vector<VgTexture_t*> _backBuffers;
	uint32_t _imageIndex;

	friend VulkanDevice;

	VulkanSwapChain(VulkanDevice& device, const VgSwapChainDesc& desc);
};

#endif