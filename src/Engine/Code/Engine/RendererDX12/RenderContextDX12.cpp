#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/MatrixUtils.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Profilling/D3D12PerformanceMarker.hpp"
#include "Engine/RendererDX12/CommandQueueDX12.hpp"
#include "Engine/RendererDX12/RenderContextDX12.hpp"
#include "Engine/Time/Time.hpp"
//#include "Engine/Renderer/D3DCommon.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------
//				THIRD PARTY LIBRARIES
//--------------------------------------------------------------------------------------------------------------------------------------------
#pragma warning( push )
#pragma warning ( disable: 4100 )
#include "ThirdParty/stb/stb_image.h"
#pragma warning( pop )

#include <string>
#include <corecrt_wstring.h>

//--------------------------------------------------------------------------------------------------------------------------------------------
//			COM POINTER TEMPLATE HEADERS
//--------------------------------------------------------------------------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h>	
#include <wrl/client.h>
#include <winerror.h>

//--------------------------------------------------------------------------------------------------------------------------------------------
//			DIRECTX 12 SPECIFIC HEADERS	
//--------------------------------------------------------------------------------------------------------------------------------------------

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include "DescriptorHeapDX12.hpp"
#include "CommandAllocatorDX12.hpp"
#include "CommandListDX12.hpp"

#define DX_SAFE_RELEASE( ptr ) if ( nullptr != ptr ) { ptr->Release(); ptr = nullptr; }

//--------------------------------------------------------------------------------------------------------------------------------------------
//				D3D12 specific includes and Macros
//--------------------------------------------------------------------------------------------------------------------------------------------

#pragma comment( lib, "D3D12.lib" )       
#pragma comment( lib, "dxgi.lib" )        
#pragma comment( lib, "d3dcompiler.lib" ) 

//--------------------------------------------------------------------------------------------------------------------------------------------
		
		RenderContextDX12*		g_theRenderer	= nullptr;

//--------------------------------------------------------------------------------------------------------------------------------------------
//				PROFILING D3D POINTER FOR THE SPECIFIC CONFIGURATIONS
//--------------------------------------------------------------------------------------------------------------------------------------------

extern	D3D12PerformanceMarker* g_D3D12PerfMarker;

//--------------------------------------------------------------------------------------------------------------------------------------------
	
RenderContextDX12::~RenderContextDX12()
{
//	DX_SAFE_RELEASE( m_context );
//	DX_SAFE_RELEASE( m_device );

	delete g_D3D12PerfMarker;
	g_D3D12PerfMarker = nullptr;
	
	ReportLiveObjects();    // do our leak reporting just before shutdown to give us a better detailed list;
	DestroyDebugModule();

}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::Startup( Window* window )
{
	EnableDebugLayer();
	
	CheckGraphicsAdapters( false );
	CreateDevice();
	
	m_commandQueue = CreateCommandQueue( DX12_COMMAND_LIST_TYPE_DIRECT );
	
	CreateSwapChain( ( HWND ) window->m_hwnd , m_commandQueue , window->GetClientWidth() , window->GetClientHeight() , m_numBackBufferFrames );
	m_currentBackBufferIndex = ( uint8_t ) t_swapchain->GetCurrentBackBufferIndex();
 	
	m_RTVDescriptorHeap = new DescriptorHeapDX12( this , D3D12_DESCRIPTOR_HEAP_TYPE_RTV , m_numBackBufferFrames );
	m_RTVDescriptorSize = ( uint8_t ) m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
 
 	UpdateRenderTargetViews();

	for ( int index = 0; index < m_numBackBufferFrames; ++index )
	{
		m_commandAllocators[ index ] = new CommandAllocatorDX12( this , D3D12_COMMAND_LIST_TYPE_DIRECT );
	}
	m_commandList = new CommandListDX12( this , m_commandAllocators[ 0 ] , D3D12_COMMAND_LIST_TYPE_DIRECT );
		
	CreateFenceEventHandle();

// 	// Make sure the command queue has finished all commands before closing.
// 	Flush( g_CommandQueue , g_Fence , g_FenceValue , g_FenceEvent );
// 
// 	::CloseHandle( g_FenceEvent );

#if defined( RENDER_DEBUG ) || defined ( _DEBUG ) || defined ( _FASTBREAK ) || defined ( _DEBUG_PROFILE ) || defined ( _FASTBREAK_PROFILE ) || defined ( _RELEASE_PROFILE )
	CreateDebugModule();
	CreateInfoQueue();
#endif
/*
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	memset( &swapChainDesc , 0 , sizeof( swapChainDesc ) );
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;

	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // on swap, the old buffer is discarded
	swapChainDesc.Flags = 0; // additional flags - see docs.  Used in special cases like for video buffers

	// how swap chain is to be used
	HWND hwnd = ( HWND ) window->m_hwnd;
	swapChainDesc.OutputWindow = hwnd; // HWND for the window to be used
	swapChainDesc.SampleDesc.Count = 1; // how many samples per pixel (1 so no MSAA)
										 // note, if we're doing MSAA, we'll do it on a secondary target

	// describe the buffer
	swapChainDesc.Windowed = TRUE;                                    // windowed/full-screen mode
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color RGBA8 color
	swapChainDesc.BufferDesc.Width = window->GetClientWidth();
	swapChainDesc.BufferDesc.Height = window->GetClientHeight();
	// save data as member variable when window is created.

	// create
*/
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::CheckGraphicsAdapters( bool useWARPAdapter /*= false */ )
{
	IDXGIFactory4* dxgiFactory = nullptr;
	UINT createFactoryFlags = 0;

#if defined ( _DEBUG ) || ( _DEBUG_PROFILE ) || ( _FASTBREAK ) ||  defined ( _FASTBREAK_PROFILE )
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	GUARANTEE_OR_DIE( CreateDXGIFactory2( createFactoryFlags , IID_PPV_ARGS( &dxgiFactory ) ) == S_OK , "DXGFI FACTORY CREATION FOR DEVICE ENUMERATION FAILED" );

	//--------------------------------------------------------------------------------
	//					NOTES
	//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	//	ComPtr usage read here https://stackoverflow.com/questions/35214778/what-does-comptr-as-do
	//--------------------------------------------------------------------------------

	IDXGIAdapter1* warpAdapter = nullptr;
	
	if ( useWARPAdapter )
	{
		GUARANTEE_OR_DIE( dxgiFactory->EnumWarpAdapter( IID_PPV_ARGS( &warpAdapter ) ) == S_OK , "WARP ADAPTER FAILED DURING DEVICE ENUMERATION" );
 		GUARANTEE_OR_DIE( warpAdapter->QueryInterface( IID_PPV_ARGS( &m_deviceAdapter ) ) == S_OK , "WARP ADAPTER ASSIGNMENT FAILED");
	}
	else
	{
		SIZE_T maxDedicatedVideoMemory = 0;
		for ( UINT i = 0; dxgiFactory->EnumAdapters1( i , &warpAdapter ) != DXGI_ERROR_NOT_FOUND; ++i )
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			warpAdapter->GetDesc1( &dxgiAdapterDesc1 );

			// Check to see if the adapter can create a D3D12 device without actually 
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			if ( ( dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE ) == 0 &&
				SUCCEEDED( D3D12CreateDevice( warpAdapter ,
					D3D_FEATURE_LEVEL_12_1 , __uuidof( ID3D12Device ) , nullptr ) ) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory )
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;

				GUARANTEE_OR_DIE( warpAdapter->QueryInterface( IID_PPV_ARGS( &m_deviceAdapter ) ) == S_OK , "HARDWARE ADAPTER ASSIGNMENT FAILED" );
			}
		}
	}
	
	DX_SAFE_RELEASE( dxgiFactory );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::CreateDevice()
{
	HRESULT deviceCreation = D3D12CreateDevice( m_deviceAdapter , D3D_FEATURE_LEVEL_12_1 , IID_PPV_ARGS( &m_device ) );
	GUARANTEE_OR_DIE( deviceCreation == S_OK , "D3D12 DEVICE CREATION FAILED" );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::CreateInfoQueue()
{
#if defined ( _DEBUG ) || ( _DEBUG_PROFILE ) || ( _FASTBREAK ) ||  defined ( _FASTBREAK_PROFILE )
	
	HRESULT hr = m_device->QueryInterface( IID_PPV_ARGS( &m_infoQueue ) );
	UNUSED( hr );

	if ( m_device->QueryInterface( IID_PPV_ARGS( &m_infoQueue ) ) == S_OK )
	{
		m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_CORRUPTION , TRUE );
		m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR , TRUE );
		m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_WARNING , TRUE );

		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		//NewFilter.DenyList.NumCategories = _countof(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = _countof( Severities );
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof( DenyIds );
		NewFilter.DenyList.pIDList = DenyIds;

		GUARANTEE_OR_DIE( m_infoQueue->PushStorageFilter( &NewFilter ) == S_OK , "INFO QUEUE STORAGE FILTER CREATION FAILED" );
	}
	else
		ERROR_AND_DIE( "FAILED TO CREATE D3D12 INFO QUEUE IN DEBUG BUILD" );
#endif
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::BeginFrame()
{

}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::UpdateFrameTime( float deltaSeconds )
{
	UNUSED( deltaSeconds );
	// FrameDataT frameData;
	// frameData.m_systemTime = ( float ) GetCurrentTimeSeconds();
	// frameData.m_systemDeltaTime = deltaSeconds;
	// //m_frameUBO->m_isDirty = true;
	// m_frameUBO->Update( &frameData , sizeof( frameData ) , sizeof( frameData ) );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::EndFrame()
{
	//m_swapChain->Present();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::Shutdown()
{
	DX_SAFE_RELEASE( m_deviceAdapter );
	DX_SAFE_RELEASE( m_device );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::CreateSwapChain( HWND hWnd , CommandQueueDX12* commandQueue , uint32_t width , uint32_t height , uint32_t bufferCount )
{
	IDXGIFactory4* dxgiFactory4 = nullptr;
	UINT createFactoryFlags = 0;

#if defined(_DEBUG)
	createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	GUARANTEE_OR_DIE( CreateDXGIFactory2( createFactoryFlags , IID_PPV_ARGS( &dxgiFactory4 ) ) == S_OK , "Factory Creation for SWAP CHAIN FAILED" );

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// It is recommended to always allow tearing if tearing support is available.
	swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	IDXGISwapChain1* swapChain1 = nullptr;
	GUARANTEE_OR_DIE( dxgiFactory4->CreateSwapChainForHwnd(
		commandQueue->m_commandQueue ,
		hWnd ,
		&swapChainDesc ,
		nullptr ,
		nullptr ,
		&swapChain1 ) == S_OK , "SWAP CHAIN CREATION FAILED" );

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
	// will be handled manually.
	//GUARANTEE_OR_DIE( dxgiFactory4->MakeWindowAssociation( hWnd , DXGI_MWA_NO_ALT_ENTER ) , "DISABLING MAUNAL OVERRIDE on Window Scaling FAILED" );

	GUARANTEE_OR_DIE( swapChain1->QueryInterface( IID_PPV_ARGS( &t_swapchain ) ) == S_OK , "SWAP CHAIN ASSIGNMENT FAILED" );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::CreateFenceEventHandle()
{
	m_fenceEvent = ::CreateEvent( NULL , FALSE , FALSE , NULL );
	ASSERT_OR_DIE( m_fenceEvent == nullptr , "FAILED to Create FENCE EVENT" );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::Flush()
{
	//uint64_t fenceValueForSignal = Signal
}

//--------------------------------------------------------------------------------------------------------------------------------------------


void RenderContextDX12::ClearScreen( const Rgba8& clearColor )
{
	float clearFloats[ 4 ];
	float scaleToFloat = 1 / 255.f;

	clearFloats[ 0 ] = ( float ) clearColor.r * scaleToFloat;
	clearFloats[ 1 ] = ( float ) clearColor.g * scaleToFloat;
	clearFloats[ 2 ] = ( float ) clearColor.b * scaleToFloat;
	clearFloats[ 3 ] = ( float ) clearColor.a * scaleToFloat;

	// can be put under clear Texture function

	//Texture* backbuffer = m_swapChain->GetBackBuffer();
	//TextureView* backbuffer_rtv = backbuffer->GetOrCreateRenderTargetView();
	//
	//ID3D12RenderTargetView* rtv = backbuffer_rtv->GetRTVHandle();
	//m_context->ClearRenderTargetView( rtv , clearFloats );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::EnableDebugLayer()
{
#if defined( RENDER_DEBUG ) || defined ( _DEBUG ) || defined ( _FASTBREAK ) || defined ( _DEBUG_PROFILE ) || defined ( _FASTBREAK_PROFILE ) || defined ( _RELEASE_PROFILE )
	D3D12GetDebugInterface( IID_PPV_ARGS( &m_dx12DebugModule ) );
	m_dx12DebugModule->EnableDebugLayer();
#endif
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::CreateDebugModule()
{
	// load the dll
 	m_debugModule = ::LoadLibraryA( "Dxgidebug.dll" );
 	if ( m_debugModule == nullptr )
 	{
		//D3D12GetDebugInterface( IID_PPV_ARGS( m_dx12DebugModule ) );
		//m_debugModule->EnableDebugLayer();

 		DebuggerPrintf( "gfx" , "Failed to find dxgidebug.dll.  No debug features enabled." );
 	}
 	else
 	{
 		// find a function in the loaded dll
 		typedef HRESULT( WINAPI* GetDebugModuleCB )( REFIID , void** );
		HMODULE temp = ( HMODULE ) m_debugModule;
		GetDebugModuleCB cb = ( GetDebugModuleCB ) ::GetProcAddress( temp , "DXGIGetDebugInterface" );

 		// create our debug object
 		HRESULT hr = cb( __uuidof( IDXGIDebug ) , ( void** ) &m_debug );
 		ASSERT_OR_DIE( SUCCEEDED( hr ), "" );
 	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::DestroyDebugModule()
{
	if ( nullptr != m_debug )
	{
		DX_SAFE_RELEASE( m_debug );   // release our debug object
		FreeLibrary( ( HMODULE ) m_debugModule ); // unload the dll

		m_debug = nullptr;
		m_debugModule = ( HMODULE )nullptr;
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::ReportLiveObjects()
{
	if ( nullptr != m_debug )
	{
		m_debug->ReportLiveObjects( DXGI_DEBUG_ALL , DXGI_DEBUG_RLO_ALL );
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

CommandQueueDX12* RenderContextDX12::CreateCommandQueue( eDX12CommandListType type )
{
	CommandQueueDX12* newCommandQueue = new CommandQueueDX12( this , type );
	return newCommandQueue;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

bool RenderContextDX12::CheckTearingSupport()
{
	BOOL allowTearing = FALSE;
	m_hasTearingSupport = false;
	// Rather than create the DXGI 1.5 factory interface directly, we create the
	// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
	// graphics debugging tools which will not support the 1.5 factory interface 
	// until a future update.

 	IDXGIFactory4* factory4 = nullptr;
 	if ( SUCCEEDED( CreateDXGIFactory1( IID_PPV_ARGS( &factory4 ) ) ) )
 	{
 		IDXGIFactory5* factory5 = nullptr;
 
 		if ( factory4->QueryInterface( IID_PPV_ARGS( &factory5 ) ) == S_OK )
 		{
 			if ( FAILED( factory5->CheckFeatureSupport(
 				DXGI_FEATURE_PRESENT_ALLOW_TEARING ,
 				&allowTearing , sizeof( allowTearing ) ) ) )
 			{
 				allowTearing = FALSE;
				m_hasTearingSupport = false;
 			}
 		}
 	}
	m_hasTearingSupport = true;
	return allowTearing == TRUE;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::UpdateRenderTargetViews()
{
	UINT strideRTV = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RTVDescriptorHeap->m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();

	for ( int backBufferIndex = 0; backBufferIndex < m_numBackBufferFrames; ++backBufferIndex )
	{
		ID3D12Resource* backBuffer = nullptr;
		GUARANTEE_OR_DIE( t_swapchain->GetBuffer( backBufferIndex , IID_PPV_ARGS( &backBuffer ) ) == S_OK , "GETTING BACK BUFFER FAILED" );

		m_device->CreateRenderTargetView( backBuffer , nullptr , rtvHandle );

		t_backBuffers[ backBufferIndex ] = backBuffer;
		rtvHandle.ptr += strideRTV;
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------
