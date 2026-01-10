#pragma once
#include "Mesh.h"
#include "Material.h"

class Model
{
public:
	Model();
	~Model();

	bool loadFromFile(const char* filename, const char* basePath);

	std::vector<Material*> getMaterials() const { return materials; }
	std::vector<Mesh*> getMeshes() const { return meshes; }

	Matrix& getModelMatrix() { return modelM; }

private:
	std::vector<Mesh*> meshes;
	std::vector<Material*> materials;
	Matrix modelM = Matrix::Identity;
};

