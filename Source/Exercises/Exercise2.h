#pragma once
#include "Module.h"
#include "Application.h"
#include "ModuleResources.h"

class Exercise2 : public Module
{
public:
	Exercise2();
	~Exercise2();
	bool init() override;
	void render() override;

private:
	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pso;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	bool createVertexBuffer();
	bool createRootSignature();
	bool createPipelineStateObject();
};

