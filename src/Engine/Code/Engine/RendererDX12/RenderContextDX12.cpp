#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/MatrixUtils.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Profilling/D3D12PerformanceMarker.hpp"
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

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#include <d3d12sdklayers.h>
#include <dxgidebug.h>
#include "Engine/RendererDX12/CommandQueueDX12.hpp"
#include <winerror.h>

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
#if defined(RENDER_DEBUG) && defined ( _DEBUG ) || defined ( _FASTBREAK ) || defined ( _DEBUG_PROFILE ) || defined ( _FASTBREAK_PROFILE ) || defined ( _RELEASE_PROFILE )
	CreateDebugModule();
#endif

//	IDXGISwapChain* swapchain = nullptr;  // Create Swap Chain

	UINT flags = 0; /*D3D12_CREATE_DEVICE_SINGLETHREADED;*/
#if defined(RENDER_DEBUG) && defined ( _DEBUG ) || defined ( _FASTBREAK ) || defined ( _DEBUG_PROFILE ) || defined ( _FASTBREAK_PROFILE ) || defined ( _RELEASE_PROFILE )
	flags |= D3D12_CREATE_DEVICE_DEBUG;
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

void RenderContextDX12::CreateDebugModule()
{
	// load the dll
 	m_debugModule = ::LoadLibraryA( "Dxgidebug.dll" );
 	if ( m_debugModule == nullptr )
 	{
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

CommandQueueDX12* RenderContextDX12::CreateCommandQueue( ID3D12Device2* device , eDX12CommandListType type )
{
	CommandQueueDX12* newCommandQueue = new CommandQueueDX12( device , type );
	return newCommandQueue;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

bool RenderContextDX12::CheckTearingSupport()
{
	BOOL allowTearing = FALSE;

	// Rather than create the DXGI 1.5 factory interface directly, we create the
	// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
	// graphics debugging tools which will not support the 1.5 factory interface 
	// until a future update.
/*
// 	IDXGIFactory4* factory4 = nullptr;
// 	if ( SUCCEEDED( CreateDXGIFactory1( IID_PPV_ARGS( &factory4 ) ) ) )
// 	{
// 		IDXGIFactory5* factory5 = nullptr;
// 
// 		if ( SUCCEEDED( factory4.As( &factory5 ) ) )
// 		{
// 			if ( FAILED( factory5->CheckFeatureSupport(
// 				DXGI_FEATURE_PRESENT_ALLOW_TEARING ,
// 				&allowTearing , sizeof( allowTearing ) ) ) )
// 			{
// 				allowTearing = FALSE;
// 			}
// 		}
// 	}
*/

	return allowTearing == TRUE;
}

//--------------------------------------------------------------------------------------------------------------------------------------------
