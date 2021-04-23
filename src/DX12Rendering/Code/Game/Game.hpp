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
	void InitializePlane();

	~Game();

	void Update( float deltaSeconds );
	void Render();
	void UpdateFrameTime( float deltaSeconds ) const;
	void UpdateCameraConstantBufferData() const;
	void UpdateCameraConstantBufferDataRTX() const;
	void UpdateModelMatrix( Mat44 modelMatrix , Rgba8 tint = WHITE ) const;
	void UpdateFromKeyboard( float deltaSeconds );
	void CameraPositionUpdateOnInput( float deltaSeconds );
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
	Transform							m_modelTestTransform;

	std::vector<Vertex_PCU>				m_triangle;
	std::vector<Vertex_PCU>				m_plane;
	Vec3								m_cameraPosition = Vec3::ZERO;
	float								m_pitch = 0.f;
	float								m_yaw = 0.f;

	float								m_nearZ = GAME_CAM_NEAR_Z;
	float								m_farZ	= GAME_CAM_FAR_Z;


private:
	//mutable Camera				m_gameCamera;
	
};