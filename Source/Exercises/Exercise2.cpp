#include "Globals.h"
#include "Exercise2.h"
#include <ReadData.h>

Exercise2::Exercise2()
{
}
Exercise2::~Exercise2()
{
}

bool Exercise2::init()
{
	bool ok = createVertexBuffer();
	ok = ok && createRootSignature();
	ok = ok && createPipelineStateObject();

	return true;
}

void Exercise2::render()
{
	ModuleD3D12* d3d12 = app->getD3D12();
	ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();
	commandList->Reset(d3d12->getCommandAllocator(), pso.Get());

	// Transition back buffer from PRESENT to RENDER_TARGET
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		d3d12->getCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	commandList->ResourceBarrier(1, &barrier);

	// Set the viewport and scissor rect
	LONG windowWidth = (LONG)d3d12->getWindowWidth();
	LONG windowHeight = (LONG)d3d12->getWindowHeight();

	D3D12_VIEWPORT viewport{ 0.0, 0.0, float(windowWidth),  float(windowHeight) , 0.0, 1.0 };
	D3D12_RECT scissor{ 0, 0, windowWidth, windowHeight };

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissor);

	// Record commands
	// Clear the render target
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = d3d12->getRenderTargetDescriptor();
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	const FLOAT clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// Draw triangle
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->DrawInstanced(3, 1, 0, 0);

	// Transition back buffer from RENDER_TARGET to PRESENT
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		d3d12->getCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	commandList->ResourceBarrier(1, &barrier);

	commandList->Close();
	ID3D12CommandList* listsToExecute[] = { commandList };
	d3d12->getCommandQueue()->ExecuteCommandLists(UINT(std::size(listsToExecute)), listsToExecute);
}

bool Exercise2::createVertexBuffer()
{
	struct Vertex
	{
		float x, y, z;
	};
	Vertex vertices[3] =
	{
		{-1.0f, -1.0f, 0.0f },  // 0
		{ 0.0f, 1.0f, 0.0f  },  // 1
		{ 1.0f, -1.0f, 0.0f }   // 2
	};

	// Create Vertex Buffer
	vertexBuffer = app->getResources()->createDefaultBuffer(vertices, sizeof(vertices));

	// Create Vertex Buffer View
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	vertexBufferView.SizeInBytes = sizeof(vertices);

	return true;
}
bool Exercise2::createRootSignature()
{
	ID3D12Device* device = app->getD3D12()->getDevice();
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> serializedRootSig;

	bool ok = SUCCEEDED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, nullptr));

	ok = ok && SUCCEEDED(device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

	return ok;
}
bool Exercise2::createPipelineStateObject()
{
	ID3D12Device* device = app->getD3D12()->getDevice();

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {{"MY_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

	auto dataVS = DX::ReadData(L"Exercise2VS.cso");
	auto dataPS = DX::ReadData(L"Exercise2PS.cso");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.InputLayout = { inputLayout, sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC) };

	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.NumRenderTargets = 1;
	psoDesc.SampleDesc = { 1, 0 };
	psoDesc.SampleMask = 0xffffffff;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	psoDesc.VS = { dataVS.data(), dataVS.size() };
	psoDesc.PS = { dataPS.data(), dataPS.size() };

	bool ok = SUCCEEDED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));

	return ok;
}
