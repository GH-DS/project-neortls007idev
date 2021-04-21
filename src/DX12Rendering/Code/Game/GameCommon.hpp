#pragma once
#include "Engine/Input/InputSystem.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

class Game;
class Window;

//--------------------------------------------------------------------------------------------------------------------------------------------

extern InputSystem* g_theInput;
extern Game*		g_theGame;
extern Window*		g_theWindow;

//--------------------------------------------------------------------------------------------------------------------------------------------

constexpr float CLIENT_ASPECT											= 16.f/9.f;

constexpr float WORLD_SIZE_X											= 1920.f;
constexpr float WORLD_SIZE_Y											= 1080.f;

constexpr float	GAME_CAM_NEAR_Z											= 0.01f;
constexpr float	GAME_CAM_FAR_Z											= 100.0f;
constexpr float	GAME_CAM_FOV											= 60.0f;

//--------------------------------------------------------------------------------------------------------------------------------------------