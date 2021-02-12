#pragma once
#include "Engine/Platform/Window.hpp"
#include "Engine/RendererDX12/D3D12Utils.hpp"
#include <chrono>

//--------------------------------------------------------------------------------------------------------------------------------------------

struct  ID3D12Resource;
struct  IDXGISwapChain4;
struct	ID3D12Device;
struct	ID3D12DeviceContext;
struct	ID3D12InfoQueue;
struct	IDXGIAdapter4;
struct	IDXGIDebug;
struct  ID3D12Debug;

class	CommandQueueDX12;
class	DirectQueueDX12;
class	DescriptorHeapDX12;
class	CommandAllocatorDX12;
class	CommandListDX12;

//--------------------------------------------------------------------------------------------------------------------------------------------
	
class RenderContextDX12
{

public:
	RenderContextDX12() {};
	~RenderContextDX12();

	HRESULT					Startup( Window* window );
	HRESULT					CheckGraphicsAdapters( bool useWARPAdapter = false );
	HRESULT					CreateDevice();
	void					CreateInfoQueue();
	void					BeginFrame();
	void					UpdateFrameTime( float deltaSeconds );
	void					EndFrame();
	void					Shutdown();
	
	HRESULT					CreateSwapChain( HWND hWnd , CommandQueueDX12* commandQueue , uint32_t width , uint32_t height , uint32_t bufferCount );
	void*					CreateFenceEventHandle();
	void					Present();
	void					Flush( uint64_t& fenceValue );

//--------------------------------------------------------------------------------------------------------------------------------------------
//			CAMERA METHODS
//--------------------------------------------------------------------------------------------------------------------------------------------
	
	void					ClearScreen( const Rgba8& clearColor );											// Clear Color
	
//--------------------------------------------------------------------------------------------------------------------------------------------
//			D3D12 DEBUG MODULE 
//--------------------------------------------------------------------------------------------------------------------------------------------
	void					EnableDebugLayer();
	void					CreateDebugModule();
	void					DestroyDebugModule();
	void					ReportLiveObjects();

//--------------------------------------------------------------------------------------------------------------------------------------------
	
	CommandQueueDX12*		CreateCommandQueue( eDX12CommandListType type );
	bool					CheckTearingSupport();
	bool					IsVSyncEnabled()											{ return m_isVsyncEnabled; }
	bool					HasTearingSupport()											{ return m_hasTearingSupport; }
	void					UpdateRenderTargetViews();
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	
public:
	ID3D12Device*								m_device												= nullptr;
	ID3D12DeviceContext*						m_context												= nullptr;
	void*										m_debugModule											= nullptr;
	IDXGIDebug*									m_debug													= nullptr;
	ID3D12Debug*								m_dx12DebugModule										= nullptr;		
	ID3D12InfoQueue*							m_infoQueue												= nullptr;
	IDXGIAdapter4*								m_deviceAdapter											= nullptr;
	bool										m_isVsyncEnabled										= false;
	bool										m_hasTearingSupport										= false;

	const uint8_t								m_numBackBufferFrames									= 3;
	uint64_t									m_frameFenceValues[ 3 ]									= {};
	uint64_t									m_fenceValue											= 0;

	ID3D12Resource*								t_backBuffers[ 3 ];
	IDXGISwapChain4*							t_swapchain												= nullptr;
	DescriptorHeapDX12*							m_RTVDescriptorHeap										= nullptr;
	UINT										m_RTVDescriptorSize										= 0;

//	DirectQueueDX12*							m_directCommandQueue									= nullptr;
	CommandQueueDX12*							m_commandQueue											= nullptr;
	uint8_t										m_currentBackBufferIndex								= 0;

	CommandAllocatorDX12*						m_commandAllocators[ 3 ];
	CommandListDX12*							m_commandList											= nullptr;
	HANDLE										m_fenceEvent											= nullptr;
private:
	
};

//--------------------------------------------------------------------------------------------------------------------------------------------