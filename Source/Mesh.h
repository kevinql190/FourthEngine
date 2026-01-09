#pragma once

#include "tiny_gltf.h"

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

private:
	ComPtr<ID3D12Resource>			vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW		vertexBufferView;
	ComPtr<ID3D12Resource>			indexBuffer;
	D3D12_INDEX_BUFFER_VIEW 		indexBufferView;

	bool loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model,
		int accesorIndex);
	bool loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model,
		const std::map<std::string, int>& attributes, const char* accesorName);
};

