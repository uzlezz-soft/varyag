#include "vkcore.h"
#include "vkadapter.h"
#include "vkdescriptor_manager.h"

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI
#include <windows.h>
#endif

#if VG_VULKAN_SUPPORTED
constexpr std::string_view MessageTypeToString(VkDebugUtilsMessageTypeFlagsEXT type)
{
	switch (type)
	{
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: return "[General]";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: return "[Validation]";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "[Performance]";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: return "[Device Address Binding]";
	default: return "[unknown]";
	}
}

VulkanCore::~VulkanCore()
{
	if (_instance)
	{
		vkb::destroy_instance(_instance);
		volkFinalize();
	}
}

VulkanCore* VulkanCore::LoadVulkan(const VgConfig& config)
{
	VulkanCore* core = (VulkanCore*)GetAllocator().Allocate<VulkanCore>();
	core->_initialized = false;

	core->_requestValidation = config.flags & VG_INIT_DEBUG;
	core->_applicationName = config.application_name;
	core->_engineName = config.engine_name;

#if _WIN32
	// If we load vulkan-1.dll on windows just as a library (which volk does)
	// The driver will do some fucked up shit
	// And if we later gonna use D3D12, it will
	// a) just crash
	// b) display artefacts

	// This will not trigger any driver stuff
	auto vulkanLib = LoadLibraryEx("vulkan-1.dll", nullptr, LOAD_LIBRARY_AS_DATAFILE);
	if (!vulkanLib)
	{
		GetAllocator().Delete(core);
		return nullptr;
	}
	FreeLibrary(vulkanLib);
#else
	if (!core->Initialize())
	{
		GetAllocator().Delete(core);
		return nullptr;
	}
#endif
	return core;
}

vg::Vector<VgAdapter_t*> VulkanCore::CollectAdapters(VgSurface surface)
{
	Initialize();

	vg::Vector<VgAdapter_t*> adapters;

	VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features13.dynamicRendering = true;
	features13.synchronization2 = true;

	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.descriptorIndexing = true;
	features12.runtimeDescriptorArray = true;
	features12.descriptorBindingPartiallyBound = true;
	features12.shaderStorageTexelBufferArrayDynamicIndexing = true;
	features12.descriptorBindingSampledImageUpdateAfterBind = true;
	features12.descriptorBindingStorageBufferUpdateAfterBind = true;
	features12.descriptorBindingStorageImageUpdateAfterBind = true;
	features12.descriptorBindingUniformBufferUpdateAfterBind = true;
	features12.shaderStorageTexelBufferArrayNonUniformIndexing = true;
	features12.shaderSampledImageArrayNonUniformIndexing = true;
	features12.shaderStorageBufferArrayNonUniformIndexing = true;
	features12.shaderStorageImageArrayNonUniformIndexing = true;
	features12.shaderUniformBufferArrayNonUniformIndexing = true;
	features12.drawIndirectCount = true;
	features12.samplerFilterMinmax = true;
	features12.samplerMirrorClampToEdge = true;
	features12.timelineSemaphore = true;
	features12.bufferDeviceAddress = true;
	

	VkPhysicalDeviceVulkan11Features features11{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
	features11.multiview = true;
	features11.shaderDrawParameters = true;

	VkPhysicalDeviceFeatures features = {
		.independentBlend = true,
		.tessellationShader = true,
		.sampleRateShading = true,
		.dualSrcBlend = true,
		.samplerAnisotropy = true,
		.textureCompressionBC = true,
		.fragmentStoresAndAtomics = true,
		.shaderUniformBufferArrayDynamicIndexing = true,
		.shaderSampledImageArrayDynamicIndexing = true,
		.shaderStorageBufferArrayDynamicIndexing = true,
		.shaderStorageImageArrayDynamicIndexing = true,
		.sparseBinding = true
	};

	vkb::PhysicalDeviceSelector selector{ _instance };

	for (auto extension : requiredExtensions)
	{
		selector.add_required_extension(extension);
	}

	auto devices = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features13)
		.set_required_features_12(features12)
		.set_required_features_11(features11)
		.set_required_features(features)
		.add_required_extension_features(mutableDescriptorTypeFeatures)
		.add_required_extension_features(VkPhysicalDeviceCustomBorderColorFeaturesEXT{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT,
			.pNext = nullptr,
			.customBorderColors = true,
			.customBorderColorWithoutFormat = true
		})
		.set_surface(static_cast<VkSurfaceKHR>(surface)).select_devices();
	if (!devices.has_value() || devices.value().empty()) return {};
	for (auto& device : devices.value())
	{
		VkPhysicalDeviceCustomBorderColorPropertiesEXT customBorderColorProperties = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_PROPERTIES_EXT,
			.pNext = nullptr
		};

		VkPhysicalDeviceProperties2 properties2 = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
			.pNext = &customBorderColorProperties
		};
		vkGetPhysicalDeviceProperties2(device, &properties2);

		if (customBorderColorProperties.maxCustomBorderColorSamplers < VulkanDescriptorManager::NumSamplerDescriptors)
		{
			continue;
		}

		adapters.push_back(new((GetAllocator().Allocate<VulkanAdapter>())) VulkanAdapter(*this, device));
	}

	return adapters;
}

void VulkanCore::DestroySurface(VkSurfaceKHR surface)
{
	vkDestroySurfaceKHR(_instance, surface, &_allocator);
}

bool VulkanCore::Initialize()
{
	if (_initialized) return true;
	if (volkInitialize() != VK_SUCCESS) return false;

	_allocator = {
		.pUserData = this,
		.pfnAllocation = [](void* userData, size_t size, size_t alignment, VkSystemAllocationScope scope)
		{
			return GetVgAllocator()->alloc(GetVgAllocator()->user_data, size, alignment);
		},
		.pfnReallocation = [](void* userData, void* original, size_t size, size_t alignment, VkSystemAllocationScope scope)
		{
			return GetVgAllocator()->realloc(GetVgAllocator()->user_data, original, size, alignment);
		},
		.pfnFree = [](void* userData, void* memory)
		{
			GetVgAllocator()->free(GetVgAllocator()->user_data, memory);
		},
		.pfnInternalAllocation = [](void* userData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope)
		{},
		.pfnInternalFree = [](void* userData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope)
		{}
	};

	vkb::InstanceBuilder builder;
	auto instance = builder.set_app_name(_applicationName.c_str())
		.set_engine_name(_engineName.c_str())
		.request_validation_layers(_requestValidation)
		.set_debug_callback_user_data_pointer(this)
		.set_debug_callback([](VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData) -> VkBool32
			{
				switch (messageSeverity)
				{
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
					LOG(DEBUG, "Vulkan: [{}] {}", MessageTypeToString(messageTypes), pCallbackData->pMessage);
					break;

				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
					LOG(INFO, "Vulkan: [{}] {}", MessageTypeToString(messageTypes), pCallbackData->pMessage);
					break;

				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
					LOG(WARN, "Vulkan: [{}] {}", MessageTypeToString(messageTypes), pCallbackData->pMessage);
					break;

				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
					LOG(ERROR, "Vulkan: [{}] {}", MessageTypeToString(messageTypes), pCallbackData->pMessage);
					break;
				}
				return VK_TRUE;
			})
		.require_api_version(1, 3, 0)
		.set_allocation_callbacks(&_allocator)
		.build();

	if (!instance.has_value())
	{
		return false;
	}
	_instance = instance.value();
	volkLoadInstanceOnly(_instance);
	_instance.fp_vkGetDeviceProcAddr = vkGetDeviceProcAddr;
	_instance.fp_vkGetInstanceProcAddr = vkGetInstanceProcAddr;

	_initialized = true;
	return true;
}

#endif