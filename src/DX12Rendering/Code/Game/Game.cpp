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
	g_theInput->PushCursorSettings( CursorSettings( RELATIVE_MODE , MOUSE_IS_WINDOWLOCKED , false ) );
	m_colorLerpTimer = 0.f;

	InitializeModelMeshesAndTransforms();


	m_triangle.emplace_back( Vertex_PCU( Vec3( 0.f , 0.5f , 0.0f ) , GREEN , Vec2::ZERO ) );
	m_triangle.emplace_back( Vertex_PCU( Vec3( -0.25f , -0.25f , 0.0f ) , BLUE , Vec2::ONE ) );
	m_triangle.emplace_back( Vertex_PCU( Vec3( 0.25f , -0.25f , 0.0f ) , RED , Vec2( 0.5f , 1.f ) ) );
	
	InitializePlane();
	g_theRenderer->m_raster = false;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::InitializeModelMeshesAndTransforms()
{
	std::vector<VertexMaster> modelverts;
	std::vector<uint> modelIndices;
	MeshBuilderOptions buildMesh;
	buildMesh.ignoreUVs = true;

	LoadObjFileIntoVertexBuffer( modelverts , modelIndices , buildMesh , "Data/Models/Gems/Crystal_006.obj" );
	VertexMaster::ConvertVertexMasterToVertexPCU( m_modelMeshVerts[ GM_CRYSTAL1 ] , modelverts );
	m_modelTransforms[ GM_CRYSTAL1 ].SetPosition( Vec3( -15.f , -3.f , 15.f ) );
	m_modelTintColors[ GM_CRYSTAL1 ] = HALF_ALPHA_CYAN;
	modelverts.clear();
	modelIndices.clear();


	LoadObjFileIntoVertexBuffer( modelverts , modelIndices , buildMesh , "Data/Models/Gems/Crystal_007.obj" );
	VertexMaster::ConvertVertexMasterToVertexPCU( m_modelMeshVerts[ GM_CRYSTAL2 ] , modelverts );
	m_modelTransforms[ GM_CRYSTAL2 ].SetPosition( Vec3( 15.f , -3.0f , 15.f ) );
	m_modelTintColors[ GM_CRYSTAL2 ] = HALF_ALPHA_PINK;
	modelverts.clear();
	modelIndices.clear();

	LoadObjFileIntoVertexBuffer( modelverts , modelIndices , buildMesh , "Data/Models/Gems/Gem_001.obj" );
	VertexMaster::ConvertVertexMasterToVertexPCU( m_modelMeshVerts[ GM_GEM ] , modelverts );
	m_modelTransforms[ GM_GEM ].SetPosition( Vec3( -15.0f , -3.f , -15.f ) );
	m_modelTintColors[ GM_GEM ] = HALF_ALPHA_WHITE;
	modelverts.clear();
	modelIndices.clear();

	LoadObjFileIntoVertexBuffer( modelverts , modelIndices , buildMesh , "Data/Models/Gems/Diamond_002.obj" );
	VertexMaster::ConvertVertexMasterToVertexPCU( m_modelMeshVerts[ GM_DIAMOND ] , modelverts );
	m_modelTransforms[ GM_DIAMOND ].SetPosition( Vec3( 15.0f , -3.f , -15.f ) );
	m_modelTintColors[ GM_DIAMOND ] = HALF_ALPHA_BLUE;
	modelverts.clear();
	modelIndices.clear();

	LoadObjFileIntoVertexBuffer( modelverts , modelIndices , buildMesh , "Data/Models/Gems/Hexagon_Gem_002.obj" );
	VertexMaster::ConvertVertexMasterToVertexPCU( m_modelMeshVerts[ GM_HEX ] , modelverts );
	m_modelTransforms[ GM_HEX ].SetPosition( Vec3( 0.f , -3.f , 0.f ) );
	m_modelTintColors[ GM_HEX ] = HALF_ALPHA_RED;
	modelverts.clear();
	modelIndices.clear();

	LoadObjFileIntoVertexBuffer( modelverts , modelIndices , buildMesh , "Data/Models/Gems/Octagonal_Gem_001.obj" );
	VertexMaster::ConvertVertexMasterToVertexPCU( m_modelMeshVerts[ GM_OCT ] , modelverts );
	m_modelTransforms[ GM_OCT ].SetPosition( Vec3( 0.f , 3.f , 0.f ) );
	m_modelTintColors[ GM_OCT ] = HALF_ALPHA_GREEN;
	modelverts.clear();
	modelIndices.clear();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

Game::~Game()
{

}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::InitializeCameras()
{
	m_gameCamera.SetProjectionPerspective( GAME_CAM_FOV , CLIENT_ASPECT , -GAME_CAM_NEAR_Z , -GAME_CAM_FAR_Z );
	//m_gameCamera.SetOrthoView( 0.5f , 1.f );
	m_gameCamera.SetPosition( Vec3( 0.f , 0.f , -5.f ) );
	m_gameCamera.SetPitchYawRollRotation( 0.f , 180.f , 0.f );
	m_yaw = 180.f;
	m_gameCamera.SetClearMode( CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT , BLACK , 1.f , 0 );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::InitializePlane()
{
	m_plane.emplace_back( Vertex_PCU( Vec3( -15.f , -8.f , 15.f ) , WHITE , Vec2::ZERO ) );
	m_plane.emplace_back( Vertex_PCU( Vec3(  15.f , -8.0f , 15.f ) , WHITE , Vec2::ZERO ) );
	m_plane.emplace_back( Vertex_PCU( Vec3( -15.0f , -8.f , -15.f ) , WHITE , Vec2::ZERO ) );
	m_plane.emplace_back( Vertex_PCU( Vec3( 15.f , -8.f , 15.f ) , WHITE , Vec2::ZERO ) );
	m_plane.emplace_back( Vertex_PCU( Vec3( 15.0f , -8.f , -15.f ) , WHITE , Vec2::ZERO ) );
	m_plane.emplace_back( Vertex_PCU( Vec3( -15.f , -8.f , -15.f ) , WHITE , Vec2::ZERO ) );
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
	UpdateFromKeyboard( deltaSeconds );
	UpdateRenderingOrder();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::Render()
{

	if( g_theRenderer->m_raster )
	{
		g_theRenderer->ClearScreen( m_clearScreenColor );
		
		UpdateCameraConstantBufferData();
		UpdateFrameTime( m_framTime );
		UpdateModelMatrix( Mat44::IDENTITY , WHITE );

		g_theRenderer->CreateVertexBufferForVertexArray( m_plane );
		g_theRenderer->DrawVertexArray( m_plane );
		
		for( int index = 0 ; index < NUM_MODELS ; index++ )
		{
			int modelIndex = m_renderingOrder[ index ].modelindex;

			g_theRenderer->CreateVertexBufferForVertexArray( m_modelMeshVerts[ modelIndex ] );
			UpdateModelMatrix( m_modelTransforms[ modelIndex ].GetAsMatrix() , m_modelTintColors[ modelIndex ] );
			g_theRenderer->DrawVertexArray( m_modelMeshVerts[ modelIndex ] );
		}
	}
	else
	{
		g_theRenderer->ClearScreen( m_clearScreenColorRT );
		g_theRenderer->CreateVertexBufferForVertexArray( m_modelMeshVerts[ GM_DIAMOND ] );
		g_theRenderer->CreatePlaneVertexBuffer();
 		g_theRenderer->CreateAccelerationStructures();
		g_theRenderer->CreateRaytracingOutputBuffer();
		g_theRenderer->CreateCameraBuffer();
		g_theRenderer->CreateShaderResourceHeap();
		g_theRenderer->UpdateCameraBuffer( &m_gameCamera );
		g_theRenderer->CreateShaderBindingTable();
		
		g_theRenderer->ClearScreenRT();
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

void Game::UpdateFromKeyboard( float deltaSeconds )
{
	if ( g_theInput->WasKeyJustPressed( KEY_SPACE ) )
	{
		g_theRenderer->m_raster = !g_theRenderer->m_raster;
	}

	CameraPositionUpdateOnInput( deltaSeconds );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::CameraPositionUpdateOnInput( float deltaSeconds )
{
	Vec3 rotation = Vec3::ZERO;

	Mat44 cameraTransform	= m_gameCamera.GetCameraTransform().GetAsMatrix();
	Vec3 forwardVector		= -cameraTransform.GetKBasis3D();
	Vec3 rightVector		= cameraTransform.GetIBasis3D();
	Vec3 UpVector			= Vec3::UNIT_VECTOR_ALONG_J_BASIS;

	float speed = 1.f;

	if ( g_theInput->IsKeyHeldDown( KEY_SHIFT ) )
	{
		speed = 4.f;
	}

	if ( g_theInput->IsKeyHeldDown( 'A' ) )
	{
		m_gameCamera.SetPosition( m_gameCamera.GetPosition() - rightVector * speed * deltaSeconds );
	}
	if ( g_theInput->IsKeyHeldDown( 'D' ) )
	{
		m_gameCamera.SetPosition( m_gameCamera.GetPosition() + rightVector * speed * deltaSeconds );
	}
	if ( g_theInput->IsKeyHeldDown( 'W' ) )
	{
		m_gameCamera.SetPosition( m_gameCamera.GetPosition() + forwardVector * speed * deltaSeconds );
	}
	if ( g_theInput->IsKeyHeldDown( 'S' ) )
	{
		m_gameCamera.SetPosition( m_gameCamera.GetPosition() - forwardVector * speed * deltaSeconds );
	}
	if ( g_theInput->IsKeyHeldDown( 'Q' ) )
	{
		m_gameCamera.SetPosition( m_gameCamera.GetPosition() - UpVector * speed * deltaSeconds );
	}
	if ( g_theInput->IsKeyHeldDown( 'E' ) )
	{
		m_gameCamera.SetPosition( m_gameCamera.GetPosition() + UpVector * speed * deltaSeconds );
	}

	if ( g_theInput->WasKeyJustPressed( 'O' ) )
	{
		m_gameCamera.SetPosition( Vec3::ZERO );
		m_yaw = 0.f;
		m_pitch = 0.f;
	}

	Vec2 mousePos = g_theInput->GetRelativeMovement();

	m_pitch -= mousePos.y * speed * deltaSeconds;
	m_yaw -= mousePos.x * speed * deltaSeconds;

	m_pitch = Clamp( m_pitch , -90.f , 90.f );

	m_gameCamera.SetPitchYawRollRotation( m_pitch , m_yaw , 0.f );
	m_gameCamera.ConstructCameraViewFrustum();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateRenderingOrder()
{
	m_renderingOrder.clear();

	Vec3 camPos = m_gameCamera.GetPosition();
	TransparencyRenderOrder modelRO;

	for( int index = 0 ; index < NUM_MODELS ; index++ )
	{
		modelRO.distanceSq = ( m_modelTransforms[ index ].GetPostion() - camPos ).GetLengthSquared();
		modelRO.modelindex = index;
		m_renderingOrder.emplace_back( modelRO );
	}

	std::sort( m_renderingOrder.begin() , m_renderingOrder.end() , customGreaterTransparencyRenderOrder );
}

//--------------------------------------------------------------------------------------------------------------------------------------------