#pragma once
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/DebugUI/ImGUISystem.hpp"
#include "Engine/Core/DebugRender.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

extern RenderContext*				g_theRenderer;
extern InputSystem*					g_theInput;
extern DevConsole*					g_theDevConsole;
extern ImGUISystem*					g_debugUI;
extern BitmapFont*					g_bitmapFont;
extern DebugRenderObjectsManager*	g_currentManager;

//--------------------------------------------------------------------------------------------------------------------------------------------

enum ENGINE_SUBSYSTEMS
{
	ES_INVALID = 0 ,
	ES_RENDER_CONTEXT ,
	ES_INPUT_SYSTEM ,
	ES_DEVCONSOLE ,
	ES_IMGUI_SYSTEM ,
	ES_BITMAPFONT ,
	ES_DEBUGRENDEROBJECTSMANAGER ,
	ES_2D_PARTICLE_SYSTEM ,
	ES_3D_PARTICLE_SYSTEM ,
	ES_AUDIO_SYSTEM ,
	ES_2D_PHYSICS_SYSTEM ,
};

//--------------------------------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
//					NOTES
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
//	
//	64 bit's for 64 Engine Systems
//  Handles initialization and Desctruction
//--------------------------------------------------------------------------------

class EngineSystemManager
{
public:
	EngineSystemManager( ENGINE_SUBSYSTEMS bitwiseOR_EngineSubsystems );
	~EngineSystemManager();

	void Startup();
	void BeginFrame();
	
	void EndFrame();
	void Shutdown();

private:
	long long unsigned int m_initializedSubsystem = ES_INVALID;
};

//--------------------------------------------------------------------------------------------------------------------------------------------