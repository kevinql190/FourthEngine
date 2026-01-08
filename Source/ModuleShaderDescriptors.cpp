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

UINT ModuleShaderDescriptors::allocTable()
{
	return nextFreeIndex++;
}

void ModuleShaderDescriptors::Reset()
{
	nextFreeIndex = 0;
}

void ModuleShaderDescriptors::createTextureSRV(UINT tableIndex, ID3D12Resource* resource, UINT slot)
{
	ID3D12Device* device = app->getD3D12()->getDevice();

	UINT descriptorIndex = tableIndex * DESCRIPTORS_PER_TABLE + slot;

	// Create SRV for texture
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle =
		CD3DX12_CPU_DESCRIPTOR_HANDLE(
			srvHeap->GetCPUDescriptorHandleForHeapStart(),
			descriptorIndex,
			descriptorSize
		);
	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = resource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	device->CreateShaderResourceView(resource, &srvDesc, CPUHandle);
}

void ModuleShaderDescriptors::createNullTexture2DSRV(UINT tableIndex, UINT slot)
{
	ID3D12Device* device = app->getD3D12()->getDevice();

	UINT descriptorIndex = tableIndex * DESCRIPTORS_PER_TABLE + slot;

	// Create SRV for null texture
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle =
		CD3DX12_CPU_DESCRIPTOR_HANDLE(
			srvHeap->GetCPUDescriptorHandleForHeapStart(),
			descriptorIndex,
			descriptorSize
		);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	device->CreateShaderResourceView(nullptr, &srvDesc, CPUHandle);
}

D3D12_CPU_DESCRIPTOR_HANDLE ModuleShaderDescriptors::getCPUDescriptorHandle(UINT index, UINT slot) const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		srvHeap->GetCPUDescriptorHandleForHeapStart(),
		index * DESCRIPTORS_PER_TABLE + slot,
		descriptorSize
	);
}

D3D12_GPU_DESCRIPTOR_HANDLE ModuleShaderDescriptors::getGPUDescriptorHandle(UINT index, UINT slot) const
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(
		srvHeap->GetGPUDescriptorHandleForHeapStart(),
		index * DESCRIPTORS_PER_TABLE + slot,
		descriptorSize
	);
}