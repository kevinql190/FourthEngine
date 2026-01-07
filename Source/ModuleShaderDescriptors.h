#pragma once
#include "Module.h"

class ModuleShaderDescriptors : public Module
{
public:
	ModuleShaderDescriptors();
	~ModuleShaderDescriptors();
	bool init() override;

	UINT createSRV(ID3D12Resource* resource);
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUDescriptorHandle(UINT index) const;
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUDescriptorHandle(UINT index) const;
	ID3D12DescriptorHeap* getHeap() const { return srvHeap.Get(); }
	void Reset();

private:
	enum { MAX_DESCRIPTORS = 4096, DESCRIPTORS_PER_TABLE = 8 };

	ComPtr<ID3D12DescriptorHeap> srvHeap;
	UINT descriptorSize = 0;
	UINT nextFreeIndex = 0;		

};

