#pragma once

#include <map>
namespace tinygltf { class Model; struct Primitive; }

class Mesh
{
public:
    struct Vertex
    {
        Vector3 position;
        Vector2 texCoord0;
    };

	Mesh();
	~Mesh();
	void load(const tinygltf::Model& model, const tinygltf::Primitive& primitive);

	int getMaterialIndex() const { return materialIndex; }
	D3D12_VERTEX_BUFFER_VIEW const& getVertexBufferView() const { return vertexBufferView; }

	bool hasIndexBuffer() const { return indexBuffer != nullptr; }
	D3D12_INDEX_BUFFER_VIEW const& getIndexBufferView() const { return indexBufferView; }

	uint32_t getVertexCount() const { return numVertices; }
	uint32_t getIndexCount() const { return numIndices; }

private:
	ComPtr<ID3D12Resource>			vertexBuffer;
	ComPtr<ID3D12Resource>			indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW		vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW 		indexBufferView;
	uint32_t						numVertices = 0;
	uint32_t						numIndices = 0;

	int32_t  materialIndex = -1;

	bool loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model,
		int accesorIndex);
	bool loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model,
		const std::map<std::string, int>& attributes, const char* accesorName);
};

