#pragma once
#include "Module.h"
#include "DebugDrawPass.h"
#include "ModuleCamera.h"

class Exercise3Camera : public Module
{
public:
	Exercise3Camera();
	~Exercise3Camera();
	bool init() override;
	void render() override;

private:
	ComPtr<ID3D12Resource>			vertexBuffer;
	ComPtr<ID3D12RootSignature>		rootSignature;
	ComPtr<ID3D12PipelineState>		pso;
	D3D12_VERTEX_BUFFER_VIEW		vertexBufferView;
	std::unique_ptr<DebugDrawPass>  debugDrawPass;

	bool createVertexBuffer();
	bool createRootSignature();
	bool createPipelineStateObject();
};

