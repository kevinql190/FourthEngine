#include "Globals.h"
#include "Exercise5.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleCamera.h"
#include <ReadData.h>

Exercise5::Exercise5()
{
}

Exercise5::~Exercise5()
{
}

bool Exercise5::init()
{
	bool ok = loadModel();
	ok = ok && createRootSignature();
	ok = ok && createPipelineStateObject();

	// Create Debug Draw Pass
	if (ok)
	{
		d3d12 = app->getD3D12();

		debugDrawPass = std::make_unique<DebugDrawPass>(d3d12->getDevice(), d3d12->getCommandQueue());
		imguiPass = std::make_unique<ImGuiPass>(app->getD3D12()->getDevice(), app->getD3D12()->getHWnd());
		fpsHistory.resize(maxFPSHistory, 0.0f);
	}

	return ok;
}

void Exercise5::preRender()
{
	imguiPass->startFrame();
	ImGuizmo::BeginFrame();

	// Set ImGuizmo rect based on window size
	unsigned width = d3d12->getWindowWidth();
	unsigned height = d3d12->getWindowHeight();
	ImGuizmo::SetRect(0, 0, float(width), float(height));
}

void Exercise5::render()
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

	// Record commands
	// Clear the render target
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = d3d12->getRenderTargetDescriptor();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = d3d12->getDepthStencilDescriptor();
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	const FLOAT clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Draw model
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Get transformation matrices
	Matrix modelM = model.getModelMatrix();
	Matrix view = app->getCamera()->GetViewMatrix();
	Matrix proj = app->getCamera()->GetProjectionMatrix();
	Matrix mvp = (modelM * view * proj).Transpose(); // Transpose because HLSL expects column-major matrices by default
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(UINT32), &mvp, 0);

	// Set texture descriptor heap and SRV
	ModuleSamplers* samplers = app->getSamplers();
	ModuleShaderDescriptors* shaderDescriptors = app->getShaderDescriptors();
	ID3D12DescriptorHeap* descriptorHeaps[] = { shaderDescriptors->getHeap(), samplers->getHeap() };
	commandList->SetDescriptorHeaps(2, descriptorHeaps);


	// Draw meshes
	for (Mesh* mesh : model.getMeshes())
	{
		// Set vertex and index buffers if present
		commandList->IASetVertexBuffers(0, 1, &mesh->getVertexBufferView());

		// Get material and set material constant buffer and texture SRV
		Material* material = model.getMaterials()[mesh->getMaterialIndex()];
		MaterialData matData = material->getData();

		commandList->SetGraphicsRootConstantBufferView(1, materialBuffers[mesh->getMaterialIndex()]->GetGPUVirtualAddress());
		commandList->SetGraphicsRootDescriptorTable(2, material->getTextureGPUHandle());
		// Set sampler
		commandList->SetGraphicsRootDescriptorTable(3, samplers->getGPUHandle(ModuleSamplers::Type(sampler)));

		// Draw call

		if (mesh->hasIndexBuffer())
		{
			commandList->IASetIndexBuffer(&mesh->getIndexBufferView());
			commandList->DrawIndexedInstanced(mesh->getIndexCount(), 1, 0, 0, 0);
		}
		else
		{
			commandList->DrawInstanced(mesh->getVertexCount(), 1, 0, 0);
		}
	}

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

void Exercise5::imGuiCommands()
{
	ImGui::ShowDemoWindow();


	if (ImGui::Begin("Scene options"))
	{
		// Grid and axis toggles
		ImGui::Checkbox("Show Grid", &showGrid);
		ImGui::Checkbox("Show Axis", &showAxis);
		// Model and material
		ImGui::Separator();
		ImGui::Text("Model has %d meshes and %d materials", (int)model.getMeshes().size(), (int)model.getMaterials().size());
		for (const Mesh* mesh : model.getMeshes())
		{
			ImGui::Text("Mesh %d: %d vertices, %d indices, material %d",
				mesh->getMaterialIndex(),
				mesh->getVertexCount(),
				mesh->hasIndexBuffer() ? mesh->getIndexCount() : 0,
				mesh->getMaterialIndex());
		}
		ImGui::Checkbox("Show guizmo", &showGuizmo);

		Matrix modelM = model.getModelMatrix();

		// Manipulate model matrix by components
		float translation[3], rotation[3], scale[3];
		ImGuizmo::DecomposeMatrixToComponents((float*)&modelM, translation, rotation, scale);
		bool transform_changed = ImGui::DragFloat3("Tr##translate", translation, 0.1f);
		transform_changed = transform_changed || ImGui::DragFloat3("Rt##rotation", rotation, 0.1f);
		transform_changed = transform_changed || ImGui::DragFloat3("Sc##scale", scale, 0.1f);
		if (ImGui::Button("Reset Transform"))
		{
			translation[0] = translation[1] = translation[2] = 0.0f;
			rotation[0] = rotation[1] = rotation[2] = 0.0f;
			scale[0] = scale[1] = scale[2] = 1.0f;
			transform_changed = true;
		}
		if (transform_changed)
		{
			ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, (float*)&modelM);

			model.setModelMatrix(modelM);
		}

		// Guizmo to manipulate model matrix
		if (showGuizmo)
		{
			// Guizmo operation radio buttons
			ImGui::RadioButton("Translate", (int*)&gizmoOperation, (int)ImGuizmo::TRANSLATE);
			ImGui::SameLine();
			ImGui::RadioButton("Rotate", (int*)&gizmoOperation, ImGuizmo::ROTATE);
			ImGui::SameLine();
			ImGui::RadioButton("Scale", (int*)&gizmoOperation, ImGuizmo::SCALE);
			// Change guizmo operation with keyboard shortcuts
			if (ImGui::IsKeyPressed(ImGuiKey_W)) gizmoOperation = ImGuizmo::TRANSLATE;
			if (ImGui::IsKeyPressed(ImGuiKey_R)) gizmoOperation = ImGuizmo::ROTATE;
			if (ImGui::IsKeyPressed(ImGuiKey_E)) gizmoOperation = ImGuizmo::SCALE;

			// Manipulate model matrix with guizmo
			ImGuizmo::Manipulate(
				(const float*)&app->getCamera()->GetViewMatrix(),
				(const float*)&app->getCamera()->GetProjectionMatrix(),
				gizmoOperation,
				ImGuizmo::LOCAL,
				(float*)&modelM
			);
		}
		model.setModelMatrix(modelM); // Update model matrix after Manipulate()

		// Focus camera on model position when Shift+F is pressed
		if (ImGui::IsKeyPressed(ImGuiKey_F))
			app->getCamera()->focusOnPosition(Vector3(translation[0], translation[1], translation[2]), Vector3(scale[0], scale[1], scale[2]));

		ImGui::End();
	}

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

bool Exercise5::loadModel()
{
	//bool ok = model.loadFromFile("Assets/Models/Duck/duck.gltf", "Assets/Models/Duck/");
	bool ok = model.loadFromFile("Assets/Models/BoxTextured/BoxTextured.gltf", "Assets/Models/BoxTextured/");
	//bool ok = model.loadFromFile("Assets/Models/BoxInterleaved/BoxInterleaved.gltf", "Assets/Models/BoxInterleaved/");

	ModuleResources* resources = app->getResources();

	// Load materials textures
	std::vector<Material*> materials = model.getMaterials();
	for (int i = 0; i < materials.size(); ++i)
	{
		Material* mat = materials[i];
		MaterialData matData = mat->getData();

		materialBuffers.push_back(resources->createDefaultBuffer(&matData, alignUp(sizeof(MaterialData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)));
	}

	return ok;
}

bool Exercise5::createRootSignature()
{
	ID3D12Device* device = app->getD3D12()->getDevice();

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	CD3DX12_ROOT_PARAMETER rootParameters[4] = {};

	// Descriptor ranges
	CD3DX12_DESCRIPTOR_RANGE srvRange, sampRange;
	srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 4, 0);

	// Root parameters
	rootParameters[0].InitAsConstants(sizeof(Matrix) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // 1 constant buffer for MVP matrix
	rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_PIXEL);					// 1 constant buffer for MaterialData
	rootParameters[2].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);				// 1 descriptor table for SRV
	rootParameters[3].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);				// 1 descriptor table for Sampler

	// Create the root signature
	rootSignatureDesc.Init(4, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> rootSignatureBlob;

	bool ok = SUCCEEDED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, nullptr));

	ok = ok && SUCCEEDED(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

	return ok;
}
bool Exercise5::createPipelineStateObject()
{
	ID3D12Device* device = app->getD3D12()->getDevice();

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	auto dataVS = DX::ReadData(L"Exercise5VS.cso");
	auto dataPS = DX::ReadData(L"Exercise5PS.cso");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.InputLayout = { inputLayout, sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC) };

	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.NumRenderTargets = 1;
	psoDesc.SampleDesc = { 1, 0 };
	psoDesc.SampleMask = 0xffffffff;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	psoDesc.VS = { dataVS.data(), dataVS.size() };
	psoDesc.PS = { dataPS.data(), dataPS.size() };

	bool ok = SUCCEEDED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));

	return ok;
}