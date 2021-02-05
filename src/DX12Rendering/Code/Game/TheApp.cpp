#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/DebugUI/ImGUISystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Input/VirtualKeyboard.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Renderer/D3D11Common.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Time/Clock.hpp"
#include "Engine/Time/Time.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/TheApp.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

extern RenderContext*						g_theRenderer;	
extern InputSystem*							g_theInput;		
extern DevConsole*							g_theDevConsole;
extern ImGUISystem*							g_debugUI;		
extern BitmapFont*							g_bitmapFont;
extern DebugRenderObjectsManager*			g_currentManager;
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

	delete g_debugUI;
	g_debugUI = nullptr;
	
	//delete g_theAudioSystem;
	//g_theAudioSystem = nullptr;

	//delete g_thePhysicsSystem;
	//g_thePhysicsSystem = nullptr;

	delete g_theDevConsole;
	g_theDevConsole = nullptr;

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
		g_theRenderer = new RenderContext();
	}
	g_theRenderer->Startup( g_theWindow );

	if ( g_bitmapFont == nullptr )
	{
		g_bitmapFont = g_theRenderer->GetOrCreateBitmapFontFromFile( "Data/Fonts/SquirrelFixedFont" ); // TO DO PASS IN THE FONT ADDRESS AND THE TEXTURE POINTER TO IT.
	}

	if ( g_theDevConsole == nullptr )
	{
		g_theDevConsole = new DevConsole();
	}
	g_theDevConsole->Startup();
	AddDebugRenderDevConsoleCommands( g_theDevConsole );
	
	if ( g_currentManager == nullptr )
	{
		// instantiating a default DRO_Manager
		g_currentManager = new DebugRenderObjectsManager();
	}
	g_currentManager->Startup();
// 	if ( g_thePhysicsSystem == nullptr )
// 	{
// 		g_thePhysicsSystem = new Physics2D();
// 	}
// 	g_thePhysicsSystem->Startup();
// 
// 	if ( g_theAudioSystem == nullptr )
// 	{
// 		g_theAudioSystem = new AudioSystem();
// 	}
// 	g_theAudioSystem->Startup();

	if ( g_debugUI == nullptr )
	{
		g_debugUI = new ImGUISystem( g_theWindow , g_theRenderer );
	}
	g_debugUI->Startup();
	
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

	BeginFrame();                        // all engine system and not game systems
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
	g_theDevConsole->BeginFrame();
	g_currentManager->BeginFrame();
	g_debugUI->BeginFrame();
	
	if ( m_taskbarProgress < 100.f  && m_taskbarProgressMode == WND_PROGRESS_VALUE )
	{
		m_taskbarProgress += 0.166f;
		g_theWindow->SetProgress( WND_PROGRESS_VALUE , m_taskbarProgress );
	}
	else if ( m_taskbarProgress >= 100.f && m_taskbarProgressMode == WND_PROGRESS_VALUE )
	{
		m_taskbarProgress = 0.f;
		m_taskbarProgressMode = WND_PROGRESS_NONE;
	}
	else
	{
		g_theWindow->SetProgress( m_taskbarProgressMode , m_taskbarProgress );
		//m_taskbarProgress = 0.f;
	}

}

//--------------------------------------------------------------------------------------------------------------------------------------------

void TheApp::Update( float deltaSeconds )
{
	//g_theInput->Update( deltaSeconds );
	g_theRenderer->UpdateFrameTime( deltaSeconds );
	g_currentManager->Update( deltaSeconds );
	UpdateFromKeyboard();

	if ( m_isPaused )							{ deltaSeconds = 0; }
	else if ( m_isSloMo == true )				{ deltaSeconds /= 10.f; }
	if ( m_isSpeedMo )							{ deltaSeconds = deltaSeconds * 4.0f; }

	g_theGame->Update( deltaSeconds );

	g_theInput->EndFrame();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void TheApp::Render() const
{
 		g_theGame->Render();

		//g_theRenderer->BeginCamera( g_theDevConsole->GetDevConsoleCamera() )
		if ( g_theDevConsole->IsOpen() )
		{
			g_theDevConsole->Render( *g_theRenderer , *g_theDevConsole->GetDevConsoleCamera() , 14.f );
		}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void TheApp::EndFrame()
{
	// all engine things that must end at the end of the frame and not the game
	g_debugUI->EndFrame();
	g_currentManager->EndFrame();
	g_theDevConsole->EndFrame();
	g_theRenderer->EndFrame();
	g_theInput->EndFrame();

// 	if ( g_theRenderer->HasAnyShaderChangedAtPath( L"\\Data\\Shaders\\" , 3.f ) )
// 	{
// 		g_theRenderer->ReCompileAllShaders();
// 	}

	Clock::EndFrame();
}


//--------------------------------------------------------------------------------------------------------------------------------------------

void TheApp::Shutdown()
{
	g_debugUI->Shutdown();
	//g_theAudioSystem->Shutdown();
	//g_thePhysicsSystem->Shutdown();
	g_currentManager->Shutdown();
	g_theDevConsole->Shutdown();
	g_theRenderer->Shutdown();
	g_theInput->Shutdown();
	// TODO :- write me g_theWindow->Shutdown();
	g_theEventSystem->Shutdown();
	Clock::Shutdown();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void TheApp::UpdateFromKeyboard()
{
	if ( g_theInput->GetButtonState( KEY_ESC ).WasJustPressed() ) { g_theWindow->HandleQuitRequested(); }

	if ( g_theInput->WasKeyJustPressed( KEY_TILDE ) )
	{
		g_theDevConsole->ToggleVisibility();
	}

	if ( g_theDevConsole->IsOpen() )
	{
		return;
	}

	if ( g_theInput != nullptr && g_theInput->WasKeyJustPressed( 'C' ) /*&& g_theInput->WasKeyJustPressed( 'T' )*/ )
	{
		g_theWindow->SetTitle( "Function Works" );
	}

	if ( g_theInput != nullptr && g_theInput->WasKeyJustPressed( 'I' ) )
	{
		g_theWindow->SetNewIcon( eIcon::INFORMATION );
	}

	if ( g_theInput->IsKeyHeldDown( KEY_SHIFT ) && g_theInput->WasKeyJustPressed( 'B' ) )
	{
		g_theWindow->DisplaySettings( BORDERLESS );
	}

	if ( g_theInput->IsKeyHeldDown( KEY_SHIFT ) && g_theInput->WasKeyJustPressed( 'R' ) )
	{
		g_theWindow->DisplaySettings( REGULAR );
	}

	if ( g_theInput->IsKeyHeldDown( KEY_SHIFT ) && g_theInput->WasKeyJustPressed( 'F' ) )
	{
		g_theWindow->DisplaySettings( FULLSCREEN );
	}

	if ( g_theInput->IsKeyHeldDown( KEY_SHIFT ) && g_theInput->WasKeyJustPressed( 'T' ) )
	{
		m_taskbarProgress = 0.f;
	}

	if ( g_theInput->IsKeyHeldDown( KEY_SHIFT ) && g_theInput->WasKeyJustPressed( 'P' ) )
	{
		m_taskbarProgressMode = WND_PROGRESS_PAUSED;
	}

	if ( g_theInput->IsKeyHeldDown( KEY_SHIFT ) && g_theInput->WasKeyJustPressed( 'I' ) )
	{
		m_taskbarProgressMode = WND_PROGRESS_INDETERMINATE;
	}

	if ( g_theInput->IsKeyHeldDown( KEY_SHIFT ) && g_theInput->WasKeyJustPressed( 'R' ) )
	{
		m_taskbarProgressMode = WND_PROGRESS_VALUE;
	}

	if ( g_theInput->IsKeyHeldDown( KEY_SHIFT ) && g_theInput->WasKeyJustPressed( 'E' ) )
	{
		m_taskbarProgressMode = WND_PROGRESS_ERROR;
	}

	if ( g_theInput->IsKeyHeldDown( KEY_SHIFT ) && g_theInput->WasKeyJustPressed( 'O' ) )
	{
		m_taskbarProgress = 0.f;
		m_taskbarProgressMode = WND_PROGRESS_NONE;
	}

	if ( g_theInput->GetButtonState( 'P' ).WasJustPressed() )
	{
		m_isPaused = !m_isPaused;
	}

	if ( g_theInput->GetButtonState( KEY_F8 ).WasJustPressed() )
	{
		delete g_theGame;
		g_theGame = nullptr;
		g_theGame = new Game();
	}

	//if ( g_theInput->WasKeyJustPressed( 'R' ) )
	//{
	//	g_theRenderer->ReCompileAllShaders();
	//}
}

