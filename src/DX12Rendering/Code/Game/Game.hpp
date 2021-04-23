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
	void InitializeModelMeshesAndTransforms();
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
	void UpdateRenderingOrder();
public:
	Rgba8											m_clearScreenColor				= Rgba8( 0 , 50 , 100 , 255 );
	Rgba8											m_clearScreenColorRT			= Rgba8( 154 , 205 , 102 , 255 );
	float											m_colorLerpTimer				= 0.f;
	float											m_framTime						= 0.f;

	mutable Camera									m_gameCamera;
	
	std::vector<Vertex_PCU>							m_modelMeshVerts[ NUM_MODELS ];
	Transform										m_modelTransforms[ NUM_MODELS ];
	Rgba8											m_modelTintColors[ NUM_MODELS ];
	std::vector< TransparencyRenderOrder >			m_renderingOrder;

	std::vector<Vertex_PCU>							m_triangle;
	std::vector<Vertex_PCU>							m_plane;
	Vec3											m_cameraPosition				= Vec3::ZERO;
	float											m_pitch							= 0.f;
	float											m_yaw							= 0.f;

	float											m_nearZ							= GAME_CAM_NEAR_Z;
	float											m_farZ							= GAME_CAM_FAR_Z;


private:
	//mutable Camera				m_gameCamera;
	
};