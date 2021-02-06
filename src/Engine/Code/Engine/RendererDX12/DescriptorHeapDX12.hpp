#pragma once
#include "Engine/RendererDX12/D3D12Common.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------
	
class DescriptorHeapDX12
{
public:
	DescriptorHeapDX12();
	~DescriptorHeapDX12();

private:
	ID3D12DescriptorHeap* m_commandAllocator = nullptr;
};

//--------------------------------------------------------------------------------------------------------------------------------------------