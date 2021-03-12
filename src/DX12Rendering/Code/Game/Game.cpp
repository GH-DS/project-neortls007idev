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

//--------------------------------------------------------------------------------------------------------------------------------------------

extern RenderContextDX12*	g_theRenderer;
extern TheApp*				g_theApp;
static  bool				s_areDevconsoleCommandsLoaded = false;

//--------------------------------------------------------------------------------------------------------------------------------------------

Game::Game()
{
	InitializeCameras();
	g_theInput->PushCursorSettings( CursorSettings( ABSOLUTE_MODE , MOUSE_IS_UNLOCKED , true ) );
	m_clearScreenColor = RED;
	m_colorLerpTimer = 0.f;

	AABB3 box( Vec3( -0.5f , -0.5f , -0.5f ) , Vec3( 0.5f , 0.5f , 0.5f ) );
	CreateCuboid( m_cubeMeshVerts , m_cubeMeshIndices , box , WHITE );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

Game::~Game()
{

}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::InitializeCameras()
{
	m_gameCamera.SetProjectionPerspective( 60.f , CLIENT_ASPECT , -.1f , -100.f );
	m_gameCamera.SetOrthoView( 540.f , CLIENT_ASPECT );
	//m_gameCamera.SetPosition( Vec3( 0.f , 0.f , 0.f ) );
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
	m_clearScreenColor = BLACK;
	m_colorLerpTimer += deltaSeconds;

	
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::Render() const
{
	g_theRenderer->ClearScreen( m_clearScreenColor );
	UpdateCameraConstantBufferData();
	UpdateFrameTime( m_framTime );
	UpdateModelMatrix( Mat44::IDENTITY );
	AABB2 box( -200.5f , -200.5f , 200.5f , 200.5f );
	std::vector<Vertex_PCU> testAABB;

	AppendVertsForAABB2( testAABB , box , WHITE );
	g_theRenderer->CreateVertexBufferForVertexArray( testAABB );
	g_theRenderer->DrawVertexArray( testAABB );

	AABB2 box2( -100.5f , -100.5f , 100.5f , 100.5f );
	std::vector<Vertex_PCU> testAABBindexedVerts;
	std::vector<uint> testAABBindices;

	const Vertex_PCU boxVerts[ 4 ] = {
								Vertex_PCU( Vec3( box2.m_mins.x,box2.m_mins.y,0.f ) , RED , Vec2( 0.f, 0.f ) ),
								Vertex_PCU( Vec3( box2.m_maxs.x,box2.m_mins.y,0.f ) , RED , Vec2( 1.f, 0.f ) ),
								Vertex_PCU( Vec3( box2.m_mins.x,box2.m_maxs.y,0.f ) , RED , Vec2( 0.f, 1.f ) ),
								Vertex_PCU( Vec3( box2.m_maxs.x,box2.m_maxs.y,0.f ) , RED , Vec2( 1.f, 1.f ) ) };

	for ( int index = 0; index < 4; index++ )
	{
		testAABBindexedVerts.push_back( boxVerts[ index ] );
	}
	
	testAABBindices.push_back( 0 );
	testAABBindices.push_back( 1 );
	testAABBindices.push_back( 2 );

	testAABBindices.push_back( 1 );
	testAABBindices.push_back( 3 );
	testAABBindices.push_back( 2 );
	g_theRenderer->CreateVertexBufferForVertexArray( testAABBindexedVerts );
	g_theRenderer->CreateIndexBufferForIndexArray( testAABBindices );
	g_theRenderer->DrawIndexedVertexArray( testAABBindexedVerts , testAABBindices );
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