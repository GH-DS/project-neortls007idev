#include "Engine/RendererDX12/SwapChainDX12.hpp"
#include "Engine/RendererDX12/RenderContextDX12.hpp"
#include "Engine/Core/EngineCommon.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

SwapChainDX12::SwapChainDX12( RenderContextDX12* owner , IDXGISwapChain4* handle ) : 
																		m_owner( owner ) ,
																		m_handle( handle )
{
}

//--------------------------------------------------------------------------------------------------------------------------------------------

SwapChainDX12::~SwapChainDX12()
{
	// delete m_backBuffer;
	// m_backBuffer = nullptr;
	// 
	// DX_SAFE_RELEASE( m_handle );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void SwapChainDX12::Present( int vsync )
{
	UNUSED( vsync );
	// m_handle->Present( vsync , 0 );
}

//--------------------------------------------------------------------------------------------------------------------------------------------
