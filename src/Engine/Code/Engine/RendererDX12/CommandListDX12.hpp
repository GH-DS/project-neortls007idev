#pragma once
#include "Engine/RendererDX12/D3D12Common.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------
	
struct ID3D12PipelineState;
struct ID3D12GraphicsCommandList;
class  RenderContextDX12;
class  CommandAllocatorDX12;

//--------------------------------------------------------------------------------------------------------------------------------------------
	
class CommandListDX12
{
public:
	CommandListDX12( RenderContextDX12* owner , CommandAllocatorDX12* commandAllocator , ID3D12PipelineState* pipelineState , D3D12_COMMAND_LIST_TYPE type );
	~CommandListDX12();

public:
	RenderContextDX12*			m_owner			= nullptr;
	ID3D12GraphicsCommandList*	m_commandList	= nullptr;
};

//--------------------------------------------------------------------------------------------------------------------------------------------