#pragma once
#include "Module.h"

#include <filesystem>

class ModuleResources : public Module
{
public:
	ModuleResources();
	~ModuleResources();
	bool init() override;

	ComPtr<ID3D12Resource> createUploadBuffer(const void* data, size_t size) const;
	ComPtr<ID3D12Resource> createDefaultBuffer(const void* data, size_t size) const;
	ComPtr<ID3D12Resource> createTextureFromFile(const std::filesystem::path& path) const;

private:
	ComPtr<ID3D12Device4> device;
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;
};

