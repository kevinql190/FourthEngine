#pragma once

namespace tinygltf { class Model; class PbrMetallicRoughness; }

struct MaterialData
{
	Vector4 baseColour;
	BOOL    hasColourTexture; // BOOL instead of bool to match HLSL 4 bytes bool
};

class Material
{
public:
	Material();
	~Material();

	void load(const tinygltf::Model& model, const tinygltf::PbrMetallicRoughness& material, const char* basePath);

	MaterialData getData() const { return data; }
	ID3D12Resource* getColourTexture() const { return colourTex.Get(); }

	D3D12_GPU_DESCRIPTOR_HANDLE getTextureGPUHandle() const { return textureGPUHandle; }

private:
	ComPtr<ID3D12Resource> colourTex;
	Vector4 colour;
	D3D12_GPU_DESCRIPTOR_HANDLE textureGPUHandle;

	MaterialData data;
};

