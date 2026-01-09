#include "Globals.h"
#include "Mesh.h"

#include "Application.h"

Mesh::Mesh()
{

}
Mesh::~Mesh()
{
}

void Mesh::load(const tinygltf::Model& model, const tinygltf::Primitive& primitive)
{
	const auto& itPos = primitive.attributes.find("POSITION");
    if (itPos == primitive.attributes.end())
		return; // If no position no geometry data

    uint32_t numVertices = uint32_t(model.accessors[itPos->second].count);
    Vertex* vertices = new Vertex[numVertices];
    uint8_t* vertexData = (uint8_t*)vertices;  // Casts Vertex Buffer to Bytes (uint8_t*) buffer 
    loadAccessorData(vertexData + offsetof(Vertex, position), sizeof(Vector3), sizeof(Vertex), numVertices, model, itPos->second);
    loadAccessorData(vertexData + offsetof(Vertex, texCoord0), sizeof(Vector2), sizeof(Vertex), numVertices, model, primitive.attributes, "TEXCOORD_0");

	// Create Vertex Buffer
	vertexBuffer = app->getResources()->createDefaultBuffer(vertexData, numVertices * sizeof(Vertex));

    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes  = sizeof(Vertex);
    vertexBufferView.SizeInBytes    = numVertices * sizeof(Vertex);

    // Load Index Buffer if present
    const tinygltf::Accessor& indAcc = model.accessors[primitive.indices];
    if (indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT ||
        indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT ||
        indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE)
    {
        uint32_t indexElementSize = tinygltf::GetComponentSizeInBytes(indAcc.componentType);
        uint32_t numIndices = uint32_t(indAcc.count);
        uint8_t* indices = new uint8_t[numIndices * indexElementSize];
        loadAccessorData(indices, indexElementSize, indexElementSize, numIndices, model, primitive.indices);

		// Create Index Buffer
		indexBuffer = app->getResources()->createDefaultBuffer(indices, numIndices * indexElementSize);

        static const DXGI_FORMAT formats[3] = { DXGI_FORMAT_R8_UINT, DXGI_FORMAT_R16_UINT, DXGI_FORMAT_R32_UINT };

        indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        indexBufferView.Format = formats[indexElementSize >> 1];
        indexBufferView.SizeInBytes = numIndices * indexElementSize;
    }
}

bool Mesh::loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model, int
    accesorIndex)
{
	if (accesorIndex < 0 || accesorIndex >= model.accessors.size())
		return false;
	const tinygltf::Accessor& accessor = model.accessors[accesorIndex];
	if (accessor.count != elemCount)
		return false;

	// Get buffer of the accessor
	const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
	const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
	size_t accessorByteOffset = accessor.byteOffset + bufferView.byteOffset;
	for (size_t i = 0; i < elemCount; ++i)
	{
		size_t indexOffset = accessorByteOffset + i * bufferView.byteStride;
		memcpy(data + i * stride, &buffer.data[indexOffset], elemSize);
	}
	return true;
}

bool Mesh::loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model,
    const std::map<std::string, int>& attributes, const char* accesorName)
{
    // Helper function to find accessor by name and call the main loadAccessorData function
    const auto& it = attributes.find(accesorName);
    if (it != attributes.end())
    {
        return loadAccessorData(data, elemSize, stride, elemCount, model, it->second);
    }
    return false;
}