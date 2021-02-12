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

private:


private:
	//mutable Camera				m_gameCamera;
	
};