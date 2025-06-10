#pragma once

#include "common.h"
#include <filesystem>

struct DDSData
{
	uint32_t width;
	uint32_t height;
	vg::Format format;
	std::vector<std::vector<uint8_t>> mips;
};

class DDSLoader
{
public:
	static bool Load(const std::filesystem::path& path, DDSData& data);
};