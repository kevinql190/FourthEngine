#pragma once
#include "Module.h"
#include "Application.h"
#include "ModuleD3D12.h"

class ModuleResources : public Module
{
public:
	ModuleResources();
	~ModuleResources();
	bool init() override;

	ComPtr<ID3D12Resource> createUploadBuffer(const void* data, size_t size);
	ComPtr<ID3D12Resource> createDefaultBuffer(const void* data, size_t size);
	ComPtr<ID3D12Resource> createStagingBuffer(const void* data, size_t size);

private:
	ComPtr<ID3D12Device4> device;
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;
};

