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

constexpr float WORLD_SIZE_X											= 20.f;
constexpr float WORLD_SIZE_Y											= 12.f;
constexpr float WORLD_CAMERA_SIZE_X										= /*16.f;*/ WORLD_SIZE_Y*CLIENT_ASPECT;
constexpr float WORLD_CAMERA_SIZE_Y										= /*9.f;*/WORLD_SIZE_Y;
constexpr float UI_SIZE_X												= 1600.f;
constexpr float UI_SIZE_Y												= 800.f;
constexpr float WORLD_CENTER_X											= WORLD_SIZE_X / 2.f;
constexpr float WORLD_CENTER_Y											= WORLD_SIZE_Y / 2.f;

constexpr float MIN_CAMERA_SHAKE										= -10.0f;
constexpr float MAX_CAMERA_SHAKE										= 10.0f;
constexpr float SCREEN_SHAKE_ABLATION_PER_SECOND						= 0.05f;

constexpr float PERCENTAGE_OF_OBSTACLES_IN_MAP							= 0.3f;

//--------------------------------------------------------------------------------------------------------------------------------------------