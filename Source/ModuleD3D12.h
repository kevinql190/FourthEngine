#pragma once
#include "Module.h"
#include <dxgi1_6.h>

class ModuleD3D12 : public Module
{
public:
	ModuleD3D12(HWND hWnd);
	~ModuleD3D12();

	HWND							getHWnd() const { return m_hWnd; }
	ID3D12Device5*					getDevice() const { return device.Get(); }
	ID3D12GraphicsCommandList4*		getCommandList() const { return commandList.Get(); }
	ID3D12Resource*					getCurrentBackBuffer() const { return renderTargets[currentIndex].Get(); }
	ID3D12CommandAllocator*			getCommandAllocator() const { return commandAllocators[currentIndex].Get(); }
	ID3D12CommandQueue*				getCommandQueue() const { return commandQueue.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE     getRenderTargetDescriptor() const;
	D3D12_CPU_DESCRIPTOR_HANDLE     getDepthStencilDescriptor() const;

	unsigned getWindowWidth() const { return windowWidth; }
	unsigned getWindowHeight() const { return windowHeight; }
	void resize();

	void flushCommandQueue();

	bool init() override;
	void preRender() override;
	void postRender() override;

private:
	HWND m_hWnd;
	void getWindowSize(unsigned& windowWidth, unsigned& windowHeight) const;
	unsigned windowWidth = 0;
	unsigned windowHeight = 0;

	void enableDebugLayer() const;
	bool createDevice();
	bool createCommandQueue();
	bool createSwapChain();
	bool createRenderTargets();
	bool createDepthStencil();
	bool createFences();
	bool createCommandAllocatorsAndLists();
	
	ComPtr<IDXGIFactory6>				factory;
	ComPtr<ID3D12Device5>				device;

	ComPtr<ID3D12CommandQueue>			commandQueue;
	ComPtr<IDXGISwapChain4>				swapChain;
	ComPtr<ID3D12Fence>					fence;
	UINT64 fenceValue = 0;
	UINT64 fenceValues[FRAMES_IN_FLIGHT] = {};
	UINT64 currentIndex = 0;
	HANDLE fenceEvent;

	ComPtr<ID3D12Resource>				renderTargets[FRAMES_IN_FLIGHT];
	ComPtr<ID3D12DescriptorHeap>		rtvHeap;
	ComPtr<ID3D12Resource>				depthStencilBuffer;
	ComPtr<ID3D12DescriptorHeap>		dsvHeap;

	ComPtr<ID3D12CommandAllocator>		commandAllocators[FRAMES_IN_FLIGHT];
	ComPtr<ID3D12GraphicsCommandList4>	commandList;

};

