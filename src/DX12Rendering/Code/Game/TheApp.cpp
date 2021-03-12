#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Input/VirtualKeyboard.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/RendererDX12/RenderContextDX12.hpp"
#include "Engine/Time/Clock.hpp"
#include "Engine/Time/Time.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/TheApp.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

extern RenderContextDX12*					g_theRenderer;
extern InputSystem*							g_theInput;
//extern DevConsole*						g_theDevConsole;
//extern ImGUISystem*						g_debugUI;
//extern BitmapFont*						g_bitmapFont;
//extern DebugRenderObjectsManager*			g_currentManager;

	   TheApp*								g_theApp		= nullptr;
	   Game*								g_theGame		= nullptr;

//--------------------------------------------------------------------------------------------------------------------------------------------

TheApp::TheApp()
{

}

//--------------------------------------------------------------------------------------------------------------------------------------------

TheApp::~TheApp()
{
	delete g_theGame;
	g_theGame = nullptr;

	delete g_theRenderer;
	g_theRenderer = nullptr;

	delete g_theInput;
	g_theInput = nullptr;

	delete g_theEventSystem;
	g_theEventSystem = nullptr;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void TheApp::Startup()
{
	
	Clock::Startup();

	if ( g_theEventSystem == nullptr )
	{
		g_theEventSystem = new EventSystem();
	}
	g_theEventSystem->Startup();

	if ( g_theWindow == nullptr )
	{
		g_theWindow = new Window();
	}

	if ( g_theInput == nullptr )
	{
		g_theInput = new InputSystem();
		g_theWindow->SetInputSystem( g_theInput );
	}
	g_theInput->Startup();

	if ( g_theRenderer == nullptr )
	{
		g_theRenderer = new RenderContextDX12();
	}
	g_theRenderer->Startup( g_theWindow );
	
	if ( g_theGame == nullptr )
	{
		g_theGame = new Game();
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void TheApp::RunFrame()
{
	static double timeLastFrameStarted = GetCurrentTimeSeconds();
	double        timeThisFrameStarted = GetCurrentTimeSeconds();
	double		  deltaSeconds		   = timeThisFrameStarted - timeLastFrameStarted;
	timeLastFrameStarted			   = timeThisFrameStarted;

	BeginFrame();                        
	Update( ( float ) deltaSeconds );
	Render();
	EndFrame();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void TheApp::BeginFrame()
{
	// all engine things that must begin at the beginning of each frame and not the game
	Clock::BeginFrame();
	g_theEventSystem->BeginFrame();
	g_theWindow->BeginFrame();
	g_theInput->BeginFrame();
	g_theRenderer->BeginFrame();

}

//--------------------------------------------------------------------------------------------------------------------------------------------

void TheApp::Update( float deltaSeconds )
{
	g_theRenderer->UpdateFrameTime( deltaSeconds );

	UpdateFromKeyboard();

	if ( m_isPaused )							{ deltaSeconds = 0; }
	else if ( m_isSloMo == true )				{ deltaSeconds /= 10.f; }
	if ( m_isSpeedMo )							{ deltaSeconds = deltaSeconds * 4.0f; }

	g_theGame->Update( 0.01667f );

	g_theInput->EndFrame();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void TheApp::Render() const
{
	g_theGame->Render();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void TheApp::EndFrame()
{
	g_theRenderer->EndFrame();
	g_theInput->EndFrame();

	Clock::EndFrame();
}


//--------------------------------------------------------------------------------------------------------------------------------------------

void TheApp::Shutdown()
{
	g_theRenderer->Shutdown();
	g_theInput->Shutdown();
	g_theWindow->Shutdown();
	g_theEventSystem->Shutdown();
	Clock::Shutdown();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void TheApp::UpdateFromKeyboard()
{
	if ( g_theInput->GetButtonState( KEY_ESC ).WasJustPressed() ) 
	{ 
		g_theWindow->HandleQuitRequested(); 
	}

	if ( g_theInput->GetButtonState( KEY_F8 ).WasJustPressed() )
	{
		delete g_theGame;
		g_theGame = nullptr;
		g_theGame = new Game();
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------
