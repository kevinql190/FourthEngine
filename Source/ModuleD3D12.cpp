#include "Globals.h"
#include "ModuleD3D12.h"

ModuleD3D12::ModuleD3D12(HWND hWnd): m_hWnd(hWnd)
{
}

ModuleD3D12::~ModuleD3D12()
{
}

bool ModuleD3D12::init()
{
	getWindowSize(windowWidth, windowHeight);

	enableDebugLayer();
	bool ok = createDevice();
	ok = ok && createCommandQueue();
	ok = ok && createSwapChain();
	ok = ok && createRenderTargets();
	ok = ok && createDepthStencil();
	ok = ok && createCommandAllocatorsAndLists();
	ok = ok && createFences();

	return ok;
}

void ModuleD3D12::preRender()
{
	currentIndex = swapChain->GetCurrentBackBufferIndex();

	if (fenceValues[currentIndex] != 0) {
		fence->SetEventOnCompletion(fenceValues[currentIndex], fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	// Reset command allocator and command list
	commandAllocators[currentIndex]->Reset();

}
void ModuleD3D12::postRender()
{
	swapChain->Present(1, 0);

	// Signal and increment the fence value
	commandQueue->Signal(fence.Get(), ++fenceValue);
	currentIndex = swapChain->GetCurrentBackBufferIndex();
	fenceValues[currentIndex] = fenceValue;
}

void ModuleD3D12::resize()
{
	unsigned width, height;
	getWindowSize(width, height);
	if (width != windowWidth || height != windowHeight)
	{
		windowWidth = std::max(1u, width);
		windowHeight = std::max(1u, height);

		flushCommandQueue();

		// Release the previous resources we will be recreating
		for (UINT i = 0; i < FRAMES_IN_FLIGHT; i++)
		{
			renderTargets[i].Reset();
			fenceValues[i] = fenceValue;
		}
		depthStencilBuffer.Reset();

		// Resize the swap chain
		swapChain->ResizeBuffers(FRAMES_IN_FLIGHT, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

		createRenderTargets();
		createDepthStencil();
	}

}
void ModuleD3D12::getWindowSize(unsigned& windowWidth, unsigned& windowHeight) const
{
	RECT rect;
	GetClientRect(m_hWnd, &rect);
	windowWidth = rect.right - rect.left;
	windowHeight = rect.bottom - rect.top;
}

void ModuleD3D12::enableDebugLayer() const
{
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugInterface;
	D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
	debugInterface->EnableDebugLayer();
#endif
}

bool ModuleD3D12::createDevice()
{
#if defined(_DEBUG)
	CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory));
#else
	CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
#endif

	ComPtr<IDXGIAdapter4> adapter;
	bool ok = SUCCEEDED(factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)));
	ok = ok && SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));

	// Device InfoQueue for Debugging
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> infoQueue;
	device.As(&infoQueue);
	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
#endif

	return ok;
}

bool ModuleD3D12::createCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	bool ok = SUCCEEDED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

	return ok;
}
bool ModuleD3D12::createSwapChain()
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = 0;
	swapChainDesc.Height = 0;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = FRAMES_IN_FLIGHT;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = 0;

	ComPtr<IDXGISwapChain1> tempSwapChain;
	bool ok = SUCCEEDED(factory->CreateSwapChainForHwnd(
		commandQueue.Get(),
		m_hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&tempSwapChain));
	ok = ok && SUCCEEDED(tempSwapChain.As(&swapChain));

	return ok;
}
bool ModuleD3D12::createRenderTargets()
{
	// Create Heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FRAMES_IN_FLIGHT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	bool ok = SUCCEEDED(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

	// Create Render Target Views
	if (ok)
	{
		UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();

		for (UINT i = 0; i < FRAMES_IN_FLIGHT; i++)
		{
			ok = SUCCEEDED(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
			if (!ok) break;
			device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);
			rtvHandle.ptr += rtvDescriptorSize;
		}
	}

	return true;
}
bool ModuleD3D12::createDepthStencil()
{
	// Create Depth Stencil Buffer
	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, windowWidth, windowHeight,
		1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	bool ok = SUCCEEDED(device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&depthStencilBuffer)
	));

	// Create Heap for Depth Stencil View
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ok = ok && SUCCEEDED(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));

	// Create Depth Stencil View 
	device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, dsvHeap->GetCPUDescriptorHandleForHeapStart());

	return ok;
}
bool ModuleD3D12::createCommandAllocatorsAndLists()
{
	// Create Command Allocators
	for (UINT i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		bool ok = SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i])));
		if (!ok) return false;
	}

	// Create Command List
	bool ok = SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&commandList)));
	ok = ok && SUCCEEDED(commandList->Close());

	return true;
}
bool ModuleD3D12::createFences()
{
	// Create Fence
	bool ok = SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE ModuleD3D12::getRenderTargetDescriptor() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		INT(currentIndex),
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
	);
}
D3D12_CPU_DESCRIPTOR_HANDLE ModuleD3D12::getDepthStencilDescriptor() const
{
	return dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void ModuleD3D12::flushCommandQueue()
{
	commandQueue->Signal(fence.Get(), ++fenceValues[currentIndex]);
	fence->SetEventOnCompletion(fenceValues[currentIndex], fenceEvent);
	WaitForSingleObject(fenceEvent, INFINITE);
}