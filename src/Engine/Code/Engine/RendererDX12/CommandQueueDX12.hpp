#pragma once
#include "Engine/RendererDX12/CommandListDX12.hpp"
#include "Engine/RendererDX12/FenceDX12.hpp"
#include "Engine/RendererDX12/D3D12Utils.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

class  RenderContextDX12;
class  FenceDX12;
struct ID3D12CommandQueue;

//--------------------------------------------------------------------------------------------------------------------------------------------
	
class CommandQueueDX12
{
public:
	CommandQueueDX12( RenderContextDX12* owner , eDX12CommandListType type );
	~CommandQueueDX12();

	void		ExecuteCommandLists( UINT NumCommandLists , CommandListDX12* const* commandLists );
	uint64_t	SignalFence( uint64_t& fenceValue );

public:
	RenderContextDX12*		m_owner;
	FenceDX12*				m_fence;
	ID3D12CommandQueue*		m_commandQueue = nullptr;
};

//--------------------------------------------------------------------------------------------------------------------------------------------