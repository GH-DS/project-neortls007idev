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
	void Render();
	void UpdateFrameTime( float deltaSeconds ) const;
	void UpdateCameraConstantBufferData() const;
	void UpdateModelMatrix( Mat44 modelMatrix , Rgba8 tint = WHITE ) const;
	void UpdateFromKeyboard();
public:
	Rgba8 m_clearScreenColor	= Rgba8( 0 , 50 , 100 , 255 );
	Rgba8 m_clearScreenColorRT	= Rgba8( 154 , 205 , 102 , 255 );
	float m_colorLerpTimer		= 0.f;
	float m_framTime			= 0.f;

	mutable Camera						m_gameCamera;
	Transform							m_cubeTestTransform;
	std::vector<Vertex_PCU>				m_cubeMeshVerts;
	std::vector<uint>					m_cubeMeshIndices;
	
	std::vector<Vertex_PCU>				m_modelMeshVerts;
	std::vector<uint>					m_modelMeshIndices;
	Transform							m_modelTestTransform;

	std::vector<Vertex_PCU>				m_triangle;

private:
	//mutable Camera				m_gameCamera;
	
};