#include "mesh.h"
#include "application.h"

Mesh::~Mesh()
{
    if (!_vertexBuffer) return;
    vg::Device device;
    _vertexBuffer.GetDevice(&device);
    device.DestroyBuffer(_vertexBuffer);
}

static std::string GetTexture(aiMaterial* mat, aiTextureType type)
{
    if (mat->GetTextureCount(type) == 0) return "";

    aiString path;
    if (mat->GetTexture(type, 0, &path) == AI_SUCCESS)
        return path.C_Str();
    return "";
}

std::shared_ptr<Mesh> Mesh::From(Application& app, aiMesh* mesh, const aiScene* scene, const std::filesystem::path& modelPath)
{
    vg::Device device = app.GetDevice();
    vg::BufferDesc bufferDesc = { mesh->mNumVertices * sizeof(Vertex) + mesh->mNumFaces * 3 * sizeof(uint32_t),
        vg::BufferUsage::General, vg::HeapType::Upload};
    vg::Buffer uploadBuffer;
    if (device.CreateBuffer(&bufferDesc, &uploadBuffer) != vg::Result::Success)
    {
        std::cerr << "Unable to create upload buffer for mesh\n";
        return nullptr;
    }

    bufferDesc.heapType = vg::HeapType::Gpu;
    vg::Buffer vertexBuffer;
    if (device.CreateBuffer(&bufferDesc, &vertexBuffer) != vg::Result::Success)
    {
        std::cerr << "Unable to create vertex buffer for mesh\n";
        device.DestroyBuffer(uploadBuffer);
        return nullptr;
    }

    vertexBuffer.SetName((std::string(mesh->mName.C_Str()) + " | Vertex+Index").c_str());

    std::vector<float> uvs(mesh->mNumVertices * 2);
    if (mesh->HasTextureCoords(0))
    {
        for (uint32_t i = 0; i < mesh->mNumVertices; i++)
        {
            uvs[i * 2 + 0] = mesh->mTextureCoords[0][i].x;
            uvs[i * 2 + 1] = mesh->mTextureCoords[0][i].y;
        }
    }
    else
    {
        memset(uvs.data(), 0, sizeof(float) * uvs.size());
    }

    std::vector<uint32_t> indices(mesh->mNumFaces * 3);
    for (uint32_t i = 0; i < mesh->mNumFaces; i++)
    {
        indices[i * 3 + 0] = mesh->mFaces[i].mIndices[0];
        indices[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
        indices[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
    }
    void* mapped = nullptr;
    if (uploadBuffer.Map(&mapped) != vg::Result::Success)
    {
        std::cerr << "Unable to upload data for mesh\n";
        device.DestroyBuffer(uploadBuffer);
        device.DestroyBuffer(vertexBuffer);
        return nullptr;
    }
    memcpy(mapped, mesh->mVertices, mesh->mNumVertices * sizeof(float) * 3);
    if (mesh->HasNormals())
        memcpy(static_cast<uint8_t*>(mapped) + sizeof(float) * 3 * mesh->mNumVertices, mesh->mNormals, mesh->mNumVertices * sizeof(float) * 3);
    if (mesh->HasTangentsAndBitangents())
        memcpy(static_cast<uint8_t*>(mapped) + sizeof(float) * 6 * mesh->mNumVertices, mesh->mTangents, mesh->mNumVertices * sizeof(float) * 3);
    if (mesh->HasTextureCoords(0))
        memcpy(static_cast<uint8_t*>(mapped) + sizeof(float) * 9 * mesh->mNumVertices, uvs.data(), mesh->mNumVertices * sizeof(float) * 2);
    memcpy(static_cast<uint8_t*>(mapped) + mesh->mNumVertices * sizeof(Vertex), indices.data(), indices.size() * sizeof(uint32_t));
    uploadBuffer.Unmap();
    
    app.SubmitImmediately([&](vg::CommandList cmd)
    {
        cmd.CopyBufferToBuffer(vertexBuffer, 0, uploadBuffer, 0, VG_WHOLE_SIZE);
    });

    Material material;
    if (mesh->mMaterialIndex >= 0)
    {
        auto aiMaterial = scene->mMaterials[mesh->mMaterialIndex];

        int twoSided = 0;
        if (aiMaterial->Get(AI_MATKEY_TWOSIDED, material.twoSided) == aiReturn_SUCCESS && twoSided != 0)
            material.twoSided = true;

        material.baseColor = GetTexture(aiMaterial, aiTextureType_DIFFUSE);
        if (material.baseColor.empty())
            material.baseColor = GetTexture(aiMaterial, aiTextureType_BASE_COLOR);

        material.specular = GetTexture(aiMaterial, aiTextureType_SPECULAR);

        material.normal = GetTexture(aiMaterial, aiTextureType_NORMALS);
    }

    auto baseColor = Texture::From(app, modelPath.parent_path() / material.baseColor);
    if (baseColor.has_value()) material.baseColorTexture = baseColor.value();

    AABB aabb = {
        { mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z },
        { mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z }
    };

    device.DestroyBuffer(uploadBuffer);
    return std::shared_ptr<Mesh>(new Mesh(mesh->mName.C_Str(), vertexBuffer, mesh->mNumFaces * 3, aabb, material));
}

Mesh::Mesh(std::string_view name, vg::Buffer vertexBuffer, uint32_t indexCount, const AABB& aabb, const Material& material)
    : _name(name), _vertexBuffer(vertexBuffer), _indexCount(indexCount), _material(material), _aabb(aabb)
{
    vg::BufferDesc desc;
    vertexBuffer.GetDesc(&desc);
    _indexOffset = desc.size - sizeof(uint32_t) * indexCount;
    _vertexCount = _indexOffset / sizeof(Vertex);

    _positionsVB = vg::VertexBufferView { _vertexBuffer, 0, sizeof(float) * 3 };
    _normalsVB = vg::VertexBufferView { _vertexBuffer, _vertexCount * sizeof(float) * 3, sizeof(float) * 3 };
    _tangentsVB = vg::VertexBufferView { _vertexBuffer, _normalsVB.offset + _vertexCount * sizeof(float) * 3, sizeof(float) * 3 };
    _uvsVB = vg::VertexBufferView { _vertexBuffer, _tangentsVB.offset + _vertexCount * sizeof(float) * 3, sizeof(float) * 2 };
}