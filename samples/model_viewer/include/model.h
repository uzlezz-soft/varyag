#pragma once

#include "mesh.h"
#include <filesystem>
#include <vector>
#include <memory>

class Model
{
public:
	~Model();

	static std::optional<std::shared_ptr<Model>> From(Application& app, const std::filesystem::path& path);

	std::vector<std::shared_ptr<Mesh>> GetMeshes() const { return _meshes; }

private:
	std::vector<std::shared_ptr<Mesh>> _meshes;

	Model(const std::vector<std::shared_ptr<Mesh>>& meshes);
};