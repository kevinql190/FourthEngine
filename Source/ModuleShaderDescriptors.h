#pragma once
#include "Module.h"

class ModuleShaderDescriptors : public Module
{
public:
	ModuleShaderDescriptors();
	~ModuleShaderDescriptors();
	bool init() override;

	UINT allocTable();
	void Reset();

	void createTextureSRV(UINT tableIndex, ID3D12Resource* resource, UINT slot);
	void createNullTexture2DSRV(UINT tableIndex, UINT slot);
	
	D3D12_CPU_DESCRIPTOR_HANDLE		getCPUDescriptorHandle(UINT index, UINT slot) const;
	D3D12_GPU_DESCRIPTOR_HANDLE		getGPUDescriptorHandle(UINT index, UINT slot) const;
	ID3D12DescriptorHeap*			getHeap() const { return srvHeap.Get(); }

private:
	enum { MAX_DESCRIPTORS = 4096, DESCRIPTORS_PER_TABLE = 2 }; // DESCRIPTORS_PER_TABLE might change if we add more types of resources

	ComPtr<ID3D12DescriptorHeap> srvHeap;
	UINT descriptorSize = 0;
	UINT nextFreeIndex = 0;		

};

