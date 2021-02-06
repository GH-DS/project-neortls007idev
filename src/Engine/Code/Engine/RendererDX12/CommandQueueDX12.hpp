#pragma once
#include "Engine/RendererDX12/CommandListDX12.hpp"
#include "Engine/RendererDX12/FenceDX12.hpp"
#include "Engine/RendererDX12/D3D12Utils.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------
	
class CommandQueueDX12
{
public:
	CommandQueueDX12( ID3D12Device2* device , eDX12CommandListType type );
	~CommandQueueDX12();

	void ExecuteCommandList( CommandListDX12 list );
private:
	FenceDX12				m_fence;
	ID3D12CommandQueue*		m_commandQueue = nullptr;
};

//--------------------------------------------------------------------------------------------------------------------------------------------