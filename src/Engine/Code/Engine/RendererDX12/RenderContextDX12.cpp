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
#include "Thirdparty/Dx12Helper/d3d12.h"
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
#include "ThirdParty/Dx12Helper/d3dx12.h"
#include <DirectXMath.h>
#include "Engine/Renderer/RenderContext.hpp"

#include "ThirdParty/DXRHelper/DXRHelper.h"
#include <directxmath.h>

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
 	
 	CreateRenderTargetViews();
	
	for ( int index = 0; index < m_numBackBufferFrames; ++index )
	{
		m_commandAllocators[ index ] = new CommandAllocatorDX12( this , D3D12_COMMAND_LIST_TYPE_DIRECT );
	}
	
	CreateRootSignature();

	m_defaultShader = new ShaderDX12( this , "Data/Shaders/Triangle.hlsl" );
	m_defaultShader->CreateFromFile( this , "Data/Shaders/Triangle.hlsl" );

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

	ZeroMemory( &m_rasterizerStateDesc , sizeof( D3D12_RASTERIZER_DESC ) );
	m_rasterizerStateDesc.FillMode = D3D12_FILL_MODE_SOLID;
	m_rasterizerStateDesc.CullMode = D3D12_CULL_MODE_BACK;
	m_rasterizerStateDesc.FrontCounterClockwise = TRUE;
	m_rasterizerStateDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	m_rasterizerStateDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	m_rasterizerStateDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	m_rasterizerStateDesc.DepthClipEnable = TRUE;
	m_rasterizerStateDesc.MultisampleEnable = FALSE;
	m_rasterizerStateDesc.AntialiasedLineEnable = FALSE;
	m_rasterizerStateDesc.ForcedSampleCount = 0;
	m_rasterizerStateDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	m_pipelineStateDesc.pRootSignature = m_rootSignature;


	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_INPUT_LAYOUT_DESC inputlayoutDesc{ inputLayout , 3 };
	CD3DX12_DEPTH_STENCIL_DESC dsvDesc = CD3DX12_DEPTH_STENCIL_DESC( D3D12_DEFAULT );

	m_pipelineStateDesc.InputLayout = inputlayoutDesc;
	m_pipelineStateDesc.pRootSignature = m_rootSignature;
	m_pipelineStateDesc.VS = m_currentShader->m_vertexStage.GetAsD3D12ByteCode();
	m_pipelineStateDesc.PS = m_currentShader->m_fragmentStage.GetAsD3D12ByteCode();
	m_pipelineStateDesc.RasterizerState = m_rasterizerStateDesc;
	m_pipelineStateDesc.BlendState = blendDesc;
	m_pipelineStateDesc.DepthStencilState = dsvDesc;
	m_pipelineStateDesc.SampleMask = UINT_MAX;
	m_pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_pipelineStateDesc.NumRenderTargets = 1;
	m_pipelineStateDesc.RTVFormats[ 0 ] = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_pipelineStateDesc.SampleDesc.Count = 1;
	m_pipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	resourceInit |= m_device->CreateGraphicsPipelineState( &m_pipelineStateDesc , IID_PPV_ARGS( &m_pipelineState ) );
	
	m_commandList = new CommandListDX12( this , m_commandAllocators[ 0 ] , m_pipelineState , D3D12_COMMAND_LIST_TYPE_DIRECT );
	CreateFenceEventHandle();
	
// 	m_depthStencilDesc.DepthEnable = TRUE; // enable depth testing
// 	m_depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // can write depth data to all of the depth/stencil buffer
// 	m_depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // pixel fragment passes depth test if destination pixel's depth is less than pixel fragment's
// 	m_depthStencilDesc.StencilEnable = FALSE; // disable stencil test
// 	m_depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK; // a default stencil read mask (doesn't matter at this point since stencil testing is turned off)
// 	m_depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK; // a default stencil write mask (also doesn't matter)
// 	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = // a stencil operation structure, again does not really matter since stencil testing is turned off
// 	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
// 	m_depthStencilDesc.FrontFace = defaultStencilOp; // both front and back facing polygons get the same treatment
// 	m_depthStencilDesc.BackFace = defaultStencilOp;

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	m_device->CreateDescriptorHeap( &dsvHeapDesc , IID_PPV_ARGS( &m_dsvHeap ) );

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES dsvHeapProp = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT );
	CD3DX12_RESOURCE_DESC  dsvTex = CD3DX12_RESOURCE_DESC::Tex2D( DXGI_FORMAT_D32_FLOAT , m_window->GetClientWidth() , m_window->GetClientHeight() , 1 , 0 , 1 , 0 , D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL );

	m_device->CreateCommittedResource(
		&dsvHeapProp ,
		D3D12_HEAP_FLAG_NONE ,
		&dsvTex ,
		D3D12_RESOURCE_STATE_DEPTH_WRITE ,
		&depthOptimizedClearValue ,
		IID_PPV_ARGS( &m_depthStencilBuffer )
	);
	m_dsvHeap->SetName( L"Depth/Stencil Resource Heap" );
	m_device->CreateDepthStencilView( m_depthStencilBuffer , &depthStencilDesc , m_dsvHeap->GetCPUDescriptorHandleForHeapStart() );
	
	// DXR
	CheckRaytracingSupport();
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

void RenderContextDX12::CreateVertexBufferForVertexArray( std::vector<Vertex_PCU>& verts )
{
	// create default heap
	// default heap is memory on the GPU. Only the GPU has access to this memory
	// To get data into this heap, we will have to upload the data using
	// an upload heap
	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT );
	CD3DX12_RESOURCE_DESC bufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer( verts.size() * sizeof( Vertex_PCU ) );
	m_device->CreateCommittedResource(
		&heapProp , // a default heap
		D3D12_HEAP_FLAG_NONE , // no flags
		&bufferResourceDesc  , // resource description for a buffer
		D3D12_RESOURCE_STATE_COPY_DEST , // we will start this heap in the copy destination state since we will copy data
										// from the upload heap to this heap
		nullptr , // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
		IID_PPV_ARGS( &m_vertexBuffer ) );

	// we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
	m_vertexBuffer->SetName( L"Vertex Buffer Resource Heap" );

	// create upload heap
	// upload heaps are used to upload data to the GPU. CPU can write to it, GPU can read from it
	// We will upload the vertex buffer using this heap to the default heap
	CD3DX12_HEAP_PROPERTIES uploadHeapProp = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD );

	m_device->CreateCommittedResource(
		&uploadHeapProp , // upload heap
		D3D12_HEAP_FLAG_NONE , // no flags
		&bufferResourceDesc , // resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ , // GPU will read from this buffer and copy its contents to the default heap
		nullptr ,
		IID_PPV_ARGS( &m_vertexBufferUploadHeap ) );
	m_vertexBufferUploadHeap->SetName( L"Vertex Buffer Upload Resource Heap" );

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = reinterpret_cast< BYTE* >( &verts[0] ); // pointer to our vertex array
	vertexData.RowPitch = verts.size() * sizeof( Vertex_PCU ); // size of all our triangle vertex data
	vertexData.SlicePitch = verts.size() * sizeof( Vertex_PCU ); // also the size of our triangle vertex data

	// we are now creating a command with the command list to copy the data from
	// the upload heap to the default heap
	UpdateSubresources( m_commandList->m_commandList , m_vertexBuffer , m_vertexBufferUploadHeap , 0 , 0 , 1 , &vertexData );

	// transition the vertex buffer data from copy destination state to vertex buffer state
	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition( m_vertexBuffer , D3D12_RESOURCE_STATE_COPY_DEST , D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER );
	m_commandList->m_commandList->ResourceBarrier( 1 , &resourceBarrier );

	// Now we execute the command list to upload the initial assets (triangle data)
//	m_commandList->m_commandList->Close();
//	ID3D12CommandList* ppCommandLists[] = { m_commandList->m_commandList };
//	m_commandQueue->m_commandQueue->ExecuteCommandLists( _countof( ppCommandLists ) , ppCommandLists );
//	
//	// increment the fence value now, otherwise the buffer might not be uploaded by the time we start drawing
//	m_fenceValue++;
//	m_commandQueue->SignalFence( m_fenceValue );
	
	// create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = verts.size() * sizeof( Vertex_PCU );
	m_vertexBufferView.StrideInBytes = sizeof( Vertex_PCU );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::CreateVertexBufferForVertexArray( uint numVerts , Vertex_PCU* verts )
{
 	DX_SAFE_RELEASE( m_vertexBuffer );
// 	DX_SAFE_RELEASE( m_vertexBufferUploadHeap );

	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT );
	CD3DX12_RESOURCE_DESC bufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer( numVerts * sizeof( Vertex_PCU ) );
	m_device->CreateCommittedResource(
		&heapProp , // a default heap
		D3D12_HEAP_FLAG_NONE , // no flags
		&bufferResourceDesc , // resource description for a buffer
		D3D12_RESOURCE_STATE_COPY_DEST , // we will start this heap in the copy destination state since we will copy data
										// from the upload heap to this heap
		nullptr , // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
		IID_PPV_ARGS( &m_vertexBuffer ) );

	// we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
	m_vertexBuffer->SetName( L"Vertex Buffer Resource Heap" );

	// create upload heap
	// upload heaps are used to upload data to the GPU. CPU can write to it, GPU can read from it
	// We will upload the vertex buffer using this heap to the default heap
	CD3DX12_HEAP_PROPERTIES uploadHeapProp = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD );

	m_device->CreateCommittedResource(
		&uploadHeapProp , // upload heap
		D3D12_HEAP_FLAG_NONE , // no flags
		&bufferResourceDesc , // resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ , // GPU will read from this buffer and copy its contents to the default heap
		nullptr ,
		IID_PPV_ARGS( &m_vertexBufferUploadHeap ) );
	m_vertexBufferUploadHeap->SetName( L"Vertex Buffer Upload Resource Heap" );

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = reinterpret_cast< BYTE* >( &verts[ 0 ] ); // pointer to our vertex array
	vertexData.RowPitch = numVerts * sizeof( Vertex_PCU ); // size of all our triangle vertex data
	vertexData.SlicePitch = numVerts * sizeof( Vertex_PCU ); // also the size of our triangle vertex data

	// we are now creating a command with the command list to copy the data from
	// the upload heap to the default heap
	UpdateSubresources( m_commandList->m_commandList , m_vertexBuffer , m_vertexBufferUploadHeap , 0 , 0 , 1 , &vertexData );

	// transition the vertex buffer data from copy destination state to vertex buffer state
	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition( m_vertexBuffer , D3D12_RESOURCE_STATE_COPY_DEST , D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER );
	m_commandList->m_commandList->ResourceBarrier( 1 , &resourceBarrier );

	// Now we execute the command list to upload the initial assets (triangle data)
//	m_commandList->m_commandList->Close();
//	ID3D12CommandList* ppCommandLists[] = { m_commandList->m_commandList };
//	m_commandQueue->m_commandQueue->ExecuteCommandLists( _countof( ppCommandLists ) , ppCommandLists );
//	
//	// increment the fence value now, otherwise the buffer might not be uploaded by the time we start drawing
//	m_fenceValue++;
//	m_commandQueue->SignalFence( m_fenceValue );

	// create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = numVerts * sizeof( Vertex_PCU );
	m_vertexBufferView.StrideInBytes = sizeof( Vertex_PCU );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::CreateIndexBufferForIndexArray( std::vector<uint>& indices )
{
	if( indices.size() < 6 )
	{
		return;
	}
	// Create index buffer

	int iBufferSize = indices.size() * sizeof( uint );
	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT );
	CD3DX12_RESOURCE_DESC bufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer( iBufferSize );
	// create default heap to hold index buffer
	m_device->CreateCommittedResource(
		&heapProp , // a default heap
		D3D12_HEAP_FLAG_NONE , // no flags
		&bufferResourceDesc , // resource description for a buffer
		D3D12_RESOURCE_STATE_COPY_DEST , // start in the copy destination state
		nullptr , // optimized clear value must be null for this type of resource
		IID_PPV_ARGS( &m_indexBuffer ) );

	// we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
	m_indexBuffer->SetName( L"Index Buffer Resource Heap" );

	// create upload heap to upload index buffer
	CD3DX12_HEAP_PROPERTIES uploadHeapProp = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD );
	
	m_device->CreateCommittedResource(
		&uploadHeapProp , // upload heap
		D3D12_HEAP_FLAG_NONE , // no flags
		&bufferResourceDesc , // resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ , // GPU will read from this buffer and copy its contents to the default heap
		nullptr ,
		IID_PPV_ARGS( &m_indexBufferUploadHeap ) );
	m_indexBufferUploadHeap->SetName( L"Index Buffer Upload Resource Heap" );

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = reinterpret_cast< BYTE* >( &indices[0] ); // pointer to our index array
	indexData.RowPitch = iBufferSize; // size of all our index buffer
	indexData.SlicePitch = iBufferSize; // also the size of our index buffer

	// we are now creating a command with the command list to copy the data from
	// the upload heap to the default heap
	UpdateSubresources( m_commandList->m_commandList , m_indexBuffer , m_indexBufferUploadHeap , 0 , 0 , 1 , &indexData );

	// transition the vertex buffer data from copy destination state to vertex buffer state
	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition( m_indexBuffer , D3D12_RESOURCE_STATE_COPY_DEST , D3D12_RESOURCE_STATE_INDEX_BUFFER );
	m_commandList->m_commandList->ResourceBarrier( 1 , &resourceBarrier );

	// create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 32-bit unsigned integer (this is what a dword is, double word, a word is 2 bytes)
	m_indexBufferView.SizeInBytes = iBufferSize;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::CreateIndexBufferForIndexArray( uint numIndices , uint* indices )
{
	if ( numIndices < 6 )
	{
		return;
	}
	// Create index buffer

	int iBufferSize = numIndices * sizeof( uint );
	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT );
	CD3DX12_RESOURCE_DESC bufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer( iBufferSize );
	// create default heap to hold index buffer
	m_device->CreateCommittedResource(
		&heapProp , // a default heap
		D3D12_HEAP_FLAG_NONE , // no flags
		&bufferResourceDesc , // resource description for a buffer
		D3D12_RESOURCE_STATE_COPY_DEST , // start in the copy destination state
		nullptr , // optimized clear value must be null for this type of resource
		IID_PPV_ARGS( &m_indexBuffer ) );

	// we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
	m_indexBuffer->SetName( L"Index Buffer Resource Heap" );

	// create upload heap to upload index buffer
	CD3DX12_HEAP_PROPERTIES uploadHeapProp = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD );

	m_device->CreateCommittedResource(
		&uploadHeapProp , // upload heap
		D3D12_HEAP_FLAG_NONE , // no flags
		&bufferResourceDesc , // resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ , // GPU will read from this buffer and copy its contents to the default heap
		nullptr ,
		IID_PPV_ARGS( &m_indexBufferUploadHeap ) );
	m_indexBufferUploadHeap->SetName( L"Index Buffer Upload Resource Heap" );

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = reinterpret_cast< BYTE* >( &indices[ 0 ] ); // pointer to our index array
	indexData.RowPitch = iBufferSize; // size of all our index buffer
	indexData.SlicePitch = iBufferSize; // also the size of our index buffer

	// we are now creating a command with the command list to copy the data from
	// the upload heap to the default heap
	UpdateSubresources( m_commandList->m_commandList , m_indexBuffer , m_indexBufferUploadHeap , 0 , 0 , 1 , &indexData );

	// transition the vertex buffer data from copy destination state to vertex buffer state
	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition( m_indexBuffer , D3D12_RESOURCE_STATE_COPY_DEST , D3D12_RESOURCE_STATE_INDEX_BUFFER );
	m_commandList->m_commandList->ResourceBarrier( 1 , &resourceBarrier );

	// create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 32-bit unsigned integer (this is what a dword is, double word, a word is 2 bytes)
	m_indexBufferView.SizeInBytes = iBufferSize;
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

	DX_SAFE_RELEASE( m_indexBuffer );
	DX_SAFE_RELEASE( m_indexBufferUploadHeap );
	DX_SAFE_RELEASE( m_vertexBuffer );
	DX_SAFE_RELEASE( m_vertexBufferUploadHeap );

//	DX_SAFE_RELEASE( m_vertexBufferUploadHeap );
//	DX_SAFE_RELEASE( m_indexBufferUploadHeap );
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

void RenderContextDX12::ClearDepth( CommandListDX12* commandList , D3D12_CPU_DESCRIPTOR_HANDLE dsv , FLOAT depth /*= 1.0f */ )
{

}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::UpdateBufferResource( CommandListDX12* commandList , ID3D12Resource** pDestinationResource , ID3D12Resource** pIntermediateResource ,
												size_t numElements , size_t elementSize , const void* bufferData , D3D12_RESOURCE_FLAGS flags /*= D3D12_RESOURCE_FLAG_NONE */ )
{
	auto device = m_device;

	size_t bufferSize = numElements * elementSize;

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapProperties.CreationNodeMask = 0;
	heapProperties.VisibleNodeMask = 0;
	
	D3D12_RESOURCE_DESC heapDesc{};
	heapDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	heapDesc.Flags = flags;
	heapDesc.DepthOrArraySize = bufferSize;
	// Create a committed resource for the GPU resource in a default heap.
		m_device->CreateCommittedResource( &heapProperties , D3D12_HEAP_FLAG_NONE ,
			&heapDesc ,
			D3D12_RESOURCE_STATE_COPY_DEST ,
			nullptr ,
			IID_PPV_ARGS( pDestinationResource ) );
	
	// Create an committed resource for the upload.
	D3D12_HEAP_PROPERTIES uploadHeap{};
	uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
	uploadHeap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
	uploadHeap.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	uploadHeap.CreationNodeMask = 0;
	uploadHeap.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC uploadHeapDesc{};
	uploadHeapDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadHeapDesc.DepthOrArraySize = bufferSize;

	if ( bufferData )
	{
		m_device->CreateCommittedResource( &uploadHeap , D3D12_HEAP_FLAG_NONE ,
			&uploadHeapDesc ,
			D3D12_RESOURCE_STATE_GENERIC_READ ,
			nullptr ,
			IID_PPV_ARGS( pIntermediateResource ) );

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;
		
		UpdateSubresources( m_commandList->m_commandList ,
			*pDestinationResource , *pIntermediateResource ,
			0 , 0 , 1 , &subresourceData );
	}
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
	
	// get a handle to the depth/stencil buffer
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle( m_dsvHeap->GetCPUDescriptorHandleForHeapStart() );

	// set the render target for the output merger stage (the output of the pipeline)
	m_commandList->m_commandList->ClearRenderTargetView( rtv , clearFloats , 0 , nullptr );
	m_commandList->m_commandList->ClearDepthStencilView( m_dsvHeap->GetCPUDescriptorHandleForHeapStart() , D3D12_CLEAR_FLAG_DEPTH , 1.0f , 0 , 0 , nullptr );
	m_commandList->m_commandList->OMSetRenderTargets( 1 , &rtv , FALSE , &dsvHandle );
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

void RenderContextDX12::CreateRenderTargetViews()
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
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	// A single 32-bit constant root parameter that is used by the vertex shader.

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDescription{};
	
	D3D12_ROOT_PARAMETER rootParameters[ 3 ] = {};
	
	rootParameters[ 0 ].Constants.Num32BitValues = sizeof( FrameDataT ) / sizeof( float );
	rootParameters[ 0 ].Constants.ShaderRegister = 0;
	rootParameters[ 0 ].Constants.RegisterSpace = 0;
	rootParameters[ 0 ].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[ 0 ].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;

	rootParameters[ 1 ].Constants.Num32BitValues = sizeof( CameraDataT ) / sizeof( float );
	rootParameters[ 1 ].Constants.ShaderRegister = 1;
	rootParameters[ 1 ].Constants.RegisterSpace = 0;
	rootParameters[ 1 ].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[ 1 ].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;

	rootParameters[ 2 ].Constants.Num32BitValues = sizeof( ModelDataT ) / sizeof( float );
	rootParameters[ 2 ].Constants.ShaderRegister = 2;
	rootParameters[ 2 ].Constants.RegisterSpace = 0;
	rootParameters[ 2 ].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[ 2 ].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;

	rootSignatureDescription.NumParameters		=  _countof( rootParameters );
	rootSignatureDescription.pParameters		= rootParameters;
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
	m_commandList->m_commandList->DrawInstanced( 3 , 1 , 0 , 0 );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::DrawVertexArray( std::vector<Vertex_PCU>& verts )
{
	m_commandList->m_commandList->IASetVertexBuffers( 0 , 1 , &m_vertexBufferView ); // set the vertex buffer (using the vertex buffer view)
	m_commandList->m_commandList->DrawInstanced( verts.size() , 1 , 0 , 0 ); // finally draw 3 vertices (draw the triangle)
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::DrawIndexedVertexArray( std::vector<Vertex_PCU>& verts , std::vector<uint>& indices )
{
	m_commandList->m_commandList->IASetVertexBuffers( 0 , 1 , &m_vertexBufferView ); // set the vertex buffer (using the vertex buffer view)
	m_commandList->m_commandList->IASetIndexBuffer( &m_indexBufferView ); // set the vertex buffer (using the vertex buffer view)
	m_commandList->m_commandList->DrawIndexedInstanced( indices.size() , verts.size() , 0 , 0 , 0 ); // finally draw 3 vertices (draw the triangle)
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::DrawIndexedVertexArray( uint numVerts , uint numIndices )
{
	m_commandList->m_commandList->IASetVertexBuffers( 0 , 1 , &m_vertexBufferView ); // set the vertex buffer (using the vertex buffer view)
	m_commandList->m_commandList->IASetIndexBuffer( &m_indexBufferView ); // set the vertex buffer (using the vertex buffer view)
	m_commandList->m_commandList->DrawIndexedInstanced( numIndices , numVerts , 0 , 0 , 0 ); // finally draw 3 vertices (draw the triangle)
}

//--------------------------------------------------------------------------------------------------------------------------------------------

bool RenderContextDX12::CheckRaytracingSupport()
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
//	DX::ThrowIfFailed( m_device->CheckFeatureSupport( D3D12_FEATURE_D3D12_OPTIONS5 , &options5 , sizeof( options5 ) ) );
	if ( options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0 )
	{
		throw std::runtime_error( "Raytracing not supported on device" );
	}
	return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

AccelerationStructureBuffers RenderContextDX12::CreateBottomLevelAS( std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource> , uint32_t>> vVertexBuffers )
{
	nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS;

	// Adding all vertex buffers and not transforming their position.
	for ( const auto& buffer : vVertexBuffers )
	{
		bottomLevelAS.AddVertexBuffer( buffer.first.Get() , 0 , buffer.second ,
			sizeof( Vertex_PCU ) , 0 , 0 );
	}

	// The AS build requires some scratch space to store temporary information.
	// The amount of scratch memory is dependent on the scene complexity.
	UINT64 scratchSizeInBytes = 0;
	// The final AS also needs to be stored in addition to the existing vertex
	// buffers. It size is also dependent on the scene complexity.
	UINT64 resultSizeInBytes = 0;

	bottomLevelAS.ComputeASBufferSizes( m_device , false , &scratchSizeInBytes ,
		&resultSizeInBytes );

	// Once the sizes are obtained, the application is responsible for allocating
	// the necessary buffers. Since the entire generation will be done on the GPU,
	// we can directly allocate those on the default heap
	AccelerationStructureBuffers buffers;
	buffers.pScratch = nv_helpers_dx12::CreateBuffer(
		m_device, scratchSizeInBytes ,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS , D3D12_RESOURCE_STATE_COMMON ,
		nv_helpers_dx12::kDefaultHeapProps );

	buffers.pResult = nv_helpers_dx12::CreateBuffer(
		m_device , resultSizeInBytes ,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS ,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE ,
		nv_helpers_dx12::kDefaultHeapProps );

	// Build the acceleration structure. Note that this call integrates a barrier
	// on the generated AS, so that it can be used to compute a top-level AS right
	// after this method.
	bottomLevelAS.Generate( m_commandList->m_commandList , buffers.pScratch.Get() ,
		buffers.pResult.Get() , false , nullptr );

	return buffers;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::CreateTopLevelAS( const std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource> , DirectX::XMMATRIX>>& instances )
{
	// Gather all the instances into the builder helper
	for (size_t i = 0; i < instances.size(); i++) {
	  m_topLevelASGenerator.AddInstance(instances[i].first.Get(),
	                                    instances[i].second, static_cast<UINT>(i),
	                                    static_cast<UINT>(0));
	}

	// As for the bottom-level AS, the building the AS requires some scratch space
	// to store temporary data in addition to the actual AS. In the case of the
	// top-level AS, the instance descriptors also need to be stored in GPU
	// memory. This call outputs the memory requirements for each (scratch,
	// results, instance descriptors) so that the application can allocate the
	// corresponding memory
	UINT64 scratchSize, resultSize, instanceDescsSize;

	m_topLevelASGenerator.ComputeASBufferSizes( m_device , true , &scratchSize , &resultSize , &instanceDescsSize );

	// Create the scratch and result buffers. Since the build is all done on GPU,
	// those can be allocated on the default heap
	m_topLevelASBuffers.pScratch = nv_helpers_dx12::CreateBuffer( m_device , scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
																	D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nv_helpers_dx12::kDefaultHeapProps);

	m_topLevelASBuffers.pResult = nv_helpers_dx12::CreateBuffer( m_device , resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
																	D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nv_helpers_dx12::kDefaultHeapProps);

	// The buffer describing the instances: ID, shader binding information,
	// matrices ... Those will be copied into the buffer by the helper through
	// mapping, so the buffer has to be allocated on the upload heap.
	m_topLevelASBuffers.pInstanceDesc = nv_helpers_dx12::CreateBuffer( m_device , instanceDescsSize, D3D12_RESOURCE_FLAG_NONE,
																		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

	// After all the buffers are allocated, or if only an update is required, we
	// can build the acceleration structure. Note that in the case of the update
	// we also pass the existing AS as the 'previous' AS, so that it can be
	// refitted in place.
	m_topLevelASGenerator.Generate( m_commandList->m_commandList,
	m_topLevelASBuffers.pScratch.Get(),
	m_topLevelASBuffers.pResult.Get(),
	m_topLevelASBuffers.pInstanceDesc.Get() );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void RenderContextDX12::CreateAccelerationStructures()
{
	// Build the bottom AS from the Triangle vertex buffer
	AccelerationStructureBuffers bottomLevelBuffers =
		CreateBottomLevelAS( { {m_vertexBuffer, 3} } );

	const float indentityValues[] = {
								1.f , 0.f , 0.f , 0.f ,
								0.f , 1.f , 0.f , 0.f ,
								0.f , 0.f , 1.f , 0.f ,
								0.f , 0.f , 0.f , 1.f
							  };

	DirectX::XMMATRIX XM_IDENTITY( &indentityValues[0] );

	// Just one instance for now
	m_instances = { {bottomLevelBuffers.pResult, XM_IDENTITY } };
	CreateTopLevelAS( m_instances );
	// Flush the command list and wait for it to finish
	m_commandList->m_commandList->Close();
	ID3D12CommandList* const commandLists[] = {
		m_commandList->m_commandList
	};
	m_commandQueue->m_commandQueue->ExecuteCommandLists( _countof( commandLists ) , commandLists );

	m_fenceValue++;
	m_commandQueue->SignalFence( m_fenceValue );

	m_commandQueue->m_fence->WaitForFenceValue( m_fenceValue , m_fenceEvent );
	WaitForSingleObject( m_fenceEvent , INFINITE );

	// Once the command list is finished executing, reset it to be reused for
	// rendering
	ThrowIfFailed( m_commandList->m_commandList->Reset( m_commandAllocators[ m_currentBackBufferIndex ]->m_commandAllocator , m_pipelineState ) );

	// Store the AS buffers. The rest of the buffers will be released once we exit
	// the function
	m_bottomLevelAS = bottomLevelBuffers.pResult;
}

//--------------------------------------------------------------------------------------------------------------------------------------------
