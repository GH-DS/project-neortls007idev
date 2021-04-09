#include "Engine/Core/EngineCommon.hpp"
#include "Engine/RendererDX12/RenderContextDX12.hpp"
#include "Engine/Time/Time.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/TheApp.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Primitives/AABB3.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/RendererDX12/CommandListDX12.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Core/OBJUtils.hpp"
#include "Engine/Input/VirtualKeyboard.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

extern RenderContextDX12*	g_theRenderer;
extern TheApp*				g_theApp;
static  bool				s_areDevconsoleCommandsLoaded = false;

//--------------------------------------------------------------------------------------------------------------------------------------------

Game::Game()
{
	InitializeCameras();
	g_theInput->PushCursorSettings( CursorSettings( RELATIVE_MODE , MOUSE_IS_UNLOCKED , false ) );
	m_clearScreenColor = RED;
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
}

//--------------------------------------------------------------------------------------------------------------------------------------------

Game::~Game()
{

}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::InitializeCameras()
{
	m_gameCamera.SetProjectionPerspective( 60.f , CLIENT_ASPECT , -.1f , -100.f );
	//m_gameCamera.SetOrthoView( 540.f , CLIENT_ASPECT );
	m_gameCamera.SetPosition( Vec3( 0.f , 0.f , 100.f ) );
	m_gameCamera.SetClearMode( CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT , BLACK , 1.f , 0 );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::Update( float deltaSeconds )
{
	m_framTime = deltaSeconds;
	float time = ( float ) GetCurrentTimeSeconds();
	float lerpValue = SinDegrees( time * 100.f );
	lerpValue *= lerpValue;
	m_clearScreenColor.LerpColor( ORANGE , PURPLE , lerpValue );
	
	/*if ( ( m_colorLerpTimer >= 0.f ) && ( m_colorLerpTimer <= 3.f ) )
	{
		m_clearScreenColor.LerpColorOverTime( RED , GREEN , 3.f , m_colorLerpTimer );
	}
	else if ( ( m_colorLerpTimer >= 3.f ) && ( m_colorLerpTimer <= 6.f ) )
	{
		m_clearScreenColor.LerpColorOverTime( GREEN , BLUE , 6.f , m_colorLerpTimer );
	}
	else if ( ( m_colorLerpTimer >= 6.f ) && ( m_colorLerpTimer < 9.f ) )
	{
		m_clearScreenColor.LerpColorOverTime( BLUE , RED , 9.f , m_colorLerpTimer );
	}
	else
	{
		m_colorLerpTimer = 0.f;
	}*/
	//m_clearScreenColor = BLACK;
	m_colorLerpTimer += deltaSeconds;

	m_cubeTestTransform.m_yaw += deltaSeconds * 5.f;
	m_modelTestTransform.m_yaw += deltaSeconds * 5.f;

	UpdateFromKeyBoard( deltaSeconds );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::Render() const
{
	g_theRenderer->ClearScreen( BLACK );
	UpdateCameraConstantBufferData();
	UpdateFrameTime( m_framTime );
	UpdateModelMatrix( m_modelTestTransform.GetAsMatrix() , m_clearScreenColor );

	g_theRenderer->CreateVertexBufferForVertexArray( m_modelMeshVerts );
	g_theRenderer->DrawVertexArray( m_modelMeshVerts );
	g_theRenderer->CreateVertexBufferForVertexArray( m_cubeMeshVerts );
	g_theRenderer->CreateIndexBufferForIndexArray( m_cubeMeshIndices );
	UpdateModelMatrix( m_cubeTestTransform.GetAsMatrix() );
	g_theRenderer->DrawIndexedVertexArray( m_cubeMeshVerts , m_cubeMeshIndices );
	//g_theRenderer->TestDraw();
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

void Game::UpdateFromKeyBoard( float deltaSeconds )
{
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

	float speed = 1.0f;

	if ( g_theInput->IsKeyHeldDown( KEY_SHIFT ) )
	{
		speed = 5.f;
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
		m_yaw				= 0.f;
		m_pitch				= 0.f;
	}

	Vec2 mousePos		= g_theInput->GetRelativeMovement();
	
	m_pitch -= mousePos.y * speed * deltaSeconds;
	m_yaw	-= mousePos.x * speed * deltaSeconds;

	m_pitch = Clamp( m_pitch , -90.f , 90.f );

	m_gameCamera.SetPitchYawRollRotation( m_pitch , m_yaw , 0.f );
	m_gameCamera.ConstructCameraViewFrustum();
}

//--------------------------------------------------------------------------------------------------------------------------------------------