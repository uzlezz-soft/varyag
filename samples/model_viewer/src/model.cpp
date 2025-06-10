#include "model.h"
#include "application.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

std::optional<std::shared_ptr<Model>> Model::From(Application& app, const std::filesystem::path& path)
{
	std::cout << "Loading model " << path.filename().generic_string() << "...\n";

	Assimp::Importer importer;
	auto scene = importer.ReadFile(path.generic_string(), aiProcessPreset_TargetRealtime_MaxQuality
		| aiProcess_FlipUVs | aiProcess_FlipWindingOrder | aiProcess_GenBoundingBoxes);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cerr << "Assimp failed to load model: " << importer.GetErrorString() << std::endl;
		return {};
	}
	uint32_t totalVertices = 0;
	std::vector<std::shared_ptr<Mesh>> meshes;
	for (uint32_t i = 0; i < scene->mNumMeshes; i++)
	{
		std::cout << "Mesh " << i + 1 << "/" << scene->mNumMeshes << "\n";
		auto mesh = Mesh::From(app, scene->mMeshes[i], scene, path);
		totalVertices += mesh->GetVertexCount();
		meshes.push_back(mesh);
	}

	std::sort(meshes.begin(), meshes.end(), [](const auto& a, const auto& b)
	{
		return a->GetMaterial().twoSided > b->GetMaterial().twoSided;
	});

	std::cout << "Total: " << totalVertices << " vertices\n";
	return std::shared_ptr<Model>(new Model(meshes));
}

Model::Model(const std::vector<std::shared_ptr<Mesh>>& meshes) : _meshes(meshes)
{
	
}

Model::~Model()
{

}