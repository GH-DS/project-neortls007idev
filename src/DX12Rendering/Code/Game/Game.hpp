#pragma once
#include "Engine/Math/Transform.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Camera.hpp"
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
			void UpdateFrameTime( float deltaSeconds ) const;
			void UpdateCameraConstantBufferData() const;
			void UpdateModelMatrix( Mat44 modelMatrix , Rgba8 tint = WHITE ) const;
public:
	Rgba8 m_clearScreenColor	= RED;
	float m_colorLerpTimer		= 0.f;
	float m_framTime			= 0.f;

	mutable Camera				m_gameCamera;
	std::vector<Vertex_PCU>		m_cubeMeshVerts;
	std::vector<uint>			m_cubeMeshIndices;

private:
	//mutable Camera				m_gameCamera;
	
};