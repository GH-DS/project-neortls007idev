#include "Engine/RendererDX12/DescriptorHeapDX12.hpp"
#include "Engine/RendererDX12/RenderContextDX12.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/D3D11Common.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

DescriptorHeapDX12::DescriptorHeapDX12( RenderContextDX12* owner , D3D12_DESCRIPTOR_HEAP_TYPE type , uint32_t numDescriptors ) : 
																																 m_owner( owner )
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type = type;

	GUARANTEE_OR_DIE( owner->m_device->CreateDescriptorHeap( &desc , IID_PPV_ARGS( &m_descriptorHeap ) ) == S_OK , "Descriptor Heap Creation Failed" );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

DescriptorHeapDX12::~DescriptorHeapDX12()
{
	DX_SAFE_RELEASE( m_descriptorHeap );
}

//--------------------------------------------------------------------------------------------------------------------------------------------
