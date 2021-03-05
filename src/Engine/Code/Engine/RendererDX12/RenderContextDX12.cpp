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

//#include <D3Dcommon.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include "DescriptorHeapDX12.hpp"
#include "CommandAllocatorDX12.hpp"
#include "CommandListDX12.hpp"
#include "ShaderDX12.hpp"

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
	//	Flush( m_fenceValue );
//	::CloseHandle( m_fenceEvent );

	SAFE_RELEASE_POINTER( m_commandList );
	SAFE_RELEASE_POINTER( m_commandQueue );

	for ( int index = 0; index < 3; index++ )
	{
		SAFE_RELEASE_POINTER( m_commandAllocators[ index ] );
	}

	SAFE_RELEASE_POINTER( m_RTVDescriptorHeap );

	for ( int index = 0; index < 3; index++ )
	{
		DX_SAFE_RELEASE( t_backBuffers[ index ] );
	}
	DX_SAFE_RELEASE( t_swapchain );

	DX_SAFE_RELEASE( m_infoQueue );
	DX_SAFE_RELEASE( m_dx12DebugModule );
	DX_SAFE_RELEASE( m_debug );
	DX_SAFE_RELEASE( m_deviceAdapter );
	DX_SAFE_RELEASE( m_device );
//	DX_SAFE_RELEASE( m_context );
	DX_SAFE_RELEASE( m_device );

	delete g_D3D12PerfMarker;
	g_D3D12PerfMarker = nullptr;
	
	ReportLiveObjects();    // do our leak reporting just before shutdown to give us a better detailed list;
	DestroyDebugModule();

}

//--------------------------------------------------------------------------------------------------------------------------------------------

HRESULT RenderContextDX12::Startup( Window* window )
{
	m_window = window;

	EnableDebugLayer();
	HRESULT resourceInit;
	resourceInit = CheckGraphicsAdapters( false );
	resourceInit |= CreateDevice();

#if defined( RENDER_DEBUG ) || defined ( _DEBUG ) || defined ( _FASTBREAK ) || defined ( _DEBUG_PROFILE ) || defined ( _FASTBREAK_PROFILE ) || defined ( _RELEASE_PROFILE )
	CreateDebugModule();
	CreateInfoQueue();
#endif

	m_commandQueue = CreateCommandQueue( DX12_COMMAND_LIST_TYPE_DIRECT );
	
	resourceInit |= CreateSwapChain( m_commandQueue , m_numBackBufferFrames );
	m_currentBackBufferIndex = ( uint8_t ) t_swapchain->GetCurrentBackBufferIndex();
 	
	m_RTVDescriptorHeap = new DescriptorHeapDX12( this , D3D12_DESCRIPTOR_HEAP_TYPE_RTV , m_numBackBufferFrames );
	m_RTVDescriptorSize = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
 	
 	UpdateRenderTargetViews();
	
	for ( int index = 0; index < m_numBackBufferFrames; ++index )
	{
		m_commandAllocators[ index ] = new CommandAllocatorDX12( this , D3D12_COMMAND_LIST_TYPE_DIRECT );
	}
	
	CreateRootSignature();

	m_defaultShader = new ShaderDX12( this , "Data/Shaders/triangle.hlsl" );
	m_defaultShader->CreateFromFile( this , "Data/Shaders/triangle.hlsl" );

	m_currentShader = m_defaultShader;

	// vertex layout
	D3D12_INPUT_ELEMENT_DESC desc;
	// pso description 
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[ 0 ].BlendEnable = FALSE;
	blendDesc.RenderTarget[ 0 ].LogicOpEnable = FALSE;
	blendDesc.RenderTarget[ 0 ].SrcBlend = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[ 0 ].DestBlend = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[ 0 ].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[ 0 ].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[ 0 ].DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[ 0 ].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[ 0 ].LogicOp = D3D12_LOGIC_OP_NOOP;
	blendDesc.RenderTarget[ 0 ].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_RASTERIZER_DESC rasterDesc{};
	rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterDesc.FrontCounterClockwise = FALSE;
	rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterDesc.DepthClipEnable = TRUE;
	rasterDesc.MultisampleEnable = FALSE;
	rasterDesc.AntialiasedLineEnable = FALSE;
	rasterDesc.ForcedSampleCount = 0;
	rasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	m_pipelineStateDesc.pRootSignature = m_rootSignature;


	D3D12_INPUT_LAYOUT_DESC inputlayoutDesc{};
	inputlayoutDesc.NumElements = 1;
	m_pipelineStateDesc.InputLayout = { nullptr ,0 };
	m_pipelineStateDesc.pRootSignature = m_rootSignature;
	m_pipelineStateDesc.VS = m_currentShader->m_vertexStage.GetAsD3D12ByteCode();
	m_pipelineStateDesc.PS = m_currentShader->m_fragmentStage.GetAsD3D12ByteCode();
	m_pipelineStateDesc.RasterizerState = rasterDesc;
	m_pipelineStateDesc.BlendState = blendDesc;
	m_pipelineStateDesc.DepthStencilState.DepthEnable = FALSE;
	m_pipelineStateDesc.DepthStencilState.StencilEnable = FALSE;
	m_pipelineStateDesc.SampleMask = UINT_MAX;
	m_pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_pipelineStateDesc.NumRenderTargets = 1;
	m_pipelineStateDesc.RTVFormats[ 0 ] = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_pipelineStateDesc.SampleDesc.Count = 1;
	resourceInit |= m_device->CreateGraphicsPipelineState( &m_pipelineStateDesc , IID_PPV_ARGS( &m_pipelineState ) );
	
	m_commandList = new CommandListDX12( this , m_commandAllocators[ 0 ] , m_pipelineState , D3D12_COMMAND_LIST_TYPE_DIRECT );
	CreateFenceEventHandle();

	return resourceInit;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

HRESULT RenderContextDX12::CheckGraphicsAdapters( bool useWARPAdapter /*= false */ )
{
	IDXGIFactory4* dxgiFactory = nullptr;
	UINT createFactoryFlags = 0;

#if defined ( _DEBUG ) || ( _DEBUG_PROFILE ) || ( _FASTBREAK ) ||  defined ( _FASTBREAK_PROFILE )
	createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	HRESULT result;
	result = CreateDXGIFactory2( createFactoryFlags , IID_PPV_ARGS( &dxgiFactory ) );
	GUARANTEE_OR_DIE( result == S_OK , "DXGFI FACTORY CREATION FOR DEVICE ENUMERATION FAILED" );
	
	//--------------------------------------------------------------------------------
	//					NOTES
	//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	//	ComPtr usage read here https://stackoverflow.com/questions/35214778/what-does-comptr-as-do
	//--------------------------------------------------------------------------------

	IDXGIAdapter1* warpAdapter = nullptr;
	
	if ( useWARPAdapter )
	{
		result |= dxgiFactory->EnumWarpAdapter( IID_PPV_ARGS( &warpAdapter ) );
		GUARANTEE_OR_DIE( result == S_OK , "WARP ADAPTER FAILED DURING DEVICE ENUMERATION" );

 		result |= warpAdapter->QueryInterface( IID_PPV_ARGS( &m_deviceAdapter ) );
 		GUARANTEE_OR_DIE( result == S_OK , "WARP ADAPTER ASSIGNMENT FAILED");
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

				result |= warpAdapter->QueryInterface( IID_PPV_ARGS( &m_deviceAdapter ) );
				GUARANTEE_OR_DIE( warpAdapter->QueryInterface( IID_PPV_ARGS( &m_deviceAdapter ) ) == S_OK , "HARDWARE ADAPTER ASSIGNMENT FAILED" );
			}
		}
	}
	
	DX_SAFE_RELEASE( dxgiFactory );
	return result;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

HRESULT RenderContextDX12::CreateDevice()
{
	HRESULT deviceCreation = D3D12CreateDevice( NULL , D3D_FEATURE_LEVEL_12_1 , _uuidof( ID3D12Device ) , ( void** ) &m_device );
	GUARANTEE_OR_DIE( deviceCreation == S_OK , "D3D12 DEVICE CREATION FAILED" );
	return deviceCreation;
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
	Present();
	Flush( m_fenceValue );
	//m_swapChain->Present();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::Shutdown()
{

}

//--------------------------------------------------------------------------------------------------------------------------------------------

HRESULT RenderContextDX12::CreateSwapChain( CommandQueueDX12* commandQueue , uint32_t bufferCount )
{
	IDXGIFactory4* dxgiFactory4 = nullptr;
	UINT createFactoryFlags = 0;

#if defined(_DEBUG)
	createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	HRESULT swapchain = CreateDXGIFactory2( createFactoryFlags , IID_PPV_ARGS( &dxgiFactory4 ) );
	GUARANTEE_OR_DIE( swapchain == S_OK , "Factory Creation for SWAP CHAIN FAILED" );

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = ( uint32_t ) m_window->GetClientWidth();
	swapChainDesc.Height = ( uint32_t ) m_window->GetClientHeight();
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
	swapchain |= dxgiFactory4->CreateSwapChainForHwnd(
		commandQueue->m_commandQueue ,
		( HWND ) m_window->m_hwnd ,
		&swapChainDesc ,
		nullptr ,
		nullptr ,
		&swapChain1 );
		
	GUARANTEE_OR_DIE( swapchain == S_OK , "SWAP CHAIN CREATION FAILED" );

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
	// will be handled manually.
	//GUARANTEE_OR_DIE( dxgiFactory4->MakeWindowAssociation( hWnd , DXGI_MWA_NO_ALT_ENTER ) , "DISABLING MAUNAL OVERRIDE on Window Scaling FAILED" );
	
	swapchain = swapChain1->QueryInterface( IID_PPV_ARGS( &t_swapchain ) );
	GUARANTEE_OR_DIE( swapchain == S_OK , "SWAP CHAIN ASSIGNMENT FAILED" );

	return swapchain;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void* RenderContextDX12::CreateFenceEventHandle()
{
	m_fenceEvent = ::CreateEvent( NULL , FALSE , FALSE , NULL );
	ASSERT_OR_DIE( m_fenceEvent != nullptr , "FAILED to Create FENCE EVENT" );
	return ( void* ) m_fenceEvent;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::Present()
{
	auto backBuffer = t_backBuffers[ m_currentBackBufferIndex ];

	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = backBuffer;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	
	m_commandList->m_commandList->ResourceBarrier( 1 , &barrier );

	m_commandList->m_commandList->Close();

	ID3D12CommandList* const commandLists[] = {
		m_commandList->m_commandList
	};
	m_commandQueue->m_commandQueue->ExecuteCommandLists( _countof( commandLists ) , commandLists );

	m_frameFenceValues[ m_currentBackBufferIndex ] = m_commandQueue->SignalFence( m_fenceValue );
	//Signal( g_CommandQueue , g_Fence , g_FenceValue );

	UINT syncInterval = m_isVsyncEnabled ? 1 : 0;
	UINT presentFlags = m_hasTearingSupport && !m_isVsyncEnabled ? DXGI_PRESENT_ALLOW_TEARING : 0;
	t_swapchain->Present( syncInterval , presentFlags );

	m_currentBackBufferIndex = ( uint8_t ) t_swapchain->GetCurrentBackBufferIndex();

	m_commandQueue->m_fence->WaitForFenceValue( m_frameFenceValues[ m_currentBackBufferIndex ] , ( void* ) m_fenceEvent );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::Flush( uint64_t& fenceValue )
{
	uint64_t fenceValueForSignal = m_commandQueue->SignalFence( fenceValue );
	m_commandQueue->m_fence->WaitForFenceValue( fenceValueForSignal , reinterpret_cast< void* >( m_fenceEvent ) );
}

//--------------------------------------------------------------------------------------------------------------------------------------------


void RenderContextDX12::ClearScreen( const Rgba8& clearColor )
{
	auto commandAllocator = m_commandAllocators[ m_currentBackBufferIndex ];
	
	ID3D12Resource* backBuffer = nullptr;
	t_swapchain->GetBuffer( m_currentBackBufferIndex , IID_PPV_ARGS( &backBuffer ) );

	commandAllocator->m_commandAllocator->Reset();
	m_commandList->m_commandList->Reset( commandAllocator->m_commandAllocator , nullptr );

	float clearFloats[ 4 ];
	float scaleToFloat = 1 / 255.f;

	clearFloats[ 0 ] = ( float ) clearColor.r * scaleToFloat;
	clearFloats[ 1 ] = ( float ) clearColor.g * scaleToFloat;
	clearFloats[ 2 ] = ( float ) clearColor.b * scaleToFloat;
	clearFloats[ 3 ] = ( float ) clearColor.a * scaleToFloat;

	m_commandList->m_commandList->SetPipelineState( m_pipelineState );
	m_commandList->m_commandList->SetGraphicsRootSignature( m_rootSignature );
	m_viewPort.TopLeftX = 0;
	m_viewPort.TopLeftY = 0;
	m_viewPort.Height = ( FLOAT ) m_window->GetClientHeight();
	m_viewPort.Width = ( FLOAT ) m_window->GetClientWidth();
	m_viewPort.MinDepth = 0.0f;
	m_viewPort.MaxDepth = 1.0f;

	m_scisrroRec.left = 0;
	m_scisrroRec.top = 0;
	m_scisrroRec.right = ( LONG ) m_window->GetClientWidth();
	m_scisrroRec.bottom = ( LONG ) m_window->GetClientHeight();
	m_commandList->m_commandList->RSSetViewports( 1 , &m_viewPort );
	m_commandList->m_commandList->RSSetScissorRects( 1 , &m_scisrroRec );

	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = backBuffer;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	
	m_commandList->m_commandList->ResourceBarrier( 1 , &barrier );
	m_RTVDescriptorHeap->m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_RTVDescriptorHeap->m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();

	rtv.ptr += ( ( UINT ) m_RTVDescriptorSize * ( UINT ) m_currentBackBufferIndex );
	
	m_commandList->m_commandList->ClearRenderTargetView( rtv , clearFloats , 0 , nullptr );
	m_commandList->m_commandList->OMSetRenderTargets( 1 , &rtv , FALSE , nullptr );
	m_commandList->m_commandList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
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
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RTVDescriptorHeap->m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
//	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RTVDescriptorHeap->m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();

	for ( int backBufferIndex = 0; backBufferIndex < m_numBackBufferFrames; ++backBufferIndex )
	{
		ID3D12Resource* backBuffer = nullptr;
		GUARANTEE_OR_DIE( t_swapchain->GetBuffer( backBufferIndex , IID_PPV_ARGS( &backBuffer ) ) == S_OK , "GETTING BACK BUFFER FAILED" );

		m_device->CreateRenderTargetView( backBuffer , nullptr , rtvHandle );

		t_backBuffers[ backBufferIndex ] = backBuffer;
		rtvHandle.ptr += m_RTVDescriptorSize;
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::CreateRootSignature()
{

// 	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
// 	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
// 	if ( S_OK != m_device->CheckFeatureSupport( D3D12_FEATURE_ROOT_SIGNATURE , &featureData , sizeof( featureData ) ) )
// 	{
// 		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
// 	}

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	// A single 32-bit constant root parameter that is used by the vertex shader.

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDescription{};
	
	D3D12_ROOT_PARAMETER rootParameters[ 1 ] = {};
	
	rootParameters[ 0 ].Constants.Num32BitValues = 16;
	rootParameters[ 0 ].Constants.RegisterSpace = 0;
	rootParameters[ 0 ].Constants.ShaderRegister = 0;
	rootParameters[ 0 ].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	rootSignatureDescription.NumParameters		= 0/* _countof( rootParameters )*/;
	rootSignatureDescription.pParameters		= nullptr/*rootParameters*/;
	rootSignatureDescription.NumStaticSamplers	= 0;
	rootSignatureDescription.pStaticSamplers	= nullptr;
	rootSignatureDescription.Flags				= rootSignatureFlags;

	// Serialize the root signature.
	GUARANTEE_OR_DIE( S_OK == D3D12SerializeRootSignature( &rootSignatureDescription , D3D_ROOT_SIGNATURE_VERSION_1 , &m_rootSignatureBlob , &m_errorBlob ) , "Serlize Root siganture failed" );
	
	// Create the root signature.
	size_t size = m_rootSignatureBlob->GetBufferSize();
	HRESULT rootSignatureCreation = m_device->CreateRootSignature( 0 ,
													m_rootSignatureBlob->GetBufferPointer() ,
													m_rootSignatureBlob->GetBufferSize() ,
													IID_PPV_ARGS( &m_rootSignature ) );
		
	GUARANTEE_OR_DIE( rootSignatureCreation == S_OK , "Root Signature Creation Failed" );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::TestDraw()
{
//	m_commandAllocators[ m_currentBackBufferIndex ]->m_commandAllocator->Reset();
//	m_commandList->m_commandList->Reset( m_commandAllocators[ m_currentBackBufferIndex ]->m_commandAllocator , m_pipelineState );


	// RECOURCE BARRIER MIGT BEE NEEDED
//	D3D12_RESOURCE_BARRIER barrier;
//	barrier.Transition.pResource = t_backBuffers[ m_currentBackBufferIndex ];
//	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
//	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
//	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
//	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
//	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
//	m_commandList->m_commandList->ResourceBarrier( 1 , &barrier );
//
//	// clearScreen
//
//	UINT strideRTV = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
//	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RTVDescriptorHeap->m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
//	rtvHandle.ptr += strideRTV * m_currentBackBufferIndex;
	
	m_commandList->m_commandList->DrawInstanced( 3 , 1 , 0 , 0 );

//	Present();
//  Flush();
//	barrier.Transition.pResource = m_rtvResource[ m_frameIndex ];
//	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
//	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
//	m_commandList->ResourceBarrier( 1 , &barrier );
//	GUARANTEE_OR_DIE( m_commandList->Close() , "close command list failed" );
//
//
//
//	CommandListDX12* ppCommandLists[] = { m_commandList };
//	m_commandQueue->ExecuteCommandLists( _countof( ppCommandLists ) , ppCommandLists );
}

//--------------------------------------------------------------------------------------------------------------------------------------------
