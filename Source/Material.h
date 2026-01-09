#pragma once

#include "tiny_gltf.h"

class Material
{
public:
	Material();
	~Material();

	void load(const tinygltf::Model& model, const tinygltf::PbrMetallicRoughness& material, const char* basePath);

private:
	ComPtr<ID3D12Resource> colourTex;
	Vector4 colour;
};

