#pragma once

#include "common.h"
#include <memory>
#include <optional>
#include <filesystem>

class Application;
class Texture
{
public:
	~Texture();

	static std::optional<std::shared_ptr<Texture>> From(Application& app, const std::filesystem::path& path);

	uint32_t GetSrv() { return _srv; }

private:
	vg::Texture _texture;
	uint32_t _srv;

	Texture(vg::Texture texture);
};