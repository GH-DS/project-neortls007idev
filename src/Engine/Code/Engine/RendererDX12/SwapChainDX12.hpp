#pragma once

//--------------------------------------------------------------------------------------------------------------------------------------------
	
class	RenderContextDX12;
struct  ID3D12Resource;
struct  IDXGISwapChain4;

//--------------------------------------------------------------------------------------------------------------------------------------------

class SwapChainDX12
{
public:
	SwapChainDX12( RenderContextDX12* owner, IDXGISwapChain4* handle );
	~SwapChainDX12();

	void Present( int vsync = 0 );
	
private:
	RenderContextDX12*		m_owner			= nullptr; // creator render context
	IDXGISwapChain4*		m_handle		= nullptr; // D3D11 created pointer, what we use when calling DXD311
	ID3D12Resource*			m_backbuffer	= nullptr;
};

//--------------------------------------------------------------------------------------------------------------------------------------------