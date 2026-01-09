#include "Globals.h"
#include "Material.h"

#include "Application.h"

Material::Material()
{
}
Material::~Material()
{
}

void Material::load(const tinygltf::Model& model, const tinygltf::PbrMetallicRoughness& material, const char* basePath)
{
    colour = Vector4(float(material.baseColorFactor[0]),
                     float(material.baseColorFactor[1]),
                     float(material.baseColorFactor[2]),
                     float(material.baseColorFactor[3]));

	// Load base color texture if present
    if (material.baseColorTexture.index >= 0)
    {
        const tinygltf::Texture& texture = model.textures[material.baseColorTexture.index];
        const tinygltf::Image& image = model.images[texture.source];
        if (!image.uri.empty())
        {
            colourTex = app->getResources()->createTextureFromFile(std::string(basePath) + image.uri);
        }
    }
}