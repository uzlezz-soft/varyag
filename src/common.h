#pragma once

#include "varyag.h"
#include <set>
#include <array>
#include <format>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <unordered_map>
#include <unordered_set>

extern bool debug;
extern VgMessageCallbackPFN messageCallback;

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

#define LOG(severity, msg, ...) do { \
if (messageCallback) \
{ \
	if constexpr (CONCAT(VG_MESSAGE_SEVERITY_, severity) == VG_MESSAGE_SEVERITY_DEBUG) \
	{ \
		if (debug) \
			messageCallback(CONCAT(VG_MESSAGE_SEVERITY_, severity), std::format(msg, ##__VA_ARGS__).c_str()); \
	} else \
		messageCallback(CONCAT(VG_MESSAGE_SEVERITY_, severity), std::format(msg, ##__VA_ARGS__).c_str()); \
} \
} while (false)

#define Assert(x) do { \
if (!(x)) \
	throw std::runtime_error(std::format("`{}` failed. File {}, Line {}", #x, __FILE__, __LINE__)); \
} while (false)

#define AssertMsg(x, msg, ...) do { \
if (!(x)) \
	throw std::runtime_error(std::format("`{}` failed: {}", #x, std::format(msg, ##__VA_ARGS__))); \
} while (false)

VgAllocator* GetVgAllocator();

namespace vg
{
    template <class T>
    class Allocator
    {
    public:
        typedef T value_type;

        Allocator() = default;
        template <class U>
        constexpr Allocator(const Allocator<U>&) noexcept {}

        [[nodiscard]] T* allocate(std::size_t n)
        {
            return static_cast<T*>(GetVgAllocator()->alloc(GetVgAllocator()->user_data, n * sizeof(T), alignof(T)));
        }

        void deallocate(T* p, std::size_t) noexcept
        {
            GetVgAllocator()->free(GetVgAllocator()->user_data, p);
        }
    };

    template <typename T, typename U>
    bool operator==(const Allocator<T>& lhs, const Allocator<U>& rhs) noexcept
    {
        return true;
    }

    template <typename T, typename U>
    bool operator!=(const Allocator<T>& lhs, const Allocator<U>& rhs) noexcept
    {
        return false;
    }

    using String = std::basic_string<char, std::char_traits<char>, vg::Allocator<char>>;
    using WString = std::basic_string<wchar_t, std::char_traits<wchar_t>, vg::Allocator<wchar_t>>;

    template <class T>
    using Vector = std::vector<T, vg::Allocator<T>>;

    template <class K, class V>
    using UnorderedMap = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, vg::Allocator<std::pair<const K, V>>>;

    template <class T>
    using Set = std::set<T, std::less<T>, vg::Allocator<T>>;

    template <class T, class Hash = std::hash<T>, class Equal = std::equal_to<T>, class Alloc = vg::Allocator<T>>
    using UnorderedSet = std::unordered_set<T, Hash, Equal, Alloc>;

    template <class K, class V>
    using UnorderedMultimap = std::unordered_multimap<K, V, std::hash<K>, std::equal_to<K>, vg::Allocator<std::pair<const K, V>>>;

    using StringStream = std::basic_stringstream<char, std::char_traits<char>, vg::Allocator<char>>;

    struct AllocatorWrapper
    {
        VgAllocator* allocator;

        template <class T>
        void* Allocate() const { return allocator->alloc(allocator->user_data, sizeof(T), alignof(T)); }
        template <class T>
        void Delete(T* obj)
        {
            obj->~T();
            allocator->free(allocator->user_data, obj);
        }
    };
}

static vg::AllocatorWrapper GetAllocator() { return { GetVgAllocator() }; }

// UPDATE EFormat
constexpr uint64_t FormatSizeBytes(VgFormat format)
{
    switch (format)
    {
    case VG_FORMAT_R32G32B32A32_TYPELESS:
    case VG_FORMAT_R32G32B32A32_FLOAT:
    case VG_FORMAT_R32G32B32A32_UINT:
    case VG_FORMAT_R32G32B32A32_SINT:
        return 16;

    case VG_FORMAT_R32G32B32_TYPELESS:
    case VG_FORMAT_R32G32B32_FLOAT:
    case VG_FORMAT_R32G32B32_UINT:
    case VG_FORMAT_R32G32B32_SINT:
        return 12;

    case VG_FORMAT_R16G16B16A16_TYPELESS:
    case VG_FORMAT_R16G16B16A16_FLOAT:
    case VG_FORMAT_R16G16B16A16_UNORM:
    case VG_FORMAT_R16G16B16A16_UINT:
    case VG_FORMAT_R16G16B16A16_SNORM:
    case VG_FORMAT_R16G16B16A16_SINT:
        return 8;

    case VG_FORMAT_R8G8B8A8_TYPELESS:
    case VG_FORMAT_R8G8B8A8_UNORM:
    case VG_FORMAT_R8G8B8A8_SRGB:
    case VG_FORMAT_R8G8B8A8_UINT:
    case VG_FORMAT_R8G8B8A8_SNORM:
    case VG_FORMAT_R8G8B8A8_SINT:
    case VG_FORMAT_B8G8R8A8_TYPELESS:
    case VG_FORMAT_B8G8R8A8_UNORM:
    case VG_FORMAT_B8G8R8A8_SRGB:
        return 4;

    case VG_FORMAT_R32G32_TYPELESS:
    case VG_FORMAT_R32G32_FLOAT:
    case VG_FORMAT_R32G32_UINT:
    case VG_FORMAT_R32G32_SINT:
        return 8;

    case VG_FORMAT_R32_TYPELESS:
    case VG_FORMAT_D32_FLOAT:
    case VG_FORMAT_R32_FLOAT:
    case VG_FORMAT_R32_UINT:
    case VG_FORMAT_R32_SINT:
        return 4;

    case VG_FORMAT_R16G16_TYPELESS:
    case VG_FORMAT_R16G16_FLOAT:
    case VG_FORMAT_R16G16_UNORM:
    case VG_FORMAT_R16G16_UINT:
    case VG_FORMAT_R16G16_SNORM:
    case VG_FORMAT_R16G16_SINT:
        return 4;

    case VG_FORMAT_R8G8_TYPELESS:
    case VG_FORMAT_R8G8_UNORM:
    case VG_FORMAT_R8G8_UINT:
    case VG_FORMAT_R8G8_SNORM:
    case VG_FORMAT_R8G8_SINT:
        return 2;

    case VG_FORMAT_R16_TYPELESS:
    case VG_FORMAT_R16_FLOAT:
    case VG_FORMAT_D16_UNORM:
    case VG_FORMAT_R16_UNORM:
    case VG_FORMAT_R16_UINT:
    case VG_FORMAT_R16_SNORM:
    case VG_FORMAT_R16_SINT:
        return 2;

    case VG_FORMAT_R8_TYPELESS:
    case VG_FORMAT_R8_UNORM:
    case VG_FORMAT_R8_UINT:
    case VG_FORMAT_R8_SNORM:
    case VG_FORMAT_R8_SINT:
    case VG_FORMAT_A8_UNORM:
        return 1;

    case VG_FORMAT_R1_UNORM:
        return 1; // Special case, treated as 1 byte.

    case VG_FORMAT_R9G9B9E5_SHAREDEXP:
    case VG_FORMAT_R11G11B10_FLOAT:
    case VG_FORMAT_R10G10B10A2_TYPELESS:
    case VG_FORMAT_R10G10B10A2_UNORM:
    case VG_FORMAT_R10G10B10A2_UINT:
        return 4;

    case VG_FORMAT_B5G6R5_UNORM:
    case VG_FORMAT_B5G5R5A1_UNORM:
        return 2;

    default:
        return 0; // Unknown or unsupported format.
    }
}

// UPDATE EFormat
constexpr uint64_t GetBCFormatBlockSize(VgFormat format)
{
    switch (format)
    {
    case VG_FORMAT_BC1_TYPELESS:
    case VG_FORMAT_BC1_UNORM:
    case VG_FORMAT_BC1_SRGB:
        return 8;

    case VG_FORMAT_BC2_TYPELESS:
    case VG_FORMAT_BC2_UNORM:
    case VG_FORMAT_BC2_SRGB:
        return 16;

    case VG_FORMAT_BC3_TYPELESS:
    case VG_FORMAT_BC3_UNORM:
    case VG_FORMAT_BC3_SRGB:
        return 16;

    case VG_FORMAT_BC4_TYPELESS:
    case VG_FORMAT_BC4_UNORM:
    case VG_FORMAT_BC4_SNORM:
        return 8;

    case VG_FORMAT_BC5_TYPELESS:
    case VG_FORMAT_BC5_UNORM:
    case VG_FORMAT_BC5_SNORM:
        return 16;

    case VG_FORMAT_BC6H_TYPELESS:
    case VG_FORMAT_BC6H_UF16:
    case VG_FORMAT_BC6H_SF16:
        return 16;

    case VG_FORMAT_BC7_TYPELESS:
    case VG_FORMAT_BC7_UNORM:
    case VG_FORMAT_BC7_SRGB:
        return 16;

    default:
        return 0; // Non-BC formats or unknown formats.
    }
}

constexpr uint32_t SampleCount(VgSampleCount count)
{
    switch (count)
    {
    case VG_SAMPLE_COUNT_1: return 1;
    case VG_SAMPLE_COUNT_2: return 2;
    case VG_SAMPLE_COUNT_4: return 4;
    case VG_SAMPLE_COUNT_8: return 8;
    default: return 0;
    }
}

// UPDATE EFormat
constexpr bool FormatIsDepthStencil(VgFormat format)
{
    switch (format) {
        // Depth-stencil formats
    case VG_FORMAT_D32_FLOAT_S8X24_UINT:
    case VG_FORMAT_D24_UNORM_S8_UINT:

        // Depth-only formats
    case VG_FORMAT_D32_FLOAT:
    case VG_FORMAT_D16_UNORM:

        // Stencil-related formats (type-less depth-stencil formats)
    case VG_FORMAT_R32G8X24_TYPELESS:
    case VG_FORMAT_R24G8_TYPELESS:
    case VG_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case VG_FORMAT_X32_TYPELESS_G8X24_UINT:
    case VG_FORMAT_X24_TYPELESS_G8_UINT:
        return true;

    default: return false;
    }
}

// UPDATE EFormat
constexpr uint32_t FormatPlaneCount(VgFormat format)
{
    switch (format)
    {
    case VG_FORMAT_R32G8X24_TYPELESS:
    case VG_FORMAT_D32_FLOAT_S8X24_UINT:
    case VG_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case VG_FORMAT_X32_TYPELESS_G8X24_UINT:
    case VG_FORMAT_R24G8_TYPELESS:
    case VG_FORMAT_D24_UNORM_S8_UINT:
    case VG_FORMAT_R24_UNORM_X8_TYPELESS:
    case VG_FORMAT_X24_TYPELESS_G8_UINT:
        return 2;

    default: return 1;
    }
}

class VgError : public std::runtime_error {
public:
    VgResult result;
    VgError(VgResult result, const std::string& msg) : std::runtime_error(msg) {}
};

class VgFailure : public VgError {
public:
    VgFailure(const std::string& str) : VgError(VG_FAILURE, str) {}
};

class VgDeviceLostError : public VgError {
public:
    VgDeviceLostError(const std::string& str) : VgError(VG_DEVICE_LOST, std::string("device lost | ") + str) {}
};

constexpr std::array static_samplers = {
    // nearestClamp
    VgSamplerDesc { VG_FILTER_NEAREST, VG_FILTER_NEAREST, VG_MIPMAP_MODE_NEAREST,
    VG_ADDRESS_MODE_CLAMP_TO_EDGE, VG_ADDRESS_MODE_CLAMP_TO_EDGE, VG_ADDRESS_MODE_CLAMP_TO_EDGE,
    0.0f, VG_ANISOTROPY_1, VG_COMPARISON_FUNC_NONE, {0, 0, 0, 0}, 0.0f, 100.0f, VG_REDUCTION_MODE_DEFAULT },
    // nearestWrap
    VgSamplerDesc { VG_FILTER_NEAREST, VG_FILTER_NEAREST, VG_MIPMAP_MODE_NEAREST,
    VG_ADDRESS_MODE_REPEAT, VG_ADDRESS_MODE_REPEAT, VG_ADDRESS_MODE_REPEAT,
    0.0f, VG_ANISOTROPY_1, VG_COMPARISON_FUNC_NONE, {0, 0, 0, 0}, 0.0f, 100.0f, VG_REDUCTION_MODE_DEFAULT },
    // linearClamp
    VgSamplerDesc { VG_FILTER_LINEAR, VG_FILTER_LINEAR, VG_MIPMAP_MODE_LINEAR,
    VG_ADDRESS_MODE_CLAMP_TO_EDGE, VG_ADDRESS_MODE_CLAMP_TO_EDGE, VG_ADDRESS_MODE_CLAMP_TO_EDGE,
    0.0f, VG_ANISOTROPY_1, VG_COMPARISON_FUNC_NONE, {0, 0, 0, 0}, 0.0f, 100.0f, VG_REDUCTION_MODE_DEFAULT },
    // linearWrap
    VgSamplerDesc { VG_FILTER_LINEAR, VG_FILTER_LINEAR, VG_MIPMAP_MODE_LINEAR,
    VG_ADDRESS_MODE_REPEAT, VG_ADDRESS_MODE_REPEAT, VG_ADDRESS_MODE_REPEAT,
    0.0f, VG_ANISOTROPY_1, VG_COMPARISON_FUNC_NONE, {0, 0, 0, 0}, 0.0f, 100.0f, VG_REDUCTION_MODE_DEFAULT },
    // anisotropicClamp
    VgSamplerDesc { VG_FILTER_LINEAR, VG_FILTER_LINEAR, VG_MIPMAP_MODE_LINEAR,
    VG_ADDRESS_MODE_CLAMP_TO_EDGE, VG_ADDRESS_MODE_CLAMP_TO_EDGE, VG_ADDRESS_MODE_CLAMP_TO_EDGE,
    0.0f, VG_ANISOTROPY_16, VG_COMPARISON_FUNC_NONE, {0, 0, 0, 0}, 0.0f, 100.0f, VG_REDUCTION_MODE_DEFAULT },
    // anisotropicWrap
    VgSamplerDesc { VG_FILTER_LINEAR, VG_FILTER_LINEAR, VG_MIPMAP_MODE_LINEAR,
    VG_ADDRESS_MODE_REPEAT, VG_ADDRESS_MODE_REPEAT, VG_ADDRESS_MODE_REPEAT,
    0.0f, VG_ANISOTROPY_16, VG_COMPARISON_FUNC_NONE, {0, 0, 0, 0}, 0.0f, 100.0f, VG_REDUCTION_MODE_DEFAULT },
    // linearMipNearestClamp
    VgSamplerDesc { VG_FILTER_LINEAR, VG_FILTER_LINEAR, VG_MIPMAP_MODE_NEAREST,
    VG_ADDRESS_MODE_CLAMP_TO_EDGE, VG_ADDRESS_MODE_CLAMP_TO_EDGE, VG_ADDRESS_MODE_CLAMP_TO_EDGE,
    0.0f, VG_ANISOTROPY_1, VG_COMPARISON_FUNC_NONE, {0, 0, 0, 0}, 0.0f, 100.0f, VG_REDUCTION_MODE_DEFAULT },
    // linearMipNearestWrap
    VgSamplerDesc { VG_FILTER_LINEAR, VG_FILTER_LINEAR, VG_MIPMAP_MODE_NEAREST,
    VG_ADDRESS_MODE_REPEAT, VG_ADDRESS_MODE_REPEAT, VG_ADDRESS_MODE_REPEAT,
    0.0f, VG_ANISOTROPY_1, VG_COMPARISON_FUNC_NONE, {0, 0, 0, 0}, 0.0f, 100.0f, VG_REDUCTION_MODE_DEFAULT },
    // downsampleBorder
    VgSamplerDesc { VG_FILTER_LINEAR, VG_FILTER_LINEAR, VG_MIPMAP_MODE_LINEAR,
    VG_ADDRESS_MODE_CLAMP_TO_BORDER, VG_ADDRESS_MODE_CLAMP_TO_BORDER, VG_ADDRESS_MODE_CLAMP_TO_BORDER,
    0.0f, VG_ANISOTROPY_1, VG_COMPARISON_FUNC_NONE, {0, 0, 0, 1}, 0.0f, 100.0f, VG_REDUCTION_MODE_DEFAULT },
    // linearMipNearestClampMinimum
    VgSamplerDesc { VG_FILTER_LINEAR, VG_FILTER_LINEAR, VG_MIPMAP_MODE_NEAREST,
    VG_ADDRESS_MODE_CLAMP_TO_EDGE, VG_ADDRESS_MODE_CLAMP_TO_EDGE, VG_ADDRESS_MODE_CLAMP_TO_EDGE,
    0.0f, VG_ANISOTROPY_1, VG_COMPARISON_FUNC_NONE, {0, 0, 0, 0}, 0.0f, 100.0f, VG_REDUCTION_MODE_MIN },
    // linearMipNearestClampMaximum
    VgSamplerDesc { VG_FILTER_LINEAR, VG_FILTER_LINEAR, VG_MIPMAP_MODE_NEAREST,
    VG_ADDRESS_MODE_CLAMP_TO_EDGE, VG_ADDRESS_MODE_CLAMP_TO_EDGE, VG_ADDRESS_MODE_CLAMP_TO_EDGE,
    0.0f, VG_ANISOTROPY_1, VG_COMPARISON_FUNC_NONE, {0, 0, 0, 0}, 0.0f, 100.0f, VG_REDUCTION_MODE_MAX },
    // linearMipNearestTransparentBorder
    VgSamplerDesc { VG_FILTER_LINEAR, VG_FILTER_LINEAR, VG_MIPMAP_MODE_NEAREST,
    VG_ADDRESS_MODE_CLAMP_TO_BORDER, VG_ADDRESS_MODE_CLAMP_TO_BORDER, VG_ADDRESS_MODE_CLAMP_TO_BORDER,
    0.0f, VG_ANISOTROPY_1, VG_COMPARISON_FUNC_NONE, {0, 0, 0, 0}, 0.0f, 100.0f, VG_REDUCTION_MODE_DEFAULT },
};