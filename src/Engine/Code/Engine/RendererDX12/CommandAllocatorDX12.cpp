#include "Engine/RendererDX12/CommandAllocatorDX12.hpp"
#include "Engine/Renderer/D3D11Common.hpp"
#include "Engine/RendererDX12/RenderContextDX12.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

CommandAllocatorDX12::CommandAllocatorDX12( RenderContextDX12* owner , D3D12_COMMAND_LIST_TYPE type ) : 
																										m_owner( owner )
{
	GUARANTEE_OR_DIE( owner->m_device->CreateCommandAllocator( type , IID_PPV_ARGS( &m_commandAllocator ) ) == S_OK , "Command ALlocator Creation Failed" );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

CommandAllocatorDX12::~CommandAllocatorDX12()
{
	DX_SAFE_RELEASE( m_commandAllocator );
	m_owner = nullptr;
}
