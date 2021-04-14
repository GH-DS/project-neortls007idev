#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/OBJUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/VirtualKeyboard.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Primitives/AABB3.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/RendererDX12/CommandListDX12.hpp"
#include "Engine/RendererDX12/RenderContextDX12.hpp"
#include "Engine/Time/Time.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/TheApp.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

extern RenderContextDX12*	g_theRenderer;
extern TheApp*				g_theApp;

//--------------------------------------------------------------------------------------------------------------------------------------------

Game::Game()
{
	InitializeCameras();
	g_theInput->PushCursorSettings( CursorSettings( ABSOLUTE_MODE , MOUSE_IS_UNLOCKED , true ) );
	m_colorLerpTimer = 0.f;

	m_cubeTestTransform.SetPosition( Vec3( 0.f , 3.f , 90.f ) );
	AABB3 box( Vec3( -0.5f , -0.5f , -0.5f ) , Vec3( 0.5f , 0.5f , 0.5f ) );
	Rgba8 tint( 255 , 255 , 255 , 255 );

	AddCubeVerts( m_cubeMeshVerts , nullptr );
	uint* indices = GetCubeIndices();
	for( int index = 0 ; index < 36 ; index++ )
	{
		m_cubeMeshIndices.push_back( indices[ index ] );
	}

	std::vector<VertexMaster> modelverts;
	MeshBuilderOptions buildMesh;
	LoadObjFileIntoVertexBuffer( modelverts , m_modelMeshIndices , buildMesh , "Data/Models/scifiFighter/mesh.obj" );
	VertexMaster::ConvertVertexMasterToVertexPCU( m_modelMeshVerts , modelverts );
	m_modelTestTransform.SetPosition( Vec3( 0.f , 0.f , 75.f ) );

	m_triangle.emplace_back( Vertex_PCU( Vec3( 0.f , 0.5f , 0.0f ) , GREEN , Vec2::ZERO ) );
	m_triangle.emplace_back( Vertex_PCU( Vec3( -0.25f , -0.25f , 0.0f ) , BLUE , Vec2::ONE ) );
	m_triangle.emplace_back( Vertex_PCU( Vec3( 0.25f , -0.25f , 0.0f ) , RED , Vec2( 0.5f , 1.f ) ) );

}

//--------------------------------------------------------------------------------------------------------------------------------------------

Game::~Game()
{

}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::InitializeCameras()
{
	//m_gameCamera.SetProjectionPerspective( 60.f , CLIENT_ASPECT , -.1f , -100.f );
	m_gameCamera.SetOrthoView( 0.5f , 1.f );
	//m_gameCamera.SetPosition( Vec3( 0.f , 0.f , 100.f ) );
	m_gameCamera.SetClearMode( CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT , BLACK , 1.f , 0 );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::Update( float deltaSeconds )
{
	m_framTime = deltaSeconds;
	float time = ( float ) GetCurrentTimeSeconds();
// 	float lerpValue = SinDegrees( time * 100.f );
// 	lerpValue *= lerpValue;
// 	m_clearScreenColor.LerpColor( ORANGE , PURPLE , lerpValue );
	
	m_colorLerpTimer += deltaSeconds;
//	m_triangle[ 0 ].m_position = Vec3( 0.f , 0.5f + SinDegrees( 50.f * time ) , 0.0f );
	//m_cubeTestTransform.m_yaw += deltaSeconds * 5.f;
	//m_modelTestTransform.m_yaw += deltaSeconds * 5.f;
	UpdateFromKeyboard();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::Render()
{

	if( g_theRenderer->m_raster )
	{
		g_theRenderer->ClearScreen( m_clearScreenColor );
		//g_theRenderer->TestDraw();

		UpdateCameraConstantBufferData();
		UpdateFrameTime( m_framTime );
		UpdateModelMatrix( Mat44::IDENTITY , WHITE );
		//
		g_theRenderer->CreateVertexBufferForVertexArray( m_triangle );
		g_theRenderer->DrawVertexArray( m_triangle );
		//g_theRenderer->CreateVertexBufferForVertexArray( m_cubeMeshVerts );
		//g_theRenderer->CreateIndexBufferForIndexArray( m_cubeMeshIndices );
		//UpdateModelMatrix( m_cubeTestTransform.GetAsMatrix() );
		//g_theRenderer->DrawIndexedVertexArray( m_cubeMeshVerts , m_cubeMeshIndices );
	}
	else
	{
		//
// 		UpdateCameraConstantBufferData();
// 		UpdateFrameTime( m_framTime );
//		UpdateModelMatrix( Mat44::IDENTITY , WHITE );
		g_theRenderer->ClearScreen( m_clearScreenColorRT );
		g_theRenderer->CreateVertexBufferForVertexArray( m_triangle );
 		g_theRenderer->CreateAccelerationStructures();
		g_theRenderer->CreateRaytracingOutputBuffer();
		g_theRenderer->CreateShaderResourceHeap();
		g_theRenderer->CreateShaderBindingTable();
		g_theRenderer->ClearScreenRT();
		//g_theRenderer->DispatchRays();
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateFrameTime( float deltaSeconds ) const
{
	FrameDataT frameData;
	frameData.m_systemTime = ( float ) GetCurrentTimeSeconds();
	frameData.m_systemDeltaTime = deltaSeconds;
	//m_frameUBO->m_isDirty = true;
	g_theRenderer->m_commandList->m_commandList->SetGraphicsRoot32BitConstants( 0 , sizeof( FrameDataT ) / sizeof( float ) , &frameData , 0 );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateCameraConstantBufferData() const
{
	CameraDataT cameraData;

	cameraData.cameraToClipTransform = m_gameCamera.GetProjectionMatrix();
	cameraData.view = m_gameCamera.GetViewMatrix();
	cameraData.position = m_gameCamera.GetPosition();
	g_theRenderer->m_commandList->m_commandList->SetGraphicsRoot32BitConstants( 1 , sizeof( CameraDataT ) / sizeof( float ) , &cameraData , 0 );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateModelMatrix( Mat44 modelMatrix , Rgba8 tint /*= WHITE */ ) const
{
	ModelDataT modelData;
	Vec4 tintAsFloats = tint.GetAsNormalizedFloat4();
	modelData.model = modelMatrix;
	modelData.normalizedModelColor = tintAsFloats;

	g_theRenderer->m_commandList->m_commandList->SetGraphicsRoot32BitConstants( 2 , sizeof( ModelDataT ) / sizeof( float ) , &modelData , 0 );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateFromKeyboard()
{
	if ( g_theInput->WasKeyJustPressed( KEY_SPACE ) )
	{
		g_theRenderer->m_raster = !g_theRenderer->m_raster;
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------