#pragma once
#include "Engine/RendererDX12/D3D12Common.hpp"
#include <stdint.h>

//--------------------------------------------------------------------------------------------------------------------------------------------
	
struct ID3D12DescriptorHeap;
class  RenderContextDX12;

//--------------------------------------------------------------------------------------------------------------------------------------------
	
class DescriptorHeapDX12
{
public:
	DescriptorHeapDX12( RenderContextDX12* owner , D3D12_DESCRIPTOR_HEAP_TYPE type , uint32_t numDescriptors );
	~DescriptorHeapDX12();

public:
	RenderContextDX12*		m_owner				= nullptr;
	ID3D12DescriptorHeap*	m_descriptorHeap	= nullptr;
};

//--------------------------------------------------------------------------------------------------------------------------------------------