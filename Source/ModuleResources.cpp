#include "Globals.h"

#include "ModuleResources.h"
#include "Application.h"
#include "ModuleD3D12.h"

#include "DirectXTex.h"

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

ComPtr<ID3D12Resource> ModuleResources::createUploadBuffer(const void* data, size_t size) const
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

ComPtr<ID3D12Resource> ModuleResources::createDefaultBuffer(const void* data, size_t size) const
{
	ModuleD3D12* d3d12 = app->getD3D12();
	// Create Default Buffer
	auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto defaultDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
	ComPtr<ID3D12Resource> defaultBuffer;
	device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &defaultDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&defaultBuffer));

	// Staging Buffer (UPLOAD HEAP)
	ComPtr<ID3D12Resource> stagingBuffer;
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&stagingBuffer));

	BYTE* pData = nullptr;
	CD3DX12_RANGE readRange(0, size);
	stagingBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pData));
	memcpy(pData, data, size);
	stagingBuffer->Unmap(0, nullptr);

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

ComPtr<ID3D12Resource> ModuleResources::createTextureFromFile(const std::filesystem::path& filePath) const
{
	//1  Load the texture using DirectXTex .
	const wchar_t* fileName = filePath.c_str();
	DirectX::ScratchImage image;
	if (FAILED(LoadFromDDSFile(fileName, DDS_FLAGS_NONE, nullptr, image)))
	{
		if (FAILED(LoadFromTGAFile(fileName, nullptr, image)))
		{
			LoadFromWICFile(fileName, WIC_FLAGS_NONE, nullptr, image);
		}
	}

	const DirectX::TexMetadata& metaData = image.GetMetadata();

	//2  Create the texture resource in the DEFAULT_HEAP.
	ModuleD3D12* d3d12 = app->getD3D12();
	auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	// Descriptor for a 2D texture
	D3D12_RESOURCE_DESC defaultDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metaData.format,
		UINT64(metaData.width),
		UINT(metaData.height),
		UINT16(metaData.arraySize),
		UINT16(metaData.mipLevels)
	);
	ComPtr<ID3D12Resource> texture;
	device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &defaultDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture));

	//3  Create a staging buffer for copying texture data.
	UINT64 size = GetRequiredIntermediateSize(texture.Get(), 0, image.GetImageCount()); // Get the required size for the upload buffer

	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
	ComPtr<ID3D12Resource> stagingBuffer;
	device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&stagingBuffer)
	);

	//4  Call UpdateSubresources to copy data from staging buffer to texture.
	std::vector<D3D12_SUBRESOURCE_DATA> subData;
	subData.reserve(image.GetImageCount());
	// Iterate over mipLevels of each array item to respect Subresource index order
	for (size_t item = 0; item < metaData.arraySize; ++item)
	{
		for (size_t level = 0; level < metaData.mipLevels; ++level)
		{
			const DirectX::Image* subImg = image.GetImage(level, item, 0);
			D3D12_SUBRESOURCE_DATA data = { subImg->pixels, subImg->rowPitch, subImg->slicePitch };
			subData.push_back(data);
		}
	}

	UpdateSubresources(commandList.Get(), texture.Get(), stagingBuffer.Get(), 0, 0, UINT(image.GetImageCount()), subData.data());

	//5  Insert a ResourceBarrier to transition the texture to RESOURCE_STATE_PIXEL_SHADER_RESOURCE.
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		texture.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	commandList->ResourceBarrier(1, &barrier);

	//6  Execute the CommandList and flush the command queue.

	commandList->Close();
	ID3D12CommandList* listsToExecute[] = { commandList.Get() };
	d3d12->getCommandQueue()->ExecuteCommandLists(UINT(std::size(listsToExecute)), listsToExecute);

	d3d12->flushCommandQueue();
	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), nullptr);

	return texture;
}