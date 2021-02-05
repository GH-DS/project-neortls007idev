#pragma once
#include "Engine/Platform/Window.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

class TheApp
{

public:
	TheApp();
	~TheApp();
	void Startup();
	void Shutdown();
	void RunFrame();

	void BeginFrame();
	void Update( float deltaSeconds );
	void UpdateFromKeyboard();
	void Render() const;

	void EndFrame();
		
private :
	bool				m_isSloMo				= false;
	bool				m_isSpeedMo				= false;
	float				m_taskbarProgress		= 0.f;
	eWindowProgressMode m_taskbarProgressMode	= WND_PROGRESS_VALUE;

public:
	bool				m_isPaused				= false;
	bool				m_debugCamera			= false;
};

//--------------------------------------------------------------------------------------------------------------------------------------------