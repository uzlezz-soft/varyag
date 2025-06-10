#include "dds_loader.h"
#include <fstream>

#pragma pack(push, 1)
struct DDSHeader
{
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitchOrLinearSize;
    uint32_t depth;
    uint32_t mipMapCount;
    uint32_t reserved1[11];
    struct PixelFormat {
        uint32_t size;
        uint32_t flags;
        uint32_t fourCC;
        uint32_t RGBBitCount;
        uint32_t RBitMask;
        uint32_t GBitMask;
        uint32_t BBitMask;
        uint32_t ABitMask;
    } pixelFormat;
    uint32_t caps[4];
    uint32_t reserved2;
};
#pragma pack(pop)

constexpr uint32_t DDS_MAGIC = 0x20534444; // "DDS "
constexpr uint32_t DDPF_FOURCC = 0x4;
constexpr uint32_t FOURCC_DXT1 = 0x31545844; // "DXT1"
constexpr uint32_t FOURCC_DXT3 = 0x33545844; // "DXT3"
constexpr uint32_t FOURCC_DXT5 = 0x35545844; // "DXT5"
constexpr uint32_t FOURCC_BC5 = 0x55344342; // "BC5 "
constexpr uint32_t FOURCC_ATI2 = 0x32495441; // "ATI2" (alternative BC5)


bool DDSLoader::Load(const std::filesystem::path& path, DDSData& data)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != DDS_MAGIC) return false;

    DDSHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    data.width = header.width;
    data.height = header.height;
    auto mips = (header.mipMapCount > 0) ? header.mipMapCount : 1;

    if (header.pixelFormat.flags & DDPF_FOURCC)
    {
        switch (header.pixelFormat.fourCC)
        {
        case FOURCC_DXT1: data.format = vg::Format::Bc1Unorm; break;
        case FOURCC_DXT3: data.format = vg::Format::Bc2Unorm; break;
        case FOURCC_DXT5: data.format = vg::Format::Bc3Unorm; break;
        case FOURCC_BC5: 
        case FOURCC_ATI2:
            data.format = vg::Format::Bc5Unorm; break;
        default: return false;
        }
    }
    else return false; // Unsupported format

    data.mips.resize(mips);
    uint32_t mipWidth = data.width;
    uint32_t mipHeight = data.height;

    for (uint32_t i = 0; i < mips; ++i)
    {
        size_t mipSize = std::max(1u, (mipWidth * mipHeight/* * 4*/) / (header.pixelFormat.fourCC == FOURCC_DXT1 ? 2 : 1));
        data.mips[i].resize(mipSize);
        file.read(reinterpret_cast<char*>(data.mips[i].data()), mipSize);

        mipWidth = std::max(1u, mipWidth / 2);
        mipHeight = std::max(1u, mipHeight / 2);
    }

    return true;
}
