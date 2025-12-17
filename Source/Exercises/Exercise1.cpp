#include "Globals.h"
#include "Exercise1.h"

Exercise1::Exercise1()
{
}
Exercise1::~Exercise1()
{
}

void Exercise1::render()
{
	ModuleD3D12* d3d12 = app->getD3D12();
	ID3D12GraphicsCommandList4* commandList = d3d12->getCommandList();
	commandList->Reset(d3d12->getCommandAllocator(), nullptr);

	// Transition back buffer from PRESENT to RENDER_TARGET
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		d3d12->getCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	commandList->ResourceBarrier(1, &barrier);

	// Record commands
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };

	commandList->ClearRenderTargetView(d3d12->getRenderTargetDescriptor(), clearColor, 0, nullptr);

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
