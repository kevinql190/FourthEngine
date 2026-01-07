#include "Globals.h"
#include "ModuleShaderDescriptors.h"

#include "Application.h"

ModuleShaderDescriptors::ModuleShaderDescriptors()
{
}

ModuleShaderDescriptors::~ModuleShaderDescriptors()
{
}

bool ModuleShaderDescriptors::init()
{
	ID3D12Device* device = app->getD3D12()->getDevice();

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = MAX_DESCRIPTORS * DESCRIPTORS_PER_TABLE;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	bool ok = SUCCEEDED(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap)));

	descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	return true;
}

UINT ModuleShaderDescriptors::createSRV(ID3D12Resource* resource)
{
	ID3D12Device* device = app->getD3D12()->getDevice();

	UINT index = nextFreeIndex++;

	// Create SRV for texture
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle =
		CD3DX12_CPU_DESCRIPTOR_HANDLE(
			srvHeap->GetCPUDescriptorHandleForHeapStart(),
			index,
			descriptorSize
		);
	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = resource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	device->CreateShaderResourceView(resource, &srvDesc, CPUHandle);
	return index;
}

D3D12_CPU_DESCRIPTOR_HANDLE ModuleShaderDescriptors::getCPUDescriptorHandle(UINT index) const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		srvHeap->GetCPUDescriptorHandleForHeapStart(),
		index,
		descriptorSize
	);
}

D3D12_GPU_DESCRIPTOR_HANDLE ModuleShaderDescriptors::getGPUDescriptorHandle(UINT index) const
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(
		srvHeap->GetGPUDescriptorHandleForHeapStart(),
		index,
		descriptorSize
	);
}

void ModuleShaderDescriptors::Reset()
{
	nextFreeIndex = 0;
}