#pragma once
#include "Mesh.h"
#include "Material.h"

class Model
{
public:
	Model();
	~Model();
	bool loadFromFile(const char* filename, const char* basePath);

private:
	std::vector<Mesh*> meshes;
	std::vector<Material*> materials;
};

