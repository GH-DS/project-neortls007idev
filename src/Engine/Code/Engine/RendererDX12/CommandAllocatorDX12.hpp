#pragma once
#include "Engine/RendererDX12/D3D12Common.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------
	
class CommandListDX12
{
public:
	CommandListDX12();
	~CommandListDX12();

private:
	ID3D12CommandAllocator* m_commandAllocator = nullptr;
};

//--------------------------------------------------------------------------------------------------------------------------------------------