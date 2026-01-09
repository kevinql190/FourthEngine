#include "Globals.h"
#include "Model.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#define TINYGLTF_IMPLEMENTATION /* Only in one of the includes */
#include "tiny_gltf.h"

Model::Model()
{
}
Model::~Model()
{
}
bool Model::loadFromFile(const char* filename, const char* basePath)
{
	tinygltf::TinyGLTF gltfContext;
	tinygltf::Model model;
	std::string error, warning;

	bool loadOk = gltfContext.LoadASCIIFromFile(&model, &error, &warning, filename);
	
	if (loadOk)
	{
		// Load meshes
		Mesh* mesh = new Mesh();
		for (const auto& srcMesh : model.meshes)
		{
			for (const auto& primitive : srcMesh.primitives)
			{
				mesh->load(model, primitive);
				meshes.push_back(mesh);
			}
		}

		// Load materials
		Material* material = new Material();
		for (const auto& srcMaterial : model.materials)
		{
			material->load(model, srcMaterial.pbrMetallicRoughness, "");
			materials.push_back(material);
		}
	}
	else LOG("Error loading %s: %s", filename, error.c_str());

	return true;
}
