#include "Globals.h"
#include "ModuleResources.h"

ModuleResources::ModuleResources()
{
}
ModuleResources::~ModuleResources()
{
}

bool ModuleResources::init()
{
	device = app->getD3D12()->getDevice();
	bool ok = SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
	ok = ok && SUCCEEDED(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList)));
	ok = ok && SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));

	return ok;
}

ComPtr<ID3D12Resource> ModuleResources::createUploadBuffer(const void* data, size_t size)
{
	ComPtr<ID3D12Resource> buffer;
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

	device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&buffer));
	
	BYTE* pData = nullptr;
	CD3DX12_RANGE readRange(0, 0);

	buffer->Map(0, &readRange, reinterpret_cast<void**>(&pData));
	memcpy(pData, data, size);
	buffer->Unmap(0, nullptr);

	return buffer;
}

ComPtr<ID3D12Resource> ModuleResources::createDefaultBuffer(const void* data, size_t size)
{
	ModuleD3D12* d3d12 = app->getD3D12();
	// Create Default Buffer
	auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto defaultDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
	ComPtr<ID3D12Resource> defaultBuffer;
	device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &defaultDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&defaultBuffer));

	// Staging Buffer (UPLOAD HEAP)
	ComPtr<ID3D12Resource> stagingBuffer = createStagingBuffer(data, size);

	// Copy data from Staging Buffer to Default Buffer
	commandList->CopyResource(defaultBuffer.Get(), stagingBuffer.Get());
	commandList->Close();
	ID3D12CommandList* listsToExecute[] = { commandList.Get() };
	d3d12->getCommandQueue()->ExecuteCommandLists(UINT(std::size(listsToExecute)), listsToExecute);

	d3d12->flushCommandQueue();
	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), nullptr);

	return defaultBuffer;
}

ComPtr<ID3D12Resource> ModuleResources::createStagingBuffer(const void* data, size_t size)
{
	ComPtr<ID3D12Resource> buffer;
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));

	BYTE* pData = nullptr;
	CD3DX12_RANGE readRange(0, size);
	buffer->Map(0, &readRange, reinterpret_cast<void**>(&pData));
	memcpy(pData, data, size);
	buffer->Unmap(0, nullptr);

	return buffer;
}