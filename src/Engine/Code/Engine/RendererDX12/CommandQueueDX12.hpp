#pragma once
#include "Engine/RendererDX12/CommandListDX12.hpp"
#include "Engine/RendererDX12/FenceDX12.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------
	
class CommandQueueDX12
{
public:
	CommandQueueDX12();
	~CommandQueueDX12();

	void ExecuteCommandList( CommandListDX12 list );
private:
	FenceDX12 m_fence;
};

//--------------------------------------------------------------------------------------------------------------------------------------------