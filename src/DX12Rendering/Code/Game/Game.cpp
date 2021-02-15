#include "Engine/Core/EngineCommon.hpp"
#include "Engine/RendererDX12/RenderContextDX12.hpp"
#include "Engine/Time/Time.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/TheApp.hpp"

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
}

//--------------------------------------------------------------------------------------------------------------------------------------------

Game::~Game()
{

}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::InitializeCameras()
{
	//m_gameCamera.SetProjectionPerspective( 60.f , CLIENT_ASPECT , -.1f , -100.f );
	//m_gameCamera.SetPosition( Vec3( 0.f , 0.f , 0.f ) );
	//m_gameCamera.SetClearMode( CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT , BLACK , 1.f , 0 );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::Update( float deltaSeconds )
{
	if ( ( m_colorLerpTimer >= 0.f ) && ( m_colorLerpTimer <= 3.f ) )
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
	}
	m_colorLerpTimer += deltaSeconds;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::Render() const
{
	g_theRenderer->ClearScreen( m_clearScreenColor );
// 	g_theRenderer->BeginCamera( m_gameCamera );
// 	g_theRenderer->SetRasterState( FILL_SOLID );
// 	g_theRenderer->EndCamera( m_gameCamera );
}

//--------------------------------------------------------------------------------------------------------------------------------------------