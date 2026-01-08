#include "Globals.h"
#include "Exercise4Plus.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleCamera.h"
#include <ReadData.h>

Exercise4Plus::Exercise4Plus()
{
}
Exercise4Plus::~Exercise4Plus()
{
}

bool Exercise4Plus::init()
{
	bool ok = createVertexBuffer();
	ok = ok && createRootSignature();
	ok = ok && createPipelineStateObject();

	// Create Debug Draw Pass
	if (ok)
	{
		d3d12 = app->getD3D12();
		ModuleResources* resources = app->getResources();

		texture = resources->createTextureFromFile(std::wstring(L"Assets/dog.dds"));

		// Create SRV Descriptor Heap
		ModuleShaderDescriptors* shaderDescriptors = app->getShaderDescriptors();
		UINT dogTable = shaderDescriptors->allocTable();
		shaderDescriptors->createTextureSRV(dogTable, texture.Get(), 0);
		textureGPUHandle = shaderDescriptors->getGPUDescriptorHandle(dogTable, 0);

		debugDrawPass = std::make_unique<DebugDrawPass>(d3d12->getDevice(), d3d12->getCommandQueue());
		imguiPass = std::make_unique<ImGuiPass>(app->getD3D12()->getDevice(), app->getD3D12()->getHWnd());
		fpsHistory.resize(maxFPSHistory, 0.0f);
	}

	return true;
}

void Exercise4Plus::preRender()
{
	imguiPass->startFrame();
}

void Exercise4Plus::render()
{
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
	Matrix mvp = (model * view * proj).Transpose(); // Transpose because HLSL expects column-major matrices by default

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

	// Set texture descriptor heap and SRV
	ModuleSamplers* samplers = app->getSamplers();
	ModuleShaderDescriptors* shaderDescriptors = app->getShaderDescriptors();
	ID3D12DescriptorHeap* descriptorHeaps[] = { shaderDescriptors->getHeap(), samplers->getHeap()};
	commandList->SetDescriptorHeaps(2, descriptorHeaps);

	commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(UINT32), &mvp, 0);
	commandList->SetGraphicsRootDescriptorTable(1, textureGPUHandle);
	commandList->SetGraphicsRootDescriptorTable(2, samplers->getGPUHandle(ModuleSamplers::Type(sampler)));

	commandList->DrawInstanced(6, 1, 0, 0);

	// Draw debug primitives
	if (showGrid) dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);  // Grid plane
	if (showAxis) dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 1.0f);  // XYZ axis

	debugDrawPass->record(commandList, windowWidth, windowHeight, view, proj);
	imGuiCommands();
	imguiPass->record(d3d12->getCommandList());

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

void Exercise4Plus::imGuiCommands()
{
	ImGui::ShowDemoWindow();
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("About"))
	{
		ImGui::Text("Engine Name: Dream Engine");
		ImGui::Text("Description: Educational Engine for the Master Degree");
		ImGui::Text("Name Author: Kevin Qiu");
		ImGui::Text("Libraries: DirectX 12, ImGui...");
		ImGui::Text("License: MIT License");
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Settings"))
	{
		// Grid and axis toggles
		ImGui::Checkbox("Show Grid", &showGrid);
		ImGui::Checkbox("Show Axis", &showAxis);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Camera Instructions"))
	{
		ImGui::Text("Right drag to rotate camera");
		ImGui::Text("WASDEQ with right click pressed to move camera");
		ImGui::Text("Wheel to zoom in/out");
		ImGui::Text("Wheel click and drag to pan camera");
		ImGui::Text("Alt + left click and drag to orbit around target");
		ImGui::Text("F to focus on current focus point and reset zoom");
		ImGui::Text("Shift + F to refocus to origin and reset zoom");
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Sampler"))
	{
		ImGui::Combo("Sampler", &sampler, "Linear/Wrap\0Point/Wrap\0Linear/Clamp\0Point/Clamp\0", 4);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("FPS"))
	{
		ImGui::SliderInt("Max FPS History", &maxFPSHistory, 10, 120);

		// Handle MaxFPSHistory resize
		static int lastMaxFPSHistory = maxFPSHistory;
		if (lastMaxFPSHistory != maxFPSHistory)
		{
			fpsHistory.clear();
			fpsHistory.resize(maxFPSHistory, 0.0f);
			fpsOffset = 0;
			lastMaxFPSHistory = maxFPSHistory;
		}

		// Update FPS history
		float currentFPS = app->getFPS();
		fpsHistory[fpsOffset] = currentFPS;
		fpsOffset = (fpsOffset + 1) % maxFPSHistory;

		// Histogram
		ImGui::Text("Framerate %.2f FPS", currentFPS);
		int count = (int)fpsHistory.size();
		ImGui::PlotHistogram(
			"##framerate",
			fpsHistory.data(),
			count,
			fpsOffset,
			nullptr,
			0.0f,    // min
			144.0f,  // max
			ImVec2(0, 80)
		);
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

bool Exercise4Plus::createVertexBuffer()
{
	struct Vertex
	{
		Vector3 position;
		Vector2 uv;
	};

	static Vertex vertices[6] =
	{
		{ Vector3(-1.0f, -1.0f, 0.0f),  Vector2(-0.2f, 1.2f) },
		{ Vector3(-1.0f, 1.0f, 0.0f),   Vector2(-0.2f, -0.2f) },
		{ Vector3(1.0f, 1.0f, 0.0f),    Vector2(1.2f, -0.2f) },
		{ Vector3(-1.0f, -1.0f, 0.0f),  Vector2(-0.2f, 1.2f) },
		{ Vector3(1.0f, 1.0f, 0.0f),    Vector2(1.2f, -0.2f) },
		{ Vector3(1.0f, -1.0f, 0.0f),   Vector2(1.2f, 1.2f) }
	};

	// Create Vertex Buffer
	vertexBuffer = app->getResources()->createDefaultBuffer(vertices, sizeof(vertices));

	// Create Vertex Buffer View
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	vertexBufferView.SizeInBytes = sizeof(vertices);

	return true;
}
bool Exercise4Plus::createRootSignature()
{
	ID3D12Device* device = app->getD3D12()->getDevice();

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	CD3DX12_ROOT_PARAMETER rootParameters[3] = {};

	// Descriptor ranges
	CD3DX12_DESCRIPTOR_RANGE srvRange, sampRange;
	srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 4, 0);

	// Root parameters
	rootParameters[0].InitAsConstants(sizeof(Matrix) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // 1 constant buffer for MVP matrix
	rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);				// 1 descriptor table for SRV
	rootParameters[2].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);				// 1 descriptor table for Sampler

	// Create the root signature
	rootSignatureDesc.Init(3, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> rootSignatureBlob;

	bool ok = SUCCEEDED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, nullptr));

	ok = ok && SUCCEEDED(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

	return ok;
}
bool Exercise4Plus::createPipelineStateObject()
{
	ID3D12Device* device = app->getD3D12()->getDevice();

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	auto dataVS = DX::ReadData(L"Exercise4VS.cso");
	auto dataPS = DX::ReadData(L"Exercise4PS.cso");

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