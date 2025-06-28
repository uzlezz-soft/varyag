#pragma once

#include "vkcore.h"
#include <vector>
#include <memory>

#if VG_VULKAN_SUPPORTED

struct VulkanExtensions
{
	uint8_t MutableDescriptors : 1;
};

class VulkanAdapter final : public VgAdapter_t
{
public:
	VulkanAdapter(VulkanCore& core, vkb::PhysicalDevice physicalDevice);
	~VulkanAdapter();

	void* GetApiObject() const override { return _adapter; }
	VgGraphicsApi Api() const override { return VG_GRAPHICS_API_VULKAN; }
	const VgAdapterProperties& GetProperties() const override { return _properties; }
	vkb::PhysicalDevice PhysicalDevice() const { return _adapter; }
	VulkanCore* Core() const { return _core; }
	const VulkanExtensions& Extensions() const { return _extensions; }

	VgDevice_t* CreateDevice(VgInitFlags initFlags) override;

private:
	VulkanCore* _core;
	vkb::PhysicalDevice _adapter;
	VgAdapterProperties _properties;

	VulkanExtensions _extensions;
};

#endif