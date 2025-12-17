#include "Globals.h"
#include "Exercise3Camera.h"
#include "Application.h"
#include "ModuleResources.h"
#include "ModuleCamera.h"
#include <ReadData.h>

Exercise3Camera::Exercise3Camera()
{
}
Exercise3Camera::~Exercise3Camera()
{
}

bool Exercise3Camera::init()
{
	bool ok = createVertexBuffer();
	ok = ok && createRootSignature();
	ok = ok && createPipelineStateObject();

	// Create Debug Draw Pass
	if (ok)
	{
		ModuleD3D12* d3d12 = app->getD3D12();
		debugDrawPass = std::make_unique<DebugDrawPass>(d3d12->getDevice(), d3d12->getCommandQueue());
	}


	return true;
}

void Exercise3Camera::render()
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

	// Get transformation matrices
	Matrix model = Matrix::Identity;
	Matrix view = *app->getCamera()->GetViewMatrix();
	Matrix proj = *app->getCamera()->GetProjectionMatrix();	

	// Record commands
	// Clear the render target
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = d3d12->getRenderTargetDescriptor();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = d3d12->getDepthStencilDescriptor();
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	const FLOAT clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Draw triangle
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

	Matrix mvp = (model * view * proj).Transpose(); // Transpose because HLSL expects column-major matrices by default
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(UINT32), &mvp, 0);

	commandList->DrawInstanced(3, 1, 0, 0);

	// Draw debug primitives
	dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);  // Grid plane
	dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 1.0f);  // XYZ axis

	debugDrawPass->record(commandList, windowWidth, windowHeight, view, proj);

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

bool Exercise3Camera::createVertexBuffer()
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
bool Exercise3Camera::createRootSignature()
{
	ID3D12Device* device = app->getD3D12()->getDevice();

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};

	// Define a single root parameter as a set of 16 32-bit constants (a 4x4 matrix)
	CD3DX12_ROOT_PARAMETER rootParameters;
	rootParameters.InitAsConstants(sizeof(Matrix) / sizeof(UINT32), 0); // number of 32 bit elements in a matrix

	// Create the root signature with the root parameter
	rootSignatureDesc.Init(1, &rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> rootSignatureBlob;

	bool ok = SUCCEEDED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, nullptr));

	ok = ok && SUCCEEDED(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

	return ok;
}
bool Exercise3Camera::createPipelineStateObject()
{
	ID3D12Device* device = app->getD3D12()->getDevice();

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = { {"MY_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} };

	auto dataVS = DX::ReadData(L"Exercise3VS.cso");
	auto dataPS = DX::ReadData(L"Exercise3PS.cso");

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
