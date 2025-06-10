#include "texture.h"
#include "application.h"
#include "dds_loader.h"

#include <fstream>
#include <iostream>
#include <FreeImage.h>

Texture::~Texture()
{
	vg::Device device;
	if (_texture && _texture.GetDevice(&device) == vg::Result::Success)
		device.DestroyTexture(_texture);
}

std::optional<std::shared_ptr<Texture>> Texture::From(Application& app, const std::filesystem::path& path)
{
	if (!exists(path)) return {};

    vg::Device device = app.GetDevice();

    if (path.extension() == ".dds")
    {
        /*DDSTexture ddsTexture;
        if (!LoadDDSTexture(path, ddsTexture) || ddsTexture.width <= 32 || ddsTexture.height <= 32) return {};

        size_t totalDataSize = 0;
        for (uint32_t i = 0; i < ddsTexture.mips; ++i)
        {
            uint32_t mipWidth = std::max(1u, ddsTexture.width >> i);
            uint32_t mipHeight = std::max(1u, ddsTexture.height >> i);
            totalDataSize += std::max(ComputeMipSize(mipWidth, mipHeight, ddsTexture.format), 256ull);
            totalDataSize = (totalDataSize + 255) & ~256;
        }

        vg::BufferDesc bufferDesc = { totalDataSize, vg::BufferUsage::General, vg::HeapType::Upload};
        vg::Buffer buffer;
        if (device.CreateBuffer(&bufferDesc, &buffer) != vg::Result::Success)
        {
            std::cerr << "Unable to create upload buffer for texture.\n";
            return {};
        }
        void* data;
        if (buffer.Map(&data) != vg::Result::Success)
        {
            std::cerr << "Unable to map upload buffer for texture.\n";
            device.DestroyBuffer(buffer);
            return {};
        }

        memcpy(data, ddsTexture.data.data(), ddsTexture.data.size());
        buffer.Unmap();

        vg::TextureDesc textureDesc = { vg::TextureType::e2d, GetVgFormat(ddsTexture.format), ddsTexture.width, ddsTexture.height,
            1, ddsTexture.mips, vg::SampleCount::e1, vg::TextureUsageFlags::ShaderResource, vg::TextureTiling::Optimal,
            vg::TextureLayout::TransferDest, vg::HeapType::Gpu };
        vg::Texture texture;
        if (device.CreateTexture(&textureDesc, &texture) != vg::Result::Success)
        {
            std::cerr << "Unable to create texture.\n";
            device.DestroyBuffer(buffer);
            return {};
        }
        texture.SetName(path.filename().generic_string().c_str());

        app.SubmitImmediately([&](vg::CommandList cmd)
        {
            cmd.BeginMarker(std::format("Loading {}", path.filename().generic_string()).c_str(), { 1.0f, 1.0f, 1.0f });

            uint32_t mipWidth = textureDesc.width;
            uint32_t mipHeight = textureDesc.height;
            uint64_t offset = 0;
            for (uint32_t i = 0; i < ddsTexture.mips; i++)
            {
                vg::Region region = { i, 0, 1, { 0, 0, 0 }, mipWidth, mipHeight, 1 };
                cmd.CopyBufferToTexture(texture, &region, buffer, offset);
                offset += (ComputeMipSize(mipWidth, mipHeight, ddsTexture.format) + 3) & ~256;

                mipWidth = std::max(1u, mipWidth / 2);
                mipHeight = std::max(1u, mipHeight / 2);
            }

            vg::TextureBarrier textureBarrier = { vg::PipelineStageFlags::AllTransfer, vg::AccessFlags::TransferWrite,
                vg::PipelineStageFlags::AllGraphics, vg::AccessFlags::ShaderSampledRead,
                vg::TextureLayout::TransferDest, vg::TextureLayout::ShaderResource, texture,
                { 0, textureDesc.mipLevels, 0, 1 } };
            vg::DependencyInfo dependency = { 0, nullptr, 0, nullptr, 1, &textureBarrier };
            cmd.Barrier(&dependency);

            cmd.EndMarker();
        });

        device.DestroyBuffer(buffer);
        return std::shared_ptr<Texture>(new Texture(texture));*/

        /*FIBITMAP* image = FreeImage_Load(FIF_DDS, path.generic_string().c_str(), DDS_DEFAULT);
        if (!image)
        {
            std::cerr << "Failed to load texture: " << path << std::endl;
            return {};
        }

        unsigned width = FreeImage_GetWidth(image);
        unsigned height = FreeImage_GetHeight(image);
        unsigned size = FreeImage_GetMemorySize(image);
        return {};*/

        DDSData data;
        if (!DDSLoader::Load(path, data)) return {};

        if (data.width < 64 || data.height < 64) return {};

        UINT64 uploadBufferSize = 0;
        std::vector<UINT64> mipOffsets(data.mips.size());

        for (uint32_t i = 0; i < data.mips.size(); ++i)
        {
            uint32_t mipWidth = std::max(1u, data.width >> i);
            uint32_t mipHeight = std::max(1u, data.height >> i);

            UINT64 mipSize = ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * (data.format == vg::Format::Bc1Unorm ? 8 : 16);
            mipOffsets[i] = uploadBufferSize;
            uploadBufferSize += mipSize;
        }

        uploadBufferSize = (uploadBufferSize + 255) & ~255;

        vg::BufferDesc bufferDesc = { uploadBufferSize, vg::BufferUsage::General, vg::HeapType::Upload };
        vg::Buffer buffer;
        if (device.CreateBuffer(&bufferDesc, &buffer) != vg::Result::Success)
        {
            std::cerr << "Unable to create upload buffer for texture.\n";
            return {};
        }
        void* mappedData;
        if (buffer.Map(&mappedData) != vg::Result::Success)
        {
            std::cerr << "Unable to map upload buffer for texture.\n";
            device.DestroyBuffer(buffer);
            return {};
        }
        for (uint32_t i = 0; i < data.mips.size(); ++i)
        {
            memcpy(static_cast<uint8_t*>(mappedData) + mipOffsets[i], data.mips[i].data(), data.mips[i].size());
        }
        buffer.Unmap();

        vg::TextureDesc textureDesc = { vg::TextureType::e2d, data.format, data.width, data.height,
            1, static_cast<uint32_t>(data.mips.size()), vg::SampleCount::e1, vg::TextureUsageFlags::ShaderResource, vg::TextureTiling::Optimal,
            vg::TextureLayout::TransferDest, vg::HeapType::Gpu };
        vg::Texture texture;
        if (device.CreateTexture(&textureDesc, &texture) != vg::Result::Success)
        {
            std::cerr << "Unable to create texture.\n";
            device.DestroyBuffer(buffer);
            return {};
        }
        texture.SetName(path.filename().generic_string().c_str());

        app.SubmitImmediately([&](vg::CommandList cmd)
        {
            cmd.BeginMarker(std::format("Loading {}", path.filename().generic_string()).c_str(), { 1.0f, 1.0f, 1.0f });

            uint32_t mipWidth = textureDesc.width;
            uint32_t mipHeight = textureDesc.height;
            uint64_t offset = 0;
            for (uint32_t i = 0; i < data.mips.size(); i++)
            {
                if (mipWidth <= 64 || mipHeight <= 64) break;
                vg::Region region = { i, 0, 1, { 0, 0, 0 }, mipWidth, mipHeight, 1 };
                cmd.CopyBufferToTexture(texture, &region, buffer, mipOffsets[i]);

                mipWidth = std::max(1u, mipWidth / 2);
                mipHeight = std::max(1u, mipHeight / 2);
            }

            vg::TextureBarrier textureBarrier = { vg::PipelineStageFlags::AllTransfer, vg::AccessFlags::TransferWrite,
                vg::PipelineStageFlags::AllGraphics, vg::AccessFlags::ShaderSampledRead,
                vg::TextureLayout::TransferDest, vg::TextureLayout::ShaderResource, texture,
                { 0, textureDesc.mipLevels, 0, 1 } };
            vg::DependencyInfo dependency = { 0, nullptr, 0, nullptr, 1, &textureBarrier };
            cmd.Barrier(&dependency);

            cmd.EndMarker();
        });

        device.DestroyBuffer(buffer);
        return std::shared_ptr<Texture>(new Texture(texture));
    }
    else
    {
        assert(false);
        return {};
    }
    return {};
}

Texture::Texture(vg::Texture texture) : _texture(texture)
{
    vg::TextureDesc desc;
    texture.GetDesc(&desc);

    vg::TextureViewDesc viewDesc = { desc.format, vg::TextureViewType::e2d, vg::TextureDescriptorType::Srv,
        { vg::ComponentMapping::Identity, vg::ComponentMapping::Identity, vg::ComponentMapping::Identity, vg::ComponentMapping::Identity, },
        0, desc.mipLevels, 0, 1 };
    texture.CreateView(&viewDesc, &_srv);
}