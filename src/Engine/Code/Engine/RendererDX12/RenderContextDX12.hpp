#pragma once
#include "Engine/Platform/Window.hpp"
#include "Engine/RendererDX12/D3D12Utils.hpp"
#include <chrono>
#include <d3dcommon.h>

//--------------------------------------------------------------------------------------------------------------------------------------------

#include <dxcapi.h>
#include <wrl/client.h>
#include "ThirdParty/DXRHelper/TopLevelASGenerator.h"
#include "ThirdParty/DXRHelper/ShaderBindingTableGenerator.h"
#include "ThirdParty/DXRHelper/BottomLevelASGenerator.h"

//--------------------------------------------------------------------------------------------------------------------------------------------

struct  ID3D12Resource;
struct  IDXGISwapChain4;
struct	ID3D12Device;
struct	ID3D12DeviceContext;
struct	ID3D12InfoQueue;
struct	IDXGIAdapter4;
struct	IDXGIDebug;
struct  ID3D12Debug;
struct  ID3D12RootSignature;

class	Window;
class	CommandQueueDX12;
class	DirectQueueDX12;
class	DescriptorHeapDX12;
class	CommandAllocatorDX12;
class	CommandListDX12;
class	ShaderDX12;

//--------------------------------------------------------------------------------------------------------------------------------------------

// DXR
struct AccelerationStructureBuffers
{
	Microsoft::WRL::ComPtr<ID3D12Resource> pScratch;      // Scratch memory for AS builder
	Microsoft::WRL::ComPtr<ID3D12Resource> pResult;       // Where the AS is
	Microsoft::WRL::ComPtr<ID3D12Resource> pInstanceDesc; // Hold the matrices of the instances
};


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
	
	HRESULT					CreateSwapChain( CommandQueueDX12* commandQueue , uint32_t bufferCount );
	void*					CreateFenceEventHandle();
	void					Present();
	void					Flush( uint64_t& fenceValue );

	void					ClearDepth( CommandListDX12* commandList ,	D3D12_CPU_DESCRIPTOR_HANDLE dsv , FLOAT depth = 1.0f );

	void					UpdateBufferResource( CommandListDX12* commandList , ID3D12Resource** pDestinationResource , ID3D12Resource** pIntermediateResource ,
													size_t numElements , size_t elementSize , const void* bufferData , D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE );

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
	void					CreateRenderTargetViews();
	void					CreateRootSignature();
//--------------------------------------------------------------------------------------------------------------------------------------------
	void					CreateVertexBufferForVertexArray( std::vector<Vertex_PCU>& verts );
	void					CreateVertexBufferForVertexArray( uint numVerts , Vertex_PCU* verts );
	void					CreateIndexBufferForIndexArray( std::vector<uint>& indices );
	void					CreateIndexBufferForIndexArray( uint numIndices , uint* indices );
	void					TestDraw();
	void					DrawVertexArray( std::vector<Vertex_PCU>& verts );
	void					DrawIndexedVertexArray( std::vector<Vertex_PCU>& verts , std::vector<uint>& indices );
	void					DrawIndexedVertexArray( uint numVerts , uint numIndices );

//--------------------------------------------------------------------------------------------------------------------------------------------
//				DXR
//--------------------------------------------------------------------------------------------------------------------------------------------
	
	bool					CheckRaytracingSupport();
	
	/// Create the acceleration structure of an instance
///
/// \param     vVertexBuffers : pair of buffer and vertex count
/// \return    AccelerationStructureBuffers for TLAS
	AccelerationStructureBuffers CreateBottomLevelAS( std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource> , uint32_t>> vVertexBuffers );

	/// Create the main acceleration structure that holds
	/// all instances of the scene
	/// \param     instances : pair of BLAS and transform
	void CreateTopLevelAS( const std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource> , DirectX::XMMATRIX>>& instances );
	
	/// Create all acceleration structures, bottom and top
	void CreateAccelerationStructures();

	// #DXR
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRayGenSignature();
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateMissSignature();
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateHitSignature();
	void CreateRaytracingPipeline();
	void CreateRaytracingOutputBuffer();
	void CreateShaderResourceHeap();
	void CreateShaderBindingTable();

	//--------------------------------------------------------------------------------------------------------------------------------------------
			
	void ClearScreenRT();											// Clear Color Ray traced Version
	void DispatchRays();

	void CreateCameraBuffer();
	void UpdateCameraBuffer();
public:

	Window*														m_window									= nullptr;
	ID3D12Device5*												m_device									= nullptr;
	ID3D12DeviceContext*										m_context									= nullptr;
	void*														m_debugModule								= nullptr;
	IDXGIDebug*													m_debug										= nullptr;
	ID3D12Debug*												m_dx12DebugModule							= nullptr;		
	ID3D12InfoQueue*											m_infoQueue									= nullptr;
	IDXGIAdapter4*												m_deviceAdapter								= nullptr;
	bool														m_isVsyncEnabled							= false;
	bool														m_hasTearingSupport							= false;

	const uint8_t												m_numBackBufferFrames						= 3;
	uint64_t													m_frameFenceValues[ 3 ]						= {};
	uint64_t													m_fenceValue								= 0;

	ID3D12Resource*												t_backBuffers[ 3 ];
	IDXGISwapChain4*											t_swapchain									= nullptr;
	DescriptorHeapDX12*											m_RTVDescriptorHeap							= nullptr;
	UINT														m_RTVDescriptorSize							= 0;

//	DirectQueueDX12*											m_directCommandQueue						= nullptr;
	CommandQueueDX12*											m_commandQueue								= nullptr;
	uint8_t														m_currentBackBufferIndex					= 0;

	CommandAllocatorDX12*										m_commandAllocators[ 3 ];
	CommandListDX12*											m_commandList								= nullptr;
	HANDLE														m_fenceEvent								= nullptr;
	
	ID3D12RootSignature*										m_rootSignature								= nullptr;
	ID3DBlob*													m_rootSignatureBlob							= nullptr;
	ID3DBlob*													m_errorBlob									= nullptr;
	
	D3D12_GRAPHICS_PIPELINE_STATE_DESC							m_pipelineStateDesc;
	ID3D12PipelineState*										m_pipelineState								= nullptr;
	ShaderDX12*													m_currentShader								= nullptr;
	ShaderDX12*													m_defaultShader								= nullptr;

	D3D12_VIEWPORT												m_viewPort;
	D3D12_RECT													m_scisrroRec;

	// Vertex buffer for the cube.
	D3D12_VERTEX_BUFFER_VIEW									m_vertexBufferView							= {};
	ID3D12Resource*												m_vertexBuffer								= nullptr;
	ID3D12Resource*												m_vertexBufferUploadHeap					= nullptr;
	// Index buffer for the cube.
	D3D12_INDEX_BUFFER_VIEW										m_indexBufferView							= {};
	ID3D12Resource*												m_indexBuffer								= nullptr;
	ID3D12Resource*												m_indexBufferUploadHeap						= nullptr;
	
	// Rasterizer State
	D3D12_RASTERIZER_DESC										m_rasterizerStateDesc{};

	// Depth Stencil State
	D3D12_DEPTH_STENCIL_DESC									m_depthStencilDesc{};
	// Depth buffer.
	ID3D12Resource*												m_depthStencilBuffer						= nullptr;
	// Descriptor heap for depth buffer.
	ID3D12DescriptorHeap*										m_dsvHeap									= nullptr;

//--------------------------------------------------------------------------------------------------------------------------------------------
//				DXR
//--------------------------------------------------------------------------------------------------------------------------------------------
	// Boolean to switch between Raster and Raytracing mode
	bool														m_raster									= true;
	Microsoft::WRL::ComPtr<ID3D12Resource>						m_bottomLevelAS;											// Storage for the bottom Level AS
	
	nv_helpers_dx12::TopLevelASGenerator						m_topLevelASGenerator;										
	AccelerationStructureBuffers								m_topLevelASBuffers;										// Storage for the top Level AS
	
	std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource> , DirectX::XMMATRIX>> m_instances;
	Microsoft::WRL::ComPtr<IDxcBlob>							m_rayGenLibrary;
	Microsoft::WRL::ComPtr<IDxcBlob>							m_hitLibrary;
	Microsoft::WRL::ComPtr<IDxcBlob>							m_missLibrary;
	Microsoft::WRL::ComPtr<IDxcBlob>							m_shadowLibrary;

	Microsoft::WRL::ComPtr<ID3D12RootSignature>					m_rayGenSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>					m_hitSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>					m_missSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>					m_shadowSignature;

	// Ray tracing pipeline state
	Microsoft::WRL::ComPtr<ID3D12StateObject>					m_rtStateObject;
	// Ray tracing pipeline state properties, retaining the shader identifiers
	// to use in the Shader Binding Table
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties>			m_rtStateObjectProps;
	Microsoft::WRL::ComPtr<ID3D12Resource>						m_outputResource;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>				m_srvUavHeap;
	nv_helpers_dx12::ShaderBindingTableGenerator				m_sbtHelper;
	
	Microsoft::WRL::ComPtr<ID3D12Resource>						m_sbtStorage;	

	Microsoft::WRL::ComPtr< ID3D12Resource >					m_cameraBuffer;
	Microsoft::WRL::ComPtr< ID3D12DescriptorHeap >				m_constHeap;
	uint32_t													m_cameraBufferSize = 0;
};

//--------------------------------------------------------------------------------------------------------------------------------------------