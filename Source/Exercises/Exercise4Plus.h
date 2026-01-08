#pragma once
#include "Module.h"
#include "DebugDrawPass.h"
#include "ModuleCamera.h"
#include "ImGuiPass.h"

class ImGuiPass;
class ModuleD3D12;

class Exercise4Plus : public Module
{
public:
	Exercise4Plus();
	~Exercise4Plus();
	bool init() override;
	void preRender() override;
	void render() override;

private:
	ModuleD3D12* d3d12 = nullptr;
	std::unique_ptr<ImGuiPass>      imguiPass;

	bool showGrid = true;
	bool showAxis = true;
	std::vector<float> fpsHistory;
	int fpsOffset = 0;
	int maxFPSHistory = 60;
	int sampler = 0;

	ComPtr<ID3D12Resource>			vertexBuffer;
	ComPtr<ID3D12RootSignature>		rootSignature;
	ComPtr<ID3D12PipelineState>		pso;
	D3D12_VERTEX_BUFFER_VIEW		vertexBufferView;

	ComPtr<ID3D12Resource>          texture;
	D3D12_GPU_DESCRIPTOR_HANDLE		textureGPUHandle;
	std::unique_ptr<DebugDrawPass>  debugDrawPass;

	void imGuiCommands();

	bool createVertexBuffer();
	bool createRootSignature();
	bool createPipelineStateObject();
};

