#pragma once
#include "Module.h"
#include "Model.h"
#include "DebugDrawPass.h"
#include "ImGuiPass.h"

class ModuleD3D12;

class Exercise5 : public Module
{
public:
	Exercise5();
	~Exercise5();
	bool init() override;
	void preRender() override;
	void render() override;

private:
	ModuleD3D12* d3d12 = nullptr;

	Model model;
	std::vector<ComPtr<ID3D12Resource>>	materialBuffers;

	ComPtr<ID3D12RootSignature>		rootSignature;
	ComPtr<ID3D12PipelineState>		pso;


	// ImGui
	std::unique_ptr<ImGuiPass>      imguiPass;
	std::unique_ptr<DebugDrawPass>  debugDrawPass;

	bool showGrid = true;
	bool showAxis = true;
	std::vector<float> fpsHistory;
	int fpsOffset = 0;
	int maxFPSHistory = 60;
	int sampler = 0;

	void imGuiCommands();
	
	bool loadModel();
	bool createRootSignature();
	bool createPipelineStateObject();
};

