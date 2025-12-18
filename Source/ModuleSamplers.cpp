#include "Globals.h"
#include "ModuleSamplers.h"
#include "Application.h"

ModuleSamplers::ModuleSamplers()
{
}
ModuleSamplers::~ModuleSamplers()
{
}

bool ModuleSamplers::init()
{
	ModuleD3D12* d3d12 = app->getD3D12();
	ID3D12Device2* device = d3d12->getDevice();

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 4;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

	bool ok = SUCCEEDED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap)));
	cpuStart = heap->GetCPUDescriptorHandleForHeapStart();
	gpuStart = heap->GetGPUDescriptorHandleForHeapStart();

	descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	// Create samplers
	D3D12_SAMPLER_DESC samplerDesc = {};

	// Linear Wrap
	D3D12_SAMPLER_DESC linearWrap =
	{
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f, 16, D3D12_COMPARISON_FUNC_NEVER,
		{0.0f, 0.0f, 0.0f, 0.0f},
		0.0f, D3D12_FLOAT32_MAX
	};
	device->CreateSampler(&linearWrap, getCPUHandle(LINEAR_WRAP));

	// Point Wrap
	D3D12_SAMPLER_DESC pointWrap =
	{
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f, 16, D3D12_COMPARISON_FUNC_NEVER,
		{0.0f, 0.0f, 0.0f, 0.0f},
		0.0f, D3D12_FLOAT32_MAX
	};
	device->CreateSampler(&pointWrap, getCPUHandle(POINT_WRAP));

	// Linear Clamp
	D3D12_SAMPLER_DESC linearClamp =
	{
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f, 16, D3D12_COMPARISON_FUNC_NEVER,
		{0.0f, 0.0f, 0.0f, 0.0f},
		0.0f, D3D12_FLOAT32_MAX
	};
	device->CreateSampler(&linearClamp, getCPUHandle(LINEAR_CLAMP));

	// Point Clamp
	D3D12_SAMPLER_DESC pointClamp =
	{
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f, 16, D3D12_COMPARISON_FUNC_NEVER,
		{0.0f, 0.0f, 0.0f, 0.0f},
		0.0f, D3D12_FLOAT32_MAX
	};
	device->CreateSampler(&pointClamp, getCPUHandle(POINT_CLAMP));

	return ok;
}