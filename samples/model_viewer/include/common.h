#pragma once

#include <array>
#include <cassert>
#include <string_view>
#include <varyag.hpp>

#ifndef NDEBUG
#define vgCheck(x) assert(static_cast<VgResult>(x) == VG_SUCCESS)
#else
#define vgCheck(x) do { x; } while (false)
#endif

template <class T, uint32_t NumFrames = 3>
struct PerFrameConstantBuffer
{
	vg::Buffer buffer;
	void* mapped;
	std::array<uint32_t, NumFrames> views;
	T data;

	PerFrameConstantBuffer(vg::Device device, std::string_view name)
	{
		uint32_t singleBufferSize = (sizeof(T) + 255) & ~255;
		vg::BufferDesc desc = { singleBufferSize * NumFrames, vg::BufferUsage::Constant, vg::HeapType::Upload };
		vg::BufferViewDesc viewDesc = { vg::BufferDescriptorType::Cbv, vg::BufferViewType::Buffer, vg::Format::Unknown,
			0, singleBufferSize, 0 };

		device.CreateBuffer(&desc, &buffer);
		buffer.SetName(name.data());
		buffer.Map(&mapped);
		for (uint64_t i = 0; i < views.size(); i++)
		{
			viewDesc.offset = singleBufferSize * i;
			buffer.CreateView(&viewDesc, &views[i]);
		}
	}

	void upload(uint64_t frameIndex)
	{
		memcpy(getDataPtr(frameIndex), &data, sizeof(data));
	}

	void destroy(vg::Device device)
	{
		device.DestroyBuffer(buffer);
	}

	vg::Buffer getBuffer() const { return buffer; }
	void* getDataPtr(uint64_t frameIndex) const { return static_cast<uint8_t*>(mapped) + ((sizeof(T) + 255) & ~255) * (frameIndex % views.size()); }
	uint32_t getView(uint64_t frameIndex) const { return views[frameIndex % views.size()]; }
};

namespace vg
{
	struct Library
	{
		Library(const vg::Config& cfg) { vg::Init(&cfg); }
		~Library() { vg::Shutdown(); }
	};

	template <class T> struct ObjectDestroyer
	{
		void operator()(T& obj) {}
	};

	template <> struct ObjectDestroyer<vg::Device>
	{
		void operator()(vg::Device& device)
		{
			vg::Adapter adapter;
			device.GetAdapter(&adapter);
			adapter.DestroyDevice(device);
		}
	};

	template <> struct ObjectDestroyer<vg::Buffer>
	{
		void operator()(vg::Buffer& buffer)
		{
			vg::Device device;
			buffer.GetDevice(&device);
			device.DestroyBuffer(buffer);
		}
	};

	template <> struct ObjectDestroyer<vg::Texture>
	{
		void operator()(vg::Texture& texture)
		{
			vg::Device device;
			texture.GetDevice(&device);
			device.DestroyTexture(texture);
		}
	};

	template <> struct ObjectDestroyer<vg::Pipeline>
	{
		void operator()(vg::Pipeline& pipeline)
		{
			vg::Device device;
			pipeline.GetDevice(&device);
			device.DestroyPipeline(pipeline);
		}
	};

	template <> struct ObjectDestroyer<vg::CommandPool>
	{
		void operator()(vg::CommandPool& pool)
		{
			vg::Device device;
			pool.GetDevice(&device);
			device.DestroyCommandPool(pool);
		}
	};

	template <> struct ObjectDestroyer<vg::CommandList>
	{
		void operator()(vg::CommandList& cmd)
		{
			vg::CommandPool pool;
			cmd.GetCommandPool(&pool);
			pool.FreeCommandList(cmd);
		}
	};

	template <> struct ObjectDestroyer<vg::SwapChain>
	{
		void operator()(vg::SwapChain& swapChain)
		{
			vg::Device device;
			swapChain.GetDevice(&device);
			device.DestroySwapChain(swapChain);
		}
	};

	template <class T>
	struct Ref
	{
		Ref() : _instance{ new Instance{} } {}
		Ref(std::nullptr_t) : _instance{ nullptr } {}
		Ref(T t) : _instance{ new Instance{t, 1} } {}
		Ref(const Ref<T>& other) : _instance(other._instance)
		{
			if (_instance) _instance->incRef();
		}
		Ref(Ref<T>&& other)
		{
			std::swap(_instance, other._instance);
		}
		~Ref()
		{
			if (_instance) _instance->decRef();
		}

		Ref& operator=(const Ref<T>& other)
		{
			if (_instance) _instance->decRef();
			_instance = other._instance;
			if (_instance) _instance->incRef();
			return *this;
		}

		T* operator->() const { return _instance ? &_instance->object : nullptr; }
		operator bool() const { return _instance && _instance->object; }
		T* operator&() const { return &_instance->object; }

		T operator*() const { return Get(); }
		T Get() const { return _instance ? _instance->object : nullptr; }

	private:
		struct Instance
		{
			T object{ nullptr };
			uint64_t references{ 1 };

			Instance() { object = nullptr; }
			~Instance()
			{
				if (!object) return;
				ObjectDestroyer<T>{}(object);
			}

			void incRef() { references++; }
			void decRef()
			{
				if (--references > 0) return;
				delete this;
			}
		};

		Instance* _instance{ nullptr };
	};
}

struct FrameData
{
	vg::Fence renderingFence;
	uint64_t fenceValue;
	vg::Ref<vg::CommandPool> graphicsCommandPool;
	vg::Ref<vg::CommandList> cmd;
	uint32_t swapChainAttachmentView;
	vg::Texture backBuffer;
};

struct MeshBindData
{
	float worldMatrix[16];
	float normalMatrix0[4];
	float normalMatrix1[4];
	float normalMatrix2[4];
	uint32_t cameraData;
	uint32_t material;
};