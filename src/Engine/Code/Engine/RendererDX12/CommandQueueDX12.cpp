#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/RendererDX12/CommandQueueDX12.hpp"
#include "Engine/RendererDX12/D3D12Common.hpp"
#include "Engine/Renderer/D3D11Common.hpp"
#include "Engine/RendererDX12/RenderContextDX12.hpp"
#include "Engine/RendererDX12/FenceDX12.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

CommandQueueDX12::CommandQueueDX12( RenderContextDX12* owner , eDX12CommandListType type ) : 
																							 m_owner( owner )
{

		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = GetD3D12CommandListType( type );
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;

		HRESULT result = m_owner->m_device->CreateCommandQueue( &desc , IID_PPV_ARGS( &m_commandQueue ) );
		GUARANTEE_OR_DIE( S_OK == result , "Command Queue Creation Failed" );

		m_fence = new FenceDX12( this , owner );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

CommandQueueDX12::~CommandQueueDX12()
{
	SAFE_RELEASE_POINTER( m_fence );
	DX_SAFE_RELEASE( m_commandQueue );
	m_owner = nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------- 
