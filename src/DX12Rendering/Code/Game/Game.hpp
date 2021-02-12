#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Transform.hpp"
#include "Game/GameCommon.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

class Game
{

public:
			Game();
			void InitializeCameras();

			~Game();

			void Update( float deltaSeconds );
			void Render() const;

public:
	Rgba8 m_clearScreenColor	= RED;
	float m_colorLerpTimer		= 0.f;

private:
	//mutable Camera				m_gameCamera;
	
};