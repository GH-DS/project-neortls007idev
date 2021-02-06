#pragma once
#include "Engine/Platform/Window.hpp"
#include "Engine/RendererDX12/D3D12Utils.hpp"
#include <chrono>

//--------------------------------------------------------------------------------------------------------------------------------------------

struct	ID3D12Device;
struct	ID3D12DeviceContext;
struct	IDXGIDebug;
struct  ID3D12Resource;
struct  IDXGISwapChain4;
struct  ID3D12Device2;

class	CommandQueueDX12;

//--------------------------------------------------------------------------------------------------------------------------------------------
	
class RenderContextDX12
{

public:
	RenderContextDX12() {};
	~RenderContextDX12();

	void					Startup( Window* window );
	void					BeginFrame();
	void					UpdateFrameTime( float deltaSeconds );
	void					EndFrame();
	void					Shutdown();
	
//--------------------------------------------------------------------------------------------------------------------------------------------
//			CAMERA METHODS
//--------------------------------------------------------------------------------------------------------------------------------------------
	
	void					ClearScreen( const Rgba8& clearColor );											// Clear Color
	
//--------------------------------------------------------------------------------------------------------------------------------------------
//			D3D12 DEBUG MODULE 
//--------------------------------------------------------------------------------------------------------------------------------------------

	void					CreateDebugModule();
	void					DestroyDebugModule();
	void					ReportLiveObjects();

//--------------------------------------------------------------------------------------------------------------------------------------------
	
	CommandQueueDX12*		CreateCommandQueue( ID3D12Device2* device , eDX12CommandListType type );
	bool					CheckTearingSupport();
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	
public:
	ID3D12Device*								m_device												= nullptr;
	ID3D12DeviceContext*						m_context												= nullptr;
	void*										m_debugModule											= nullptr;
	IDXGIDebug*									m_debug													= nullptr;

	ID3D12Resource*								t_backbuffer											= nullptr;
	IDXGISwapChain4*							t_swapchain												= nullptr;
};

//--------------------------------------------------------------------------------------------------------------------------------------------