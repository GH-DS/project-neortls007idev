#include "Engine/RendererDX12/FenceDX12.hpp"
#include "Engine/RendererDX12/RenderContextDX12.hpp"
#include "Engine/RendererDX12/CommandQueueDX12.hpp"
#include "Engine/Renderer/D3D11Common.hpp"
#include <concrt.h>

//--------------------------------------------------------------------------------------------------------------------------------------------

FenceDX12::FenceDX12( CommandQueueDX12* owner , RenderContextDX12* deviceOwner ) : 
	m_ownerQueue( owner ) , 
	m_deviceOwner( deviceOwner )
{
	deviceOwner->m_device->CreateFence( 0 , D3D12_FENCE_FLAG_NONE , IID_PPV_ARGS( &m_fence ) );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

FenceDX12::~FenceDX12()
{
	DX_SAFE_RELEASE( m_fence );
	m_ownerQueue = nullptr;
	m_deviceOwner = nullptr;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void FenceDX12::WaitForFenceValue( uint64_t fenceValue , void* fenceEvent , std::chrono::milliseconds duration /*= std::chrono::milliseconds::max() */ )
{
	if ( m_fence->GetCompletedValue() < fenceValue )
	{
		m_fence->SetEventOnCompletion( fenceValue , reinterpret_cast< HANDLE >( fenceEvent ) );
		::WaitForSingleObject( reinterpret_cast< HANDLE >( fenceEvent ) , static_cast< DWORD >( duration.count() ) );
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------