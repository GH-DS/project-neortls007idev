#pragma once
#include "Engine/RendererDX12/D3D12Common.hpp"

class	RenderContextDX12;
struct	ID3D12CommandAllocator;
//--------------------------------------------------------------------------------------------------------------------------------------------
	
class CommandAllocatorDX12
{
public:
	CommandAllocatorDX12( RenderContextDX12* owner , D3D12_COMMAND_LIST_TYPE type );
	~CommandAllocatorDX12();

public:
	RenderContextDX12*		m_owner				= nullptr;
	ID3D12CommandAllocator* m_commandAllocator	= nullptr;
};

//--------------------------------------------------------------------------------------------------------------------------------------------