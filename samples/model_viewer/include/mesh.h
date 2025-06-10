#pragma once

#include "common.h"
#include "texture.h"
#include "frustum.h"

#include <string_view>
#include <string>
#include <memory>

#include <assimp/mesh.h>
#include <assimp/scene.h>

struct Vertex
{
	float position[3];
	float normal[3];
	float tangent[3];
	float uv[2];
};

struct Material
{
	bool twoSided{ false };

	std::string baseColor;
	std::string normal;
	std::string specular;

	std::shared_ptr<Texture> baseColorTexture;
};

class Application;
class Mesh
{
public:
	~Mesh();

	static std::shared_ptr<Mesh> From(Application& app, aiMesh* mesh, const aiScene* scene, const std::filesystem::path& modelPath);

	const Material& GetMaterial() const { return _material; }
	vg::Buffer GetVertexBuffer() const { return _vertexBuffer; }
	uint64_t GetIndexOffset() const { return _indexOffset; }
	uint32_t GetIndexCount() const { return _indexCount; }
	uint32_t GetVertexCount() const { return _vertexCount; }
	const AABB& GetAABB() const { return _aabb; }
	
	const vg::VertexBufferView& GetPositionsVB() const { return _positionsVB; }
	const vg::VertexBufferView& GetNormalsVB() const { return _normalsVB; }
	const vg::VertexBufferView& GetTangentsVB() const { return _tangentsVB; }
	const vg::VertexBufferView& GetUVsVB() const { return _uvsVB; }

private:
	std::string _name;
	vg::Buffer _vertexBuffer;
	uint64_t _vertexCount;
	uint32_t _indexCount;
	uint64_t _indexOffset;
	Material _material;
	AABB _aabb;

	vg::VertexBufferView _positionsVB;
	vg::VertexBufferView _normalsVB;
	vg::VertexBufferView _tangentsVB;
	vg::VertexBufferView _uvsVB;

	Mesh(std::string_view name, vg::Buffer vertexBuffer, uint32_t indexCount, const AABB& aabb, const Material& material);
};