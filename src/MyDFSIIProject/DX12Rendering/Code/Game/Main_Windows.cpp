#include <cassert>
#include <crtdbg.h>
#include "Engine/Core/EngineCommon.hpp"
#include "GameCommon.hpp"
#include "Game/TheApp.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Platform/Window.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//--------------------------------------------------------------------------------------------------------------------------------------------

extern TheApp*				g_theApp;
extern InputSystem*			g_theInput;

		Window*				g_theWindow = nullptr;

//-----------------------------------------------------------------------------------------------

const	char*	APP_NAME		= "FPS";

//-----------------------------------------------------------------------------------------------
// Processes all Windows messages (WM_xxx) for this app that have queued up since last frame.
// For each message in the queue, our WindowsMessageHandlingProcedure (or "WinProc") function
//	is called, telling us what happened (key up/down, minimized/restored, gained/lost focus, etc.)
//-----------------------------------------------------------------------------------------------

int WINAPI WinMain( _In_ HINSTANCE applicationInstanceHandle, _In_opt_ HINSTANCE, _In_ LPSTR commandLineString, _In_ int )
{
	UNUSED( applicationInstanceHandle );
	UNUSED( commandLineString );
	
	g_theApp = new TheApp();

	g_theWindow = new Window();
	g_theWindow->Open( APP_NAME , CLIENT_ASPECT , 0.9f );

	g_theApp->Startup();
		
	while( !g_theWindow->IsQuitting() )			
	{
		g_theApp->RunFrame();					
	}

	g_theApp->Shutdown();
	delete g_theApp;							
	g_theApp = nullptr;

	g_theWindow->Close();
	delete g_theWindow;
	g_theWindow = nullptr;

	return 0;
}

//--------------------------------------------------------------------------------------------------------------------------------------------