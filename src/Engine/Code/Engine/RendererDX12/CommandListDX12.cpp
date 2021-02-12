#include "Engine/RendererDX12/CommandListDX12.hpp"
#include "Engine/RendererDX12/RenderContextDX12.hpp"
#include "Engine/RendererDX12/CommandAllocatorDX12.hpp"
#include "Engine/Renderer/D3D11Common.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

CommandListDX12::CommandListDX12( RenderContextDX12* owner , CommandAllocatorDX12* commandAllocator , D3D12_COMMAND_LIST_TYPE type ) :
																																		m_owner( owner )
{
	GUARANTEE_OR_DIE( owner->m_device->CreateCommandList( 0 , type , commandAllocator->m_commandAllocator , nullptr , IID_PPV_ARGS( &m_commandList ) ) == S_OK ,
		"Command List Creation Failed Check Call Stack" );
	
	GUARANTEE_OR_DIE( m_commandList->Close() == S_OK , "Initial closing of command list after creation FAILED" );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

CommandListDX12::~CommandListDX12()
{
	DX_SAFE_RELEASE( m_commandList );
	m_owner = nullptr;
}
