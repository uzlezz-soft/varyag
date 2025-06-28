#include "d3d12device.h"
#include "../common.h"
#include <array>
#include <string_view>

#if VG_D3D12_SUPPORTED

#include <agilitysdk/d3dx12/d3dx12.h>

#include "d3d12adapter.h"
#include "d3d12descriptor_manager.h"
#include "d3d12commands.h"
#include "d3d12buffer.h"
#include "d3d12shader_module.h"
#include "d3d12pipeline.h"
#include "d3d12swap_chain.h"
#include "d3d12texture.h"

#include "shaders/DrawIdEmulationCopyCS.h"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

#pragma comment(lib, "dxcore.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

constexpr std::string_view ShaderModelToString(D3D_SHADER_MODEL model)
{
    switch (model)
    {
    case D3D_SHADER_MODEL_5_1: return "5.1";
    case D3D_SHADER_MODEL_6_0: return "6.0";
    case D3D_SHADER_MODEL_6_1: return "6.1";
    case D3D_SHADER_MODEL_6_2: return "6.2";
    case D3D_SHADER_MODEL_6_3: return "6.3";
    case D3D_SHADER_MODEL_6_4: return "6.4";
    case D3D_SHADER_MODEL_6_5: return "6.5";
    case D3D_SHADER_MODEL_6_6: return "6.6";
    case D3D_SHADER_MODEL_6_7: return "6.7";
    case D3D_SHADER_MODEL_6_8: return "6.8";
    case D3D_SHADER_MODEL_6_9: return "6.9";
    default: return "[Unknown]";
    }
}

constexpr std::string_view FeatureLevelToString(D3D_FEATURE_LEVEL level)
{
    switch (level)
    {
    case D3D_FEATURE_LEVEL_1_0_CORE: return "1.0 core";
    case D3D_FEATURE_LEVEL_9_1: return "9.1";
    case D3D_FEATURE_LEVEL_9_2: return "9.2";
    case D3D_FEATURE_LEVEL_9_3: return "9.3";
    case D3D_FEATURE_LEVEL_10_0: return "10.0";
    case D3D_FEATURE_LEVEL_10_1: return "10.1";
    case D3D_FEATURE_LEVEL_11_0: return "11.0";
    case D3D_FEATURE_LEVEL_11_1: return "11.1";
    case D3D_FEATURE_LEVEL_12_0: return "12.0";
    case D3D_FEATURE_LEVEL_12_1: return "12.1";
    case D3D_FEATURE_LEVEL_12_2: return "12.2";
    default: return "[Unknown]";
    }
}

constexpr std::string_view ResourceBindingTierToString(D3D12_RESOURCE_BINDING_TIER tier)
{
    switch (tier)
    {
    case D3D12_RESOURCE_BINDING_TIER_1: return "Tier 1";
    case D3D12_RESOURCE_BINDING_TIER_2: return "Tier 2";
    case D3D12_RESOURCE_BINDING_TIER_3: return "Tier 3";
    default: return "[Unknown]";
    }
}

constexpr std::string_view MeshShaderTierToString(D3D12_MESH_SHADER_TIER tier)
{
    switch (tier)
    {
    case D3D12_MESH_SHADER_TIER_NOT_SUPPORTED: return "[mesh shaders not supported]";
    case D3D12_MESH_SHADER_TIER_1: return "Tier 1";
    default: return "[Unknown]";
    }
}

constexpr std::string_view RaytacingTierToString(D3D12_RAYTRACING_TIER tier)
{
    switch (tier)
    {
    case D3D12_RAYTRACING_TIER_NOT_SUPPORTED: return "[raytracing not supported]";
    case D3D12_RAYTRACING_TIER_1_0: return "Tier 1.0";
    case D3D12_RAYTRACING_TIER_1_1: return "Tier 1.1";
    default: return "[Unknown]";
    }
}

constexpr std::string_view ExecuteIndirectTierToString(D3D12_EXECUTE_INDIRECT_TIER tier)
{
    switch (tier)
    {
    case D3D12_EXECUTE_INDIRECT_TIER_1_0: return "Tier 1.0";
    case D3D12_EXECUTE_INDIRECT_TIER_1_1: return "Tier 1.1";
    default: return "[Unknown]";
    }
}

static vg::String ConvertWCharToString(const wchar_t* wstr)
{
    if (!wstr) return {};
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0)
        return {};

    vg::String result(sizeNeeded, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result.data(), sizeNeeded, nullptr, nullptr);
    result.pop_back();
    return result;
}

static void PrintAdapters()
{
    ComPtr<IDXGIFactory6> dxgiFactory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory)))) {
        return;
    }

    // Enumerate adapters
    ComPtr<IDXGIAdapter1> adapter;
    UINT adapterIndex = 0;
    while (dxgiFactory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND) {
        DXGI_ADAPTER_DESC1 adapterDesc;
        adapter->GetDesc1(&adapterDesc);
        std::string_view shaderModel;

        // Create a D3D12 device for the adapter
        ComPtr<ID3D12Device> device;
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))))
        {
            CD3DX12FeatureSupport features;
            features.Init(device.Get());
            
            auto fl = features.MaxSupportedFeatureLevel();
            auto sm = features.HighestShaderModel();
            auto rbt = features.ResourceBindingTier();
            auto ms = features.MeshShaderTier();
            auto rt = features.RaytracingTier();
            auto uploadHeap = features.GPUUploadHeapSupported();
            auto maxSamplers = features.MaxSamplerDescriptorHeapSize();
            auto ei = features.ExecuteIndirectTier();
            auto eb = features.EnhancedBarriersSupported();
            
            LOG(DEBUG, "Adapter {}: {} | FL {}, SM {}, RBT {}, MS {}, RT {}, GPU UPLOAD {}, MAX SAMPLERS {}, EI {}, EB {}", adapterIndex, ConvertWCharToString(adapterDesc.Description),
                FeatureLevelToString(fl), ShaderModelToString(sm), ResourceBindingTierToString(rbt),
                MeshShaderTierToString(ms), RaytacingTierToString(rt), static_cast<bool>(uploadHeap),
                maxSamplers, ExecuteIndirectTierToString(ei), static_cast<bool>(eb));
        }
        else
            LOG(DEBUG, "Failed to create D3D12 device (adapter {})", adapterIndex);

        adapter.Reset();
        adapterIndex++;
    }
}

D3D12Device::D3D12Device(D3D12Adapter& adapter, VgInitFlags initFlags)
    : _adapter(&adapter), _commandSignatureManager(*this)
{
    if (initFlags & VG_INIT_DEBUG)
    {
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&_debugInterface))))
        {
            _debugInterface->EnableDebugLayer();
            //_debugInterface->SetEnableGPUBasedValidation(TRUE);
            _debugInterface->SetEnableSynchronizedCommandQueueValidation(TRUE);
            _debugInterface->SetEnableAutoName(TRUE);
        }
    }

    ThrowOnErrorMsg(D3D12CreateDevice(_adapter->Adapter().Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&_baseDevice)),
        "Failed to create D3D12 device.");
    ThrowOnErrorMsg(_baseDevice->QueryInterface(IID_PPV_ARGS(&_device)), "Failed to create D3D12 device.");

    D3D12_FEATURE_DATA_SHADER_MODEL shaderModelFeature = {D3D_SHADER_MODEL_6_6};
    ThrowOnErrorMsg(_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModelFeature, sizeof(shaderModelFeature)),
        "This device does not support Shader Model 6.6");
    Assert(shaderModelFeature.HighestShaderModel >= D3D_SHADER_MODEL_6_6);

    _device->SetName(L"Varyag D3D12 Device");
    if (initFlags & VG_INIT_DEBUG)
    {
        ComPtr<ID3D12InfoQueue> infoQueue;
        if (SUCCEEDED(_device.As(&infoQueue)))
        {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);

            std::array<D3D12_MESSAGE_SEVERITY, 1> ignoreMessageSeverities
            {
                D3D12_MESSAGE_SEVERITY_INFO
            };
            std::array<D3D12_MESSAGE_ID, 2> ignoreMessageIDs
            {
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE
            };

            D3D12_INFO_QUEUE_FILTER infoQueueFilter
            {
                .DenyList
                {
                    .NumSeverities = static_cast<UINT>(ignoreMessageSeverities.size()),
                    .pSeverityList = ignoreMessageSeverities.data(),
                    .NumIDs = static_cast<UINT>(ignoreMessageIDs.size()),
                    .pIDList = ignoreMessageIDs.data()
                },
            };

            ThrowOnError(infoQueue->PushStorageFilter(&infoQueueFilter));
            ThrowOnError(_device->QueryInterface(IID_PPV_ARGS(&_debugDevice)));
        }
    }

    _allocationCallbacks = {
        .pAllocate = [](size_t size, size_t alignment, void* allocator) {
            return static_cast<VgAllocator*>(allocator)->alloc(static_cast<VgAllocator*>(allocator)->user_data, size, alignment);
        },
        .pFree = [](void* memory, void* allocator) {
            static_cast<VgAllocator*>(allocator)->free(static_cast<VgAllocator*>(allocator)->user_data, memory);
        },
        .pPrivateData = GetVgAllocator()
    };

    D3D12MA::ALLOCATOR_DESC allocatorDesc = {
        .Flags = D3D12MA::ALLOCATOR_FLAGS::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED
            | D3D12MA::ALLOCATOR_FLAGS::ALLOCATOR_FLAG_SINGLETHREADED,
        .pDevice = _device.Get(),
        .PreferredBlockSize = 0,
        .pAllocationCallbacks = &_allocationCallbacks,
        .pAdapter = _adapter->Adapter().Get()
    };
    ThrowOnError(D3D12MA::CreateAllocator(&allocatorDesc, &_allocator));

    D3D12_COMMAND_QUEUE_DESC queue = {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
        .Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
        .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
        .NodeMask = 0
    };
    ThrowOnError(_device->CreateCommandQueue(&queue, IID_PPV_ARGS(&_graphicsQueue)));
    queue.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    ThrowOnError(_device->CreateCommandQueue(&queue, IID_PPV_ARGS(&_computeQueue)));
    queue.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    ThrowOnError(_device->CreateCommandQueue(&queue, IID_PPV_ARGS(&_transferQueue)));

    _graphicsQueue->SetName(L"Graphics Queue");
    _computeQueue->SetName(L"Compute Queue");
    _transferQueue->SetName(L"Transfer Queue");

    InitDescriptorManagement();

    CD3DX12FeatureSupport features;
    features.Init(_device.Get());
    _executeIndirectTier = features.ExecuteIndirectTier();

    if (_executeIndirectTier >= D3D12_EXECUTE_INDIRECT_TIER_1_1) _utilityPipelines.DrawIdEmulationComputeShader = nullptr;
    else
    {
        D3D12_SHADER_BYTECODE bytecode = {
            .pShaderBytecode = DrawIdEmulationCopyCS,
            .BytecodeLength = sizeof(DrawIdEmulationCopyCS) / sizeof(DrawIdEmulationCopyCS[0])
        };

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
        .pRootSignature = GetComputeRootSignature(),
        .CS = bytecode,
        .NodeMask = NodeMask(),
        .CachedPSO = {
            .pCachedBlob = nullptr,
            .CachedBlobSizeInBytes = 0
        },
        .Flags = D3D12_PIPELINE_STATE_FLAG_NONE
        };
        ThrowOnError(_device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&_utilityPipelines.DrawIdEmulationComputeShader)));
        _utilityPipelines.DrawIdEmulationComputeShader->SetName(L"[Internal Compute Pipeline | emulating DrawId]");
    }

    LOG(INFO, "Created D3D12 device");
}

D3D12Device::~D3D12Device()
{
    GetAllocator().Delete(_descriptorManager);
}

VgAdapter D3D12Device::Adapter() const
{
    return _adapter;
}

VgCommandPool D3D12Device::CreateCommandPool(VgCommandPoolFlags flags, VgQueue queue)
{
    return new(GetAllocator().Allocate<D3D12CommandPool>()) D3D12CommandPool(*this, flags, queue);
}

void D3D12Device::DestroyCommandPool(VgCommandPool pool)
{
    GetAllocator().Delete(pool);
}

VgBuffer D3D12Device::CreateBuffer(const VgBufferDesc& desc)
{
    return new(GetAllocator().Allocate<D3D12Buffer>()) D3D12Buffer(*this, desc);
}

void D3D12Device::DestroyBuffer(VgBuffer buffer)
{
    GetAllocator().Delete(buffer);
}

VgShaderModule D3D12Device::CreateShaderModule(const void* data, uint64_t size)
{
    return new(GetAllocator().Allocate<D3D12ShaderModule>()) D3D12ShaderModule(*this, data, size);
}

void D3D12Device::DestroyShaderModule(VgShaderModule module)
{
    GetAllocator().Delete(module);
}

VgPipeline D3D12Device::CreateGraphicsPipeline(const VgGraphicsPipelineDesc& desc)
{
    return new(GetAllocator().Allocate<D3D12GraphicsPipeline>()) D3D12GraphicsPipeline(*this, desc);
}

VgPipeline D3D12Device::CreateComputePipeline(VgShaderModule shaderModule)
{
    return new(GetAllocator().Allocate<D3D12ComputePipeline>()) D3D12ComputePipeline(*this, shaderModule);
}

void D3D12Device::DestroyPipeline(VgPipeline pipeline)
{
    GetAllocator().Delete(pipeline);
}

VgFence D3D12Device::CreateFence(uint64_t initialValue)
{
    std::unique_lock lock(_fenceMutex);

    ID3D12Fence* fence;
    ThrowOnError(_device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    HANDLE event = CreateEvent(nullptr, false, false, nullptr);
    void* fenceHandle = reinterpret_cast<void*>(_nextFenceIndex++);
    _fences[fenceHandle] = { fence, event };
    return fenceHandle;
}

void D3D12Device::DestroyFence(VgFence fence)
{
    std::unique_lock lock(_fenceMutex);

    auto data = _fences[fence];
    data.Fence->Release();
    CloseHandle(data.Event);
    _fences.erase(fence);
}

constexpr D3D12_FILTER FormD3D12Filter(VgFilter min_filter, VgFilter mag_filter, VgMipmapMode mipmap_mode,
    bool comparison = false, bool anisotropic = false, VgReductionMode reduction_mode = VG_REDUCTION_MODE_DEFAULT)
{
    int filter = 0;

    // Min filter contributes bits 4 and 3
    filter |= (min_filter << 4);
    // Mag filter contributes bit 2
    filter |= (mag_filter << 2);
    // Mip filter contributes bit 1
    filter |= mipmap_mode;

    // Comparison filter flag (bit 7)
    if (comparison) filter |= 0x80;
    // Anisotropic filter flag (bit 5 and needs mag=min=linear, mip=linear)
    if (anisotropic) filter |= 0x55; // Must ensure min=linear, mag=linear, mip=linear for anisotropic

    if (reduction_mode == VG_REDUCTION_MODE_MIN) filter |= 0x100;
    else if (reduction_mode == VG_REDUCTION_MODE_MAX) filter |= 0x180;
    return static_cast<D3D12_FILTER>(filter);
}

static D3D12_SAMPLER_DESC ConvertToD3D12SamplerDesc(const VgSamplerDesc& desc) {
    D3D12_SAMPLER_DESC d3d12Desc = {};
    d3d12Desc.Filter = FormD3D12Filter(desc.min_filter, desc.mag_filter, desc.mipmap_mode,
        desc.comparison_func != VG_COMPARISON_FUNC_NONE, desc.max_anisotropy > VG_ANISOTROPY_1, desc.reduction_mode);

    auto mapAddressMode = [](VgAddressMode mode) -> D3D12_TEXTURE_ADDRESS_MODE {
        switch (mode) {
        case VG_ADDRESS_MODE_REPEAT:          return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case VG_ADDRESS_MODE_MIRRORED_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case VG_ADDRESS_MODE_CLAMP_TO_EDGE:   return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case VG_ADDRESS_MODE_CLAMP_TO_BORDER: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case VG_ADDRESS_MODE_MIRROR_ONCE:     return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
        default:                               return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        }
        };
    d3d12Desc.AddressU = mapAddressMode(desc.address_u);
    d3d12Desc.AddressV = mapAddressMode(desc.address_v);
    d3d12Desc.AddressW = mapAddressMode(desc.address_w);

    d3d12Desc.MipLODBias = desc.mip_lod_bias;
    d3d12Desc.MaxAnisotropy = 1u << desc.max_anisotropy;

    // Map comparison functions
    auto mapComparisonFunc = [](VgComparisonFunc func) -> D3D12_COMPARISON_FUNC {
        switch (func) {
        case VG_COMPARISON_FUNC_NEVER:        return D3D12_COMPARISON_FUNC_NEVER;
        case VG_COMPARISON_FUNC_LESS:         return D3D12_COMPARISON_FUNC_LESS;
        case VG_COMPARISON_FUNC_EQUAL:        return D3D12_COMPARISON_FUNC_EQUAL;
        case VG_COMPARISON_FUNC_LESS_EQUAL:   return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case VG_COMPARISON_FUNC_GREATER:      return D3D12_COMPARISON_FUNC_GREATER;
        case VG_COMPARISON_FUNC_NOT_EQUAL:    return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case VG_COMPARISON_FUNC_GREATER_EQUAL:return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case VG_COMPARISON_FUNC_ALWAYS:       return D3D12_COMPARISON_FUNC_ALWAYS;
        default:                               return D3D12_COMPARISON_FUNC_NONE;
        }
    };

    d3d12Desc.ComparisonFunc = mapComparisonFunc(desc.comparison_func);
    d3d12Desc.BorderColor[0] = desc.border_color[0];
    d3d12Desc.BorderColor[1] = desc.border_color[1];
    d3d12Desc.BorderColor[2] = desc.border_color[2];
    d3d12Desc.BorderColor[3] = desc.border_color[3];
    d3d12Desc.MinLOD = desc.min_lod;
    d3d12Desc.MaxLOD = desc.max_lod;

    return d3d12Desc;
}

VgSampler D3D12Device::CreateSampler(const VgSamplerDesc& desc)
{
    auto samplerDesc = ConvertToD3D12SamplerDesc(desc);
    auto index = _descriptorManager->SamplerHeap().RequestDescriptor();
    _device->CreateSampler(&samplerDesc, _descriptorManager->SamplerHeap().GetCPUDescriptorHandle(index));
    return reinterpret_cast<void*>(static_cast<uintptr_t>(index + 1));
}

void D3D12Device::DestroySampler(VgSampler sampler)
{
    auto index = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(sampler)) - 1;
    _descriptorManager->SamplerHeap().FreeDescriptor(index);
}

VgTexture D3D12Device::CreateTexture(const VgTextureDesc& desc)
{
    return new(GetAllocator().Allocate<D3D12Texture>()) D3D12Texture(*this, desc);
}

void D3D12Device::DestroyTexture(VgTexture texture)
{
    GetAllocator().Delete(texture);
}

VgSwapChain D3D12Device::CreateSwapChain(const VgSwapChainDesc& desc)
{
    return new(GetAllocator().Allocate<D3D12SwapChain>()) D3D12SwapChain(*this, desc);
}

void D3D12Device::DestroySwapChain(VgSwapChain swapChain)
{
    GetAllocator().Delete(swapChain);
}

void D3D12Device::WaitQueueIdle(VgQueue queue)
{
    ComPtr<ID3D12Fence> fence;
    _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    HANDLE fenceEvent = CreateEvent(nullptr, false, false, nullptr);
    GetQueue(queue)->Signal(fence.Get(), 1);
    if (fence->GetCompletedValue() < 1)
    {
        fence->SetEventOnCompletion(1, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
    CloseHandle(fenceEvent);
}

void D3D12Device::WaitIdle()
{
    WaitQueueIdle(VG_QUEUE_GRAPHICS);
    WaitQueueIdle(VG_QUEUE_COMPUTE);
    WaitQueueIdle(VG_QUEUE_TRANSFER);
}

void D3D12Device::SignalFence(VgFence fence, uint64_t value)
{
    std::unique_lock lock(_fenceMutex);
    ThrowOnError(_fences[fence].Fence->Signal(value));
}

void D3D12Device::WaitFence(VgFence fence, uint64_t value)
{
    std::unique_lock lock(_fenceMutex);

    const auto& data = _fences[fence];
    if (data.Fence->GetCompletedValue() < value)
    {
        data.Fence->SetEventOnCompletion(value, data.Event);
        WaitForSingleObject(data.Event, INFINITE);
    }
}

uint64_t D3D12Device::GetFenceValue(VgFence fence)
{
    return reinterpret_cast<ID3D12Fence*>(fence)->GetCompletedValue();
}

void D3D12Device::SubmitCommandLists(uint32_t numSubmits, const VgSubmitInfo* submits)
{
    const auto submit = [&](ID3D12CommandQueue* queue, const VgSubmitInfo& info, vg::Vector<ID3D12CommandList*> cmd)
        {
            if (cmd.empty()) return;
            for (uint32_t i = 0; i < info.num_wait_fences; i++)
            {
                queue->Wait(_fences[info.wait_fences[i].fence].Fence, info.wait_fences[i].value);
                //queue->Wait(static_cast<ID3D12Fence*>(info.wait_fences[i].fence), info.wait_fences[i].value);
            }
            queue->ExecuteCommandLists(static_cast<UINT>(cmd.size()), cmd.data());
            for (uint32_t i = 0; i < info.num_signal_fences; i++)
            {
                queue->Signal(_fences[info.signal_fences[i].fence].Fence, info.signal_fences[i].value);
                //queue->Signal(static_cast<ID3D12Fence*>(info.signal_fences[i].fence), info.signal_fences[i].value);
            }
        };

    // Do it better if I want thread_local
    // Because right now it uses the allocator after the program exits
    /*thread_local*/ vg::Vector<ID3D12CommandList*> directLists;
    /*thread_local*/ vg::Vector<ID3D12CommandList*> computeLists;
    /*thread_local*/ vg::Vector<ID3D12CommandList*> copyLists;
    for (uint32_t i = 0; i < numSubmits; i++)
    {
        const VgSubmitInfo& info = submits[i];

        directLists.clear();
        computeLists.clear();
        copyLists.clear();

        for (uint32_t i = 0; i < info.num_command_lists; i++)
        {
            auto cmd = static_cast<D3D12CommandList*>(info.command_lists[i]);
            switch (cmd->CommandPool()->Queue())
            {
            case VG_QUEUE_GRAPHICS:
                directLists.push_back(cmd->Cmd());
                break;

            case VG_QUEUE_COMPUTE:
                computeLists.push_back(cmd->Cmd());
                break;

            case VG_QUEUE_TRANSFER:
                copyLists.push_back(cmd->Cmd());
                break;
            }
        }

        {
            std::unique_lock lock(_fenceMutex);
            submit(_graphicsQueue.Get(), info, directLists);
            submit(_computeQueue.Get(), info, computeLists);
            submit(_transferQueue.Get(), info, copyLists);
        }
    }
}

uint32_t D3D12Device::GetSamplerIndex(VgSampler sampler)
{
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(sampler)) - 1;
}

void D3D12Device::InitDescriptorManagement()
{
    _descriptorManager = new (GetAllocator().Allocate<D3D12DescriptorManager>()) D3D12DescriptorManager(*this);

    const auto space = 1;
    std::array<CD3DX12_DESCRIPTOR_RANGE1, 3> resourceRanges;
    resourceRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, D3D12DescriptorManager::NumResourceDescriptors,
        0, space, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);
    resourceRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, D3D12DescriptorManager::NumResourceDescriptors,
        0, space, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);
    resourceRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, D3D12DescriptorManager::NumResourceDescriptors,
        0, space, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);
    
    CD3DX12_DESCRIPTOR_RANGE1 samplerRange;
    samplerRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, D3D12DescriptorManager::NumSamplerDescriptors,
        0, 2, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);

    std::array<CD3DX12_ROOT_PARAMETER1, 4> parameters;
    parameters[0].InitAsConstants(32, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
    parameters[1].InitAsConstants(1, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    parameters[2].InitAsDescriptorTable(static_cast<UINT>(resourceRanges.size()), resourceRanges.data(), D3D12_SHADER_VISIBILITY_ALL);
    parameters[3].InitAsDescriptorTable(1, &samplerRange, D3D12_SHADER_VISIBILITY_ALL);

    std::array<D3D12_STATIC_SAMPLER_DESC, static_samplers.size()> rootSignatureStaticSamplers;
    for (uint32_t i = 0; i < static_samplers.size(); i++)
    {
        auto desc = ConvertToD3D12SamplerDesc(static_samplers[i]);
        
        rootSignatureStaticSamplers[i] = {
            .Filter = desc.Filter,
            .AddressU = desc.AddressU,
            .AddressV = desc.AddressV,
            .AddressW = desc.AddressW,
            .MipLODBias = desc.MipLODBias,
            .MaxAnisotropy = desc.MaxAnisotropy,
            .ComparisonFunc = desc.ComparisonFunc,
            .BorderColor = (desc.BorderColor[0] == 0.0f && desc.BorderColor[1] == 0.0f && desc.BorderColor[2] == 0.0f) ?
                (desc.BorderColor[3] == 0.0f ? D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK : D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
                : D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
            .MinLOD = desc.MinLOD,
            .MaxLOD = desc.MaxLOD,
            .ShaderRegister = i,
            .RegisterSpace = 0,
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        };
    }

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {
        .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
        .Desc_1_1 = {
            .NumParameters = static_cast<UINT>(parameters.size()),
            .pParameters = parameters.data(),
            .NumStaticSamplers = static_cast<UINT>(rootSignatureStaticSamplers.size()),
            .pStaticSamplers = rootSignatureStaticSamplers.data(),
            .Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
            | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED
        }
    };
    ComPtr<ID3DBlob> serializedRootSignature;
    ComPtr<ID3DBlob> errorBlob;
    ThrowOnErrorWithBlob(D3D12SerializeVersionedRootSignature(&desc, &serializedRootSignature, &errorBlob), errorBlob);
    ThrowOnError(_device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(),
        serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(&_computeRootSignature)));
    
    desc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ThrowOnErrorWithBlob(D3D12SerializeVersionedRootSignature(&desc, &serializedRootSignature, &errorBlob), errorBlob);
    ThrowOnError(_device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(),
        serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(&_graphicsRootSignature)));

    _computeRootSignature->SetName(L"Compute Root Signature");
    _graphicsRootSignature->SetName(L"Graphics Root Signature");
}

D3D12IndirectCommandSignatureManager::D3D12IndirectCommandSignatureManager(D3D12Device& device)
    : _device(&device)
{
}

D3D12IndirectCommandSignatureManager::~D3D12IndirectCommandSignatureManager()
{
}

template <D3D12_INDIRECT_ARGUMENT_TYPE ArgType>
static ComPtr<ID3D12CommandSignature> GetOrCreateCommandSignature(D3D12Device& device, uint32_t stride,
    vg::UnorderedMap<uint32_t, ComPtr<ID3D12CommandSignature>>& map, std::mutex& mutex)
{
    std::scoped_lock lock(mutex);

    const auto it = map.find(stride);
    if (it != map.end()) return it->second;

    if (device.GetExecuteIndirectTier() >= D3D12_EXECUTE_INDIRECT_TIER_1_1)
    {
        std::array<D3D12_INDIRECT_ARGUMENT_DESC, 2> args;
        // Emulating gl_DrawID
        args[0] = {
            .Type = D3D12_INDIRECT_ARGUMENT_TYPE_INCREMENTING_CONSTANT,
            .IncrementingConstant = {
                .RootParameterIndex = 1,
                .DestOffsetIn32BitValues = 0
            }
        };
        args[1] = { .Type = ArgType };

        D3D12_COMMAND_SIGNATURE_DESC desc = {
            .ByteStride = stride,
            .NumArgumentDescs = 1,
            .pArgumentDescs = &args.back(),
            .NodeMask = device.NodeMask()
        };

        ID3D12RootSignature* rootSignature = nullptr;
        if constexpr (ArgType == D3D12_INDIRECT_ARGUMENT_TYPE_DRAW || ArgType == D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED)
        {
            rootSignature = device.GetGraphicsRootSignature();
            desc.NumArgumentDescs = static_cast<UINT>(args.size());
            desc.pArgumentDescs = args.data();
        }

        ComPtr<ID3D12CommandSignature> signature;
        ThrowOnError(device.Device()->CreateCommandSignature(&desc, rootSignature, IID_PPV_ARGS(&signature)));
        map[stride] = signature;
        return signature;
    }
    else
    {
        //D3D12_INDIRECT_ARGUMENT_DESC arg = { .Type = ArgType };
        std::array<D3D12_INDIRECT_ARGUMENT_DESC, 2> args;
        args[0] = {
            .Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
            .Constant = {
                .RootParameterIndex = 1,
                .DestOffsetIn32BitValues = 0,
                .Num32BitValuesToSet = 1
            }
        };
        args[1] = { .Type = ArgType };

        D3D12_COMMAND_SIGNATURE_DESC desc = {
            .ByteStride = stride,
            .NumArgumentDescs = 1,
            .pArgumentDescs = &args.back(),
            .NodeMask = device.NodeMask()
        };

        ID3D12RootSignature* rootSignature = nullptr;
        if constexpr (ArgType == D3D12_INDIRECT_ARGUMENT_TYPE_DRAW || ArgType == D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED)
        {
            rootSignature = device.GetGraphicsRootSignature();
            desc.ByteStride = (ArgType == D3D12_INDIRECT_ARGUMENT_TYPE_DRAW ? sizeof(VgDrawIndirectCommand) : sizeof(VgDrawIndexedIndirectCommand)) + sizeof(uint32_t);
            desc.NumArgumentDescs = static_cast<UINT>(args.size());
            desc.pArgumentDescs = args.data();
        }

        ComPtr<ID3D12CommandSignature> signature;
        ThrowOnError(device.Device()->CreateCommandSignature(&desc, rootSignature, IID_PPV_ARGS(&signature)));
        map[stride] = signature;
        return signature;
    }
}

ComPtr<ID3D12CommandSignature> D3D12IndirectCommandSignatureManager::GetDrawCommandSignature(uint32_t stride)
{
    return GetOrCreateCommandSignature<D3D12_INDIRECT_ARGUMENT_TYPE_DRAW>(*_device, stride, _drawCommandSignatures, _drawMutex);
}

ComPtr<ID3D12CommandSignature> D3D12IndirectCommandSignatureManager::GetDrawIndexedCommandSignature(uint32_t stride)
{
    return GetOrCreateCommandSignature<D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED>(*_device, stride, _drawIndexedCommandSignatures, _drawIndexedMutex);
}

ComPtr<ID3D12CommandSignature> D3D12IndirectCommandSignatureManager::GetDispatchCommandSignature(uint32_t stride)
{
    return GetOrCreateCommandSignature<D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH>(*_device, stride, _dispatchCommandSignatures, _dispatchMutex);
}

#endif