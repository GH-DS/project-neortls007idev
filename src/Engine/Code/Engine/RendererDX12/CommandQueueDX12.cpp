#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/RendererDX12/CommandQueueDX12.hpp"
#include "Engine/RendererDX12/D3D12Common.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

CommandQueueDX12::CommandQueueDX12( ID3D12Device2* device , eDX12CommandListType type )
{

		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = GetD3D12CommandListType( type );
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;

		HRESULT result =  device->CreateCommandQueue( &desc , IID_PPV_ARGS( &m_commandQueue ) );
		GUARANTEE_OR_DIE( S_OK == result , "Command Queue Creation Failed" );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

CommandQueueDX12::~CommandQueueDX12()
{

}
