#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/VirtualKeyboard.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Primitives/GPUMesh.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Renderer/SwapChain.hpp"
#include "Engine/Time/Time.hpp"

#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/TheApp.hpp"
#include "Engine/Core/VertexMaster.hpp"
#include "Engine/Core/OBJUtils.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/ShaderState.hpp"
#include "Engine/Core/NamedProperties.hpp"
#include "Engine/DebugUI/ImGUISystem.hpp"
#include "Engine/Renderer/Sampler.hpp"
#include "ThirdParty/ImGUI/imgui.h"

//--------------------------------------------------------------------------------------------------------------------------------------------

extern RenderContext*	g_theRenderer;
extern TheApp*			g_theApp;
extern DevConsole*		g_theDevConsole;
extern ImGUISystem*		g_debugUI;

static  bool			s_areDevconsoleCommandsLoaded = false;

//--------------------------------------------------------------------------------------------------------------------------------------------

STATIC shaderLightDataT Game::m_lights;
STATIC Rgba8			Game::m_ambientLightColor;
STATIC LightType		Game::m_lightType[ TOTAL_LIGHTS ];
STATIC fresnelData_t	Game::m_fresnelShaderData;
STATIC dissolveData_t	Game::m_dissolveShaderData;
STATIC fogDataT			Game::m_fogData;
STATIC Texture*			Game::m_dissolveShaderPatternTexture;

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::ImGUITest()
{
	ImGui::NewFrame();
	if( ImGui::Begin( "UI Controls" ) )
	{
		ImGui::Text( "Effects" );
		if( ImGui::CollapsingHeader( "Effects" ) )
		{
			ImGui::BeginChild( "Effects" );
			ImGui::Checkbox( "Tone Map" , &m_isToneMapShaderActive );
			ImGui::Checkbox( "Bloom Blur" , &m_isblurShaderActive );
			ImGui::EndChild();	
		}

		ImGui::RadioButton( "one" , 0 ) ; ImGui::SameLine();
		ImGui::RadioButton( "two" , 1 ); ImGui::SameLine();
		ImGui::RadioButton( "oneb" , 0 );
		
		ImGui::SliderFloat( "float" , &uiTestSlider , 0.0f , 10000.0f );
		ImGui::ColorEdit3( "Clear Color" , ( float* ) &uiTestColor );
	}
	ImGui::End();	
}

//--------------------------------------------------------------------------------------------------------------------------------------------

class Rahul
{
public:
	bool SomeMethod(EventArgs& args)
	{
		UNUSED( args );
		g_theDevConsole->PrintString( WHITE , "Rahul object is called" , DEVCONSOLE_SYTEMLOG );
		return false;
	}
};

//--------------------------------------------------------------------------------------------------------------------------------------------

Game::Game()
{
	if ( !s_areDevconsoleCommandsLoaded )
	{
		AddLightDevConsoleCommands( g_theDevConsole );
		AddShaderDevConsoleCommands( g_theDevConsole );
		AddFogCommandsToDevConsole( g_theDevConsole );
		s_areDevconsoleCommandsLoaded = true;
	}
	
	LoadShaders();
	LoadTextures();
	
	m_tileDiffuse	= g_theRenderer->GetOrCreateTextureFromFile( "Data/Images/tile_diffuse.png" );
	m_tileDiffuse	= g_theRenderer->GetOrCreateTextureFromFile( "Data/Images/Test_StbiFlippedAndOpenGL.png" );
	m_tileNormal	= g_theRenderer->GetOrCreateTextureFromFile( "Data/Images/tile_normal.png" );
	//m_tileDiffuse	= g_theRenderer->GetOrCreateTextureFromFile( "Data/Images/couch/diffuse.png" );
	//m_tileNormal	= g_theRenderer->GetOrCreateTextureFromFile( "Data/Images/couch/normal.png" );
	   	
	InitializeCameras();
	IntializeGameObjects();
		
	m_cubeMeshTransform.SetPosition( 5.f , 0.0f , -10.0f );
	m_sphereMeshTransform.SetPosition( -5.f , 0.0f , -10.0f );
	m_quadTransform.SetPosition( 0.f , 0.0f , -10.0f );
	m_objSciFiShipTransform.SetPosition( 0.f , 10.0f , -10.0f );

	InitializeLightData();
	InitializeShaderMaterialData();

	NamedProperties temp;
	temp.SetValue( "num" , 3.67f );

	float n = temp.GetValue( "num" , 0.0f );

	Rgba8 x = temp.GetValue( "color" , Rgba8( 255 , 255 , 255 , 255 ) );

	UNUSED( n );
	UNUSED( x );
	
 	m_testMaterial = new Material();

 	m_testMaterial->CreateFromFile( "Data/Materials/testMaterial.xml" );
	m_testMaterial->SetData( m_dissolveShaderData );

	Rahul r = Rahul();
	g_theEventSystem->SubscribeToMethod( "me" , &r , &Rahul::SomeMethod );
	g_theEventSystem->FireEvent( "me" , g_gameConfigBlackboard );

	g_theInput->PushCursorSettings( CursorSettings( ABSOLUTE_MODE , MOUSE_IS_WINDOWLOCKED , true ) );

	m_unitCubeMesh = new GPUMesh( g_theRenderer );
	
	//m_cubeMapex = g_theRenderer->CreateTextureCubeFromFile( "Data/Images/CubeMaps/test_sky.png" );
	m_cubeMapex = g_theRenderer->GetOrCreateTextureCubeFromFile( "Data/Images/CubeMaps/galaxy2.png" );
	m_cubeMapTest = g_theRenderer->GetOrCreateShader( "Data/Shaders/CubeMap.hlsl" );

	//std::vector<VertexMaster>	cubeMeshVerts;
	std::vector<Vertex_PCU>		cubeMeshVerts;
	std::vector<uint>			cubeMeshIndices;

	AABB3 box( Vec3( -0.5f , -0.5f , -0.5f ) , Vec3( 0.5f , 0.5f , 0.5f ) );
	CreateCuboid( cubeMeshVerts , cubeMeshIndices , box , WHITE );
	//VertexMaster::ConvertVertexMasterToVertexPCU( cubeMeshLitVerts , cubeMeshVerts );

	m_unitCubeMesh->UpdateVertices( ( uint ) cubeMeshVerts.size() , cubeMeshVerts.data() );
	m_unitCubeMesh->UpdateIndices( cubeMeshIndices );

	m_cubeSampler = new Sampler( g_theRenderer , SAMPLER_CUBEMAP );
	m_linear = new Sampler( g_theRenderer , SAMPLER_BILINEAR );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::InitializeLightData()
{
	m_lights.ambientLight								= Vec4( 1.f , 1.f , 1.f , 0.f );
	m_ambientLightColor.SetColorFromNormalizedFloat( m_lights.ambientLight );
	m_lights.lights[ 0 ].color							= Vec3( 1.f , 1.f , 1.f );
	m_lights.lights[ 0 ].intensity						= 1.0f;
	m_lights.lights[ 0 ].worldPosition					= Vec3( 0.f , 0.f , -5.f );
	m_lights.lights[ 0 ].attenuation					= Vec3::UNIT_VECTOR_ALONG_J_BASIS;
	m_lights.lights[ 0 ].specularAttenuation			= Vec3::UNIT_VECTOR_ALONG_K_BASIS;

	RandomNumberGenerator rng;
	
	for ( uint index = 1 ; index < TOTAL_LIGHTS ; index++ )
	{
		Rgba8 color;
		color.RollRandomColor( rng );
		rng.manuallyIncrementPosition();
		m_lights.lights[ index ].color					= color.GetAsNormalizedFloat3();
		m_lights.lights[ index ].intensity				= 0.f;
		m_lights.lights[ index ].attenuation			= Vec3::UNIT_VECTOR_ALONG_K_BASIS;
		m_lights.lights[ index ].specularAttenuation	= Vec3::UNIT_VECTOR_ALONG_K_BASIS;
		m_lights.lights[ index ].worldPosition			= Vec3( index * 1.5f , 0.f , 10.f );
	}

	for ( uint index = 0 ; index < TOTAL_LIGHTS ; index++ )
	{
		m_lightType[ index ] = POINT_LIGHT;
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::InitializeShaderMaterialData()
{
	m_fresnelShaderData.fresnelColor			= Vec3::UNIT_VECTOR_ALONG_J_BASIS;
	m_fresnelShaderData.fresnelfactor			= 1.f;
	m_fresnelShaderData.fresnelPower			= 1.f;

	m_dissolveShaderData.startColor				= CYAN.GetAsNormalizedFloat3();
	m_dissolveShaderData.burnEdgeWidth			= 1.f;
	m_dissolveShaderData.endColor				= ORANGE.GetAsNormalizedFloat3();
	m_dissolveShaderData.burnValue				= 0.f;

	m_fogData.nearFog							= 0.f;
	m_fogData.farFog							= 15.f;
	
	m_fogData.nearFogColor						= GRAY.GetAsNormalizedFloat3();
	m_fogData.farFogColor						= /*WHITE.GetAsNormalizedFloat3();*/ Rgba8( 37 , 70 , 87 , 127 ).GetAsNormalizedFloat3();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::LoadShaders()
{
	m_lightShaders[ LitShaderTypes::LIT ]						= g_theRenderer->GetOrCreateShader( "Data/Shaders/litDefault.hlsl" );
	m_lightShaders[ LitShaderTypes::UNLIT ]						= g_theRenderer->GetOrCreateShader( "Data/Shaders/default.hlsl" );
	m_lightShaders[ LitShaderTypes::UV ]						= g_theRenderer->GetOrCreateShader( "Data/Shaders/uvDebugger.hlsl" );
	m_lightShaders[ LitShaderTypes::NORMAL ]					= g_theRenderer->GetOrCreateShader( "Data/Shaders/normalLit.hlsl" );
	m_lightShaders[ LitShaderTypes::TANGENT ]					= g_theRenderer->GetOrCreateShader( "Data/Shaders/tangentLit.hlsl" );
	m_lightShaders[ LitShaderTypes::BITANGENT ]					= g_theRenderer->GetOrCreateShader( "Data/Shaders/bitangentLit.hlsl" );
	m_lightShaders[ LitShaderTypes::SURFACE_NORMAL ]			= g_theRenderer->GetOrCreateShader( "Data/Shaders/surfaceNormalLit.hlsl" );
	m_lightShaders[ LitShaderTypes::DISSOLVE ]					= g_theRenderer->GetOrCreateShader( "Data/Shaders/dissolve.hlsl" );
	m_lightShaders[ LitShaderTypes::FRESNEL ]					= g_theRenderer->GetOrCreateShader( "Data/Shaders/fresnel.hlsl" );
	m_lightShaders[ LitShaderTypes::TRIPLANAR_UNLIT ]			= g_theRenderer->GetOrCreateShader( "Data/Shaders/triplanarUnlit.hlsl" );
	m_lightShaders[ LitShaderTypes::TRIPLANAR_LIT ]				= g_theRenderer->GetOrCreateShader( "Data/Shaders/triplanarLit.hlsl" );
	//m_lightShaders[ LitShaderTypes::FOG ]						= g_theRenderer->GetOrCreateShader( "Data/Shaders/fog.hlsl" );

	m_blurShader												= g_theRenderer->GetOrCreateShader( "Data/Shaders/blur.hlsl" );
	m_toneMapShader												= g_theRenderer->GetOrCreateShader( "Data/Shaders/toneMap.hlsl" );

	m_currentShader												= m_lightShaders[ LitShaderTypes::LIT ];
	m_currentShaderIndex										= LitShaderTypes::LIT;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::LoadTextures()
{
	m_dissolveShaderPatternTexture = g_theRenderer->GetOrCreateTextureFromFile( "Data/Images/noise.png" );
	
	m_triplanarShaderTextures[ 0 ] = g_theRenderer->GetOrCreateTextureFromFile( "Data/Images/rockT6/rock_06_diff_1k.png" );
	m_triplanarShaderTextures[ 1 ] = g_theRenderer->GetOrCreateTextureFromFile( "Data/Images/rockT6/rock_06_nor_1k.png" );
	m_triplanarShaderTextures[ 2 ] = g_theRenderer->GetOrCreateTextureFromFile( "Data/Images/grass/aerial_grass_rock_diff_1k.png" );
	m_triplanarShaderTextures[ 3 ] = g_theRenderer->GetOrCreateTextureFromFile( "Data/Images/grass/aerial_grass_rock_nor_1k.png" );
	m_triplanarShaderTextures[ 4 ] = g_theRenderer->GetOrCreateTextureFromFile( "Data/Images/dirt/dirt_aerial_03_diff_1k.png" );
	m_triplanarShaderTextures[ 5 ] = g_theRenderer->GetOrCreateTextureFromFile( "Data/Images/dirt/dirt_aerial_03_nor_1k.png" );

	m_objSciFiShipMeshTex_D		   = g_theRenderer->GetOrCreateTextureFromFile( "Data/Models/scifiFighter/diffuse.jpg" );
	m_objSciFiShipMeshTex_N		   = g_theRenderer->GetOrCreateTextureFromFile( "Data/Models/scifiFighter/normal.png" );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::IntializeGameObjects()
{
	m_cubeMesh = new GPUMesh( g_theRenderer ); 
	std::vector<VertexMaster>	cubeMeshVerts;
	std::vector<VertexLit>		cubeMeshLitVerts;
	std::vector<uint>			cubeMeshIndices;

	AABB3 box( Vec3( -1 , -1 , -1 ) , Vec3( 5 , 1 , 1 ) );
	CreateCuboid( cubeMeshVerts , cubeMeshIndices , box , WHITE );
	VertexMaster::ConvertVertexMasterToVertexLit( cubeMeshLitVerts , cubeMeshVerts );

	m_cubeMesh->UpdateVertices( ( uint ) cubeMeshVerts.size() , cubeMeshVerts.data() );
	m_cubeMesh->UpdateIndices( cubeMeshIndices );

	std::vector<VertexMaster>	sphereMeshVerts;
	std::vector<VertexLit>		sphereMeshLitVerts;
	std::vector<uint>			sphereIndices;

	CreateUVSphere( m_hCuts , m_vCuts , sphereMeshVerts , sphereIndices , 1.f );

	// this is incorrect it seems
	VertexMaster::ConvertVertexMasterToVertexLit( sphereMeshLitVerts , sphereMeshVerts );

	m_meshSphere = new GPUMesh( g_theRenderer );
	//m_meshSphere->UpdateVertices( ( uint ) sphereMeshLitVerts.size() , sphereMeshLitVerts.data() );
	m_meshSphere->UpdateVertices( ( uint ) sphereMeshVerts.size() , sphereMeshVerts.data() );
	m_meshSphere->UpdateIndices( sphereIndices );

	std::vector<VertexMaster>	quadMeshVerts;
	std::vector<VertexLit>		quadMeshLitVerts;
	std::vector<uint>			quadIndices;

	//CreateQuad( quadMeshVerts , quadIndices , AABB2( -5.f , -5.f , 5.f , 5.f ) );
	CreateQuad( quadMeshVerts , quadIndices , AABB2::ZERO_TO_ONE );
	m_quadMesh = new GPUMesh( g_theRenderer );
//	m_quadMesh->UpdateVertices( ( uint ) sphereMeshLitVerts.size() , sphereMeshLitVerts.data() );
	m_quadMesh->UpdateVertices( ( uint ) quadMeshVerts.size() , quadMeshVerts.data() );
	m_quadMesh->UpdateIndices( quadIndices );
	
	MeshBuilderOptions objMeshOptions1;
	objMeshOptions1.generateTangents = true;
	objMeshOptions1.generateNormals = true;
	//objMeshOptions1.clean = true;
	m_objSciFiShipMesh = new GPUMesh( g_theRenderer );
	m_objSciFiShipMesh = LoadObjFileIntoGpuMesh( objMeshOptions1 , "Data/Models/scifiFighter/mesh.obj" );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

Game::~Game()
{
	delete m_testMaterial;
	m_testMaterial		= nullptr;
	 
	delete m_cubeMesh;
	m_cubeMesh			= nullptr;

	delete m_meshSphere;
	m_meshSphere		= nullptr;

	delete m_quadMesh;
	m_quadMesh			= nullptr;

	delete m_objSciFiShipMesh;
	m_objSciFiShipMesh			= nullptr;

	m_meshTex_D			= nullptr;
	m_meshTex_N			= nullptr;
	m_tileDiffuse		= nullptr;
	m_tileNormal		= nullptr;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::InitializeCameras()
{
		m_uiCamera.SetOrthoView( Vec2( 0.f , 0.f ) , Vec2( UI_SIZE_X , UI_SIZE_Y ) );
		m_gameCamera.SetProjectionPerspective( 60.f , CLIENT_ASPECT , -.1f , -100.f );
		m_gameCamera.SetPosition( Vec3( 0.f , 0.f , 0.f ) );
		//m_gameCamera.SetClearMode( CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT , Rgba8( 37 , 70 , 87 , 127 ) , 1.f , 0 );
		m_gameCamera.SetClearMode( CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT , BLACK , 1.f , 0 );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::Update( float deltaSeconds )
{
	m_ambientLightColor.SetColorFromNormalizedFloat( m_lights.ambientLight );
	
	DebugDrawUI( deltaSeconds );
	
	if ( m_isHUDEnabled )
	{
		DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.625f , 1.00f ) , Vec2::ONE , 20.f , RED , deltaSeconds ,
			"[ H ] : HIDE HELP HUD" );
	}
	else
	{
		DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.625f , 1.00f ) , Vec2::ONE , 20.f , RED , deltaSeconds ,
			"[ H ] : SHOW HELP HUD" );
	}

	UpdateLightPosition( deltaSeconds );

	static float y = 0;
	y += deltaSeconds;
	//m_cubeMeshTransform.SetRotation( -15.f * ( float ) GetCurrentTimeSeconds()/* 0.f*/ ,  -20.f * ( float ) GetCurrentTimeSeconds() , 0.f );
	//m_sphereMeshTransform.SetRotation( 20.f * ( float ) GetCurrentTimeSeconds() /*0.f*/,  50.f * ( float ) GetCurrentTimeSeconds() , 0.f );
	UpdateFromKeyBoard( deltaSeconds );

	if ( m_testMaterial->m_uboIsDirty )
	{
		m_testMaterial->SetData( m_dissolveShaderData );
		m_testMaterial->m_uboIsDirty = false;
	}
	ImGUITest();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateLightPosition( float deltaSeconds )
{

	for ( uint index = 0; index < TOTAL_LIGHTS; index++ )
	{
		Rgba8 lightColor;
		lightColor.SetColorFromNormalizedFloat( Vec4( m_lights.lights[ index ].color , m_lights.lights[ index ].intensity ) );

		if ( ( index == m_currentLightIndex ) & m_isLightFollowingTheCamera )
		{
			m_lights.lights[ m_currentLightIndex ].worldPosition = m_gameCamera.GetPosition();
			Vec3 direction = m_gameCamera.GetCameraTransform().GetAsMatrix().GetKBasis3D();
			m_lights.lights[ m_currentLightIndex ].direction = -direction;
			continue;
		}
		if ( POINT_LIGHT == m_lightType[ index ] )
		{
			DebugAddWorldPoint( m_lights.lights[ index ].worldPosition , 0.125f , lightColor , deltaSeconds * 0.5f , DEBUG_RENDER_USE_DEPTH );
		}
		if ( DIRECTIONAL_LIGHT == m_lightType[ index ] )
		{
			DebugAddWorldArrow( m_lights.lights[ index ].worldPosition , m_lights.lights[ index ].worldPosition + m_lights.lights[ index ].direction , lightColor , deltaSeconds * 0.5f );
		}
		if ( SPOT_LIGHT == m_lightType[ index ] )
		{
			DebugAddWorldArrow( m_lights.lights[ index ].worldPosition , m_lights.lights[ index ].worldPosition + m_lights.lights[ index ].direction , lightColor , deltaSeconds * 0.5f );
		}
	}


	if ( m_isLightAnimated )
	{
		m_lights.lights[ m_currentLightIndex ].worldPosition = Vec3( 0.f , 0.f , -5.f ) + Vec3::MakeFromSpericalCoordinates(
			45.f * ( float ) GetCurrentTimeSeconds() , 30.f * SinDegrees( ( float ) GetCurrentTimeSeconds() ) , 5.f );
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::DebugDrawUI( float deltaSeconds )
{
	std::string cureentShaderName;

	switch ( m_currentShaderIndex )
	{
		case LitShaderTypes::LIT:
			cureentShaderName = "LIT SHADER";
			break;

		case LitShaderTypes::UNLIT:
		cureentShaderName = "UNLIT SHADER";
		break;

		case LitShaderTypes::UV:
			cureentShaderName = "UV SHADER";
			break;

		case LitShaderTypes::NORMAL:
			cureentShaderName = "VERTEX NORMAL SHADER";
			break;

		case LitShaderTypes::TANGENT:
			cureentShaderName = "VERTEX TANGENT SHADER";
			break;

		case LitShaderTypes::BITANGENT:
			cureentShaderName = "VERTEX BITANGENT SHADER";
			break;

		case LitShaderTypes::SURFACE_NORMAL:
			cureentShaderName = "SURFACE NORMAL SHADER";
			break;
		case LitShaderTypes::DISSOLVE:
			cureentShaderName = "DISSOLVE LIT SHADER";
			break;
		
		case LitShaderTypes::FRESNEL:
			cureentShaderName = "FRESNEL SHADER";
			break;
		
		case LitShaderTypes::TRIPLANAR_LIT:
			cureentShaderName = "TRIPLANAR LIT SHADER";
			break;
		
		case LitShaderTypes::TRIPLANAR_UNLIT:
			cureentShaderName = "TRIPLANAR UNLIT SHADER";
			break;
	}

	float leftVerticalAlignment = ( 1080.f * 0.25f ) / 16.f;
	float normalizedOffset = RangeMapFloat( 0.f , 1080.f , 0.f , 1.f , leftVerticalAlignment );

	DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1.00f ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
		"[ LT-ARRW / RT-ARRW ] : SWITCH SHADERS" );
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 1 * normalizedOffset ) ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
		"CURRENT SHADER = %s" , cureentShaderName.c_str() );
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 2 * normalizedOffset ) ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
		"[ F5 ] = Position Light At Origin" );
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 3 * normalizedOffset ) ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
		"[ F6 ] = Position Light At Current Camera Position" );
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 4 * normalizedOffset ) ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
		"[ F7 ] = Light Follows Camera" );
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 5 * normalizedOffset ) ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
		"[ F9 ] = Light Moves in an fixed Path" );
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 6 * normalizedOffset ) ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
		"[ 9 / 0 ] = Change Ambient Light Intensity (Clamped 0-1)" );
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 7 * normalizedOffset ) ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
		"[ - / + ] = Change Light Intensity (UnClamped)" );
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 8 * normalizedOffset ) ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
		"[ [ / ] ] = Change Specular Factor (Clamped 0-1)" );
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 9 * normalizedOffset ) ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
		"[ U / J ] = Change Specular Power (Clamped 1-INF)" );
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 10 * normalizedOffset ) ) , Vec2::ZERO_ONE, 14.5f , PINK , deltaSeconds ,
		"[ T / R / G / Y ] = Change Current Light Attenuation to ZERO / Constant / Linear / Quadratic" );
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 11 * normalizedOffset ) ) , Vec2::ZERO_ONE, 14.5f , PINK , deltaSeconds ,
		"[ 1 - 8 ] = Change Current Selected Light" );
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 12 * normalizedOffset ) ) , Vec2::ZERO_ONE, 14.5f , PINK , deltaSeconds ,
		"[ M ] = Cycle Current Selected Light between POINT / DIRECTIONAL / SPOT" );
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 13 * normalizedOffset ) ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
		"[ Q / E ] = ENABLE / DISABLE Currently Selected Light" );

	if ( LitShaderTypes::FRESNEL == m_currentShaderIndex )
	{
		DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 14 * normalizedOffset ) ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
			"[ Z / X ] = Fresnel Factor = %f", m_fresnelShaderData.fresnelfactor );

		DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 15 * normalizedOffset ) ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
			"[ C / V ] = Fresnel Power = %f" , m_fresnelShaderData.fresnelPower );

		DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 16 * normalizedOffset ) ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
			"Fresnel Color = ( %u , %u ,%u )" , (uint)(m_fresnelShaderData.fresnelColor.x * 255), ( uint ) (m_fresnelShaderData.fresnelColor.y * 255), ( uint ) ( m_fresnelShaderData.fresnelColor.z * 255 ) );
	}

	if ( LitShaderTypes::DISSOLVE == m_currentShaderIndex )
	{
		DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 14 * normalizedOffset ) ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
			"[ Z / X ] = Dissolve Burn Value = %f" , m_dissolveShaderData.burnValue );

		DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 15 * normalizedOffset ) ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
			"[ C / V ] = Dissolve Burn Edge Width = %f" , m_dissolveShaderData.burnValue );

		DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 16 * normalizedOffset ) ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
			"Dissolve Start Color = ( %u , %u ,%u )" , ( uint ) ( m_dissolveShaderData.startColor.x * 255 ) , ( uint ) ( m_dissolveShaderData.startColor.y * 255 ) , ( uint ) ( m_dissolveShaderData.startColor.z * 255 ) );

		DebugAddScreenTextf( Vec4( 0.f , 0.f , 0.f , 1 - ( 17 * normalizedOffset ) ) , Vec2::ZERO_ONE , 14.5f , PINK , deltaSeconds ,
			"Dissolve End Color = ( %u , %u ,%u )" , ( uint ) ( m_dissolveShaderData.endColor.x * 255 ) , ( uint ) ( m_dissolveShaderData.endColor.y * 255 ) , ( uint ) ( m_dissolveShaderData.endColor.z * 255 ) );
	}

//--------------------------------------------------------------------------------------------------------------------------------------------

	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1.f ) , Vec2::ONE , 14.5f , ORANGE , deltaSeconds ,
		"Global Specular Power = %.2f" , m_lights.SPECULAR_POWER );
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 1 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE , deltaSeconds ,
		"Global Specular Factor = %.2f" , m_lights.SPECULAR_FACTOR );
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 2 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE , deltaSeconds , "Ambient Light Intensity = %.2f" , m_lights.ambientLight.w );

	Rgba8 ambientLightColor;
	ambientLightColor.SetColorFromNormalizedFloat( m_lights.ambientLight );

	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 3 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE , deltaSeconds , "Ambient Light Color = R(%u) G(%u) B(%u) " , ambientLightColor.r , ambientLightColor.g , ambientLightColor.b );

	Rgba8 debuglightColor;
	debuglightColor.SetColorFromNormalizedFloat( Vec4( m_lights.lights[ m_currentLightIndex ].color , 1.f ) );

	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 4 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
		deltaSeconds , "Current Light Number = %u" , m_currentLightIndex );

	if ( POINT_LIGHT == m_lightType[ m_currentLightIndex ] )
	{
		DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 5 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
			deltaSeconds , "Current Light Type = POINT LIGHT");
	}
	else if ( DIRECTIONAL_LIGHT == m_lightType[ m_currentLightIndex ] )
	{
		DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 5 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
			deltaSeconds , "Current Light Type = DIRECTIONAL LIGHT" );
	}
	else if ( SPOT_LIGHT == m_lightType[ m_currentLightIndex ] )
	{
		DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 5 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
			deltaSeconds , "Current Light Type = SPOT LIGHT" );
	}

	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 6 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
		deltaSeconds , "Current Light Intensity = %.2f" , m_lights.lights[ m_currentLightIndex ].intensity );
	
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 7 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE , deltaSeconds ,
		"Diffuse Attenuation = ( %.3f , %.3f , %.3f ) " , m_lights.lights[ m_currentLightIndex ].attenuation.x ,
		m_lights.lights[ m_currentLightIndex ].attenuation.y , m_lights.lights[ m_currentLightIndex ].attenuation.z );
	
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 8 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
		deltaSeconds , "Current Light Color = R(%u) G(%u) B(%u) " , debuglightColor.r , debuglightColor.g ,
		debuglightColor.b );
	
	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 11 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
		deltaSeconds , "Current Light InnerAngle = %f" , m_lights.lights[ m_currentLightIndex ].dotInnerAngle );

	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 12 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
		deltaSeconds , "Current Light OuterAngle = %f" , m_lights.lights[ m_currentLightIndex ].dotOuterAngle );

	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 13 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
		deltaSeconds , "Current Light Specular attenuation = ( %.0f , %.0f , %.0f )" ,
		m_lights.lights[ m_currentLightIndex ].specularAttenuation.x ,
		m_lights.lights[ m_currentLightIndex ].specularAttenuation.y ,
		m_lights.lights[ m_currentLightIndex ].specularAttenuation.z );

	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 14 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
		deltaSeconds , "Current Light direction Factor = %f" , m_lights.lights[ m_currentLightIndex ].directionFactor );

	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 15 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
		deltaSeconds , "Current Light direction = ( %.0f , %.0f , %.0f )" ,
		m_lights.lights[ m_currentLightIndex ].direction.x ,
		m_lights.lights[ m_currentLightIndex ].direction.y ,
		m_lights.lights[ m_currentLightIndex ].direction.z );

	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 17 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
		deltaSeconds , "Fog Near Distance = %f" , m_fogData.nearFog );

	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 18 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
		deltaSeconds , "Fog Far Distance = %f" , m_fogData.farFog );

	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 19 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
		deltaSeconds , "Fog Near Color = ( %u , %u , %u )" ,
		( uint ) ( m_fogData.nearFogColor.x * 255.f ) , ( uint ) ( m_fogData.nearFogColor.y * 255.f ) , ( uint ) ( m_fogData.nearFogColor.z * 255.f ) );

	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 20 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
		deltaSeconds , "Fog Far Color = ( %u , %u , %u )" ,
		( uint ) ( m_fogData.farFogColor.x * 255.f ) , ( uint ) ( m_fogData.farFogColor.y * 255.f ) , ( uint ) ( m_fogData.farFogColor.z * 255.f ) );

	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 22 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
		deltaSeconds , "RTVs Created = %d" , g_theRenderer->GetTotalRenderTargetPoolSize() );

	DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 23 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
		deltaSeconds , "RTVs To be Recycled = %d" , g_theRenderer->GetTexturePoolFreeCount() );

	if ( m_isblurShaderActive )
	{
		DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 25 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
			deltaSeconds , "[F2] Bloom = ACTIVE" );
	}

	if ( !m_isblurShaderActive )
	{
		DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 25 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
			deltaSeconds , "[F2] Bloom = INACTIVE" );
	}

	if ( m_isToneMapShaderActive )
	{
		DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 26 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
							 deltaSeconds , "[F3] Tone Map = ACTIVE" );
		DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 27 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
			deltaSeconds , "[UP/DOWN ARROW] Switch Tone Map" );
		DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 28 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
			deltaSeconds , "[SHIFT + UP/DOWN ARROW] Tone Map Strength = %f" , m_toneMapTransform.Tw );
		DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 29 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
			deltaSeconds , "[B/N] Tone Map Power = %f" , m_tonePower );
	}

	if ( !m_isToneMapShaderActive )
	{
		DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 26 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
			deltaSeconds , "[F3] TONEMAP = INACTIVE" );
		DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 27 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
			deltaSeconds , "[UP/DOWN ARROW] Switch Tone Map" );
		DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 28 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
			deltaSeconds , "[SHIFT + UP/DOWN ARROW] Tone Map Strength = %f" , m_toneMapTransform.Tw );
		DebugAddScreenTextf( Vec4( 0.f , 0.f , 1.f , 1 - ( 29 * normalizedOffset ) ) , Vec2::ONE , 14.5f , ORANGE ,
			deltaSeconds , "[B/N] Tone Map Power = %f" , m_tonePower );
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::Render() const
{
	Texture* backBuffer		= g_theRenderer->m_swapChain->GetBackBuffer();
	Texture* colorTarget	= g_theRenderer->GetOrCreatematchingRenderTarget( backBuffer , "realColorTarget" );
	Texture* bloomTarget	= g_theRenderer->GetOrCreatematchingRenderTarget( backBuffer , "BloomColorTarget" );
	//Texture* normalTarget	= g_theRenderer->GetOrCreatematchingRenderTarget( backBuffer );
	//Texture* tangentTarget	= g_theRenderer->GetOrCreatematchingRenderTarget( backBuffer );
	//Texture* albedoTarget	= g_theRenderer->GetOrCreatematchingRenderTarget( backBuffer );

	m_gameCamera.SetColorTarget( 0 , colorTarget );
	m_gameCamera.SetColorTarget( 1 , bloomTarget );
	//m_gameCamera.SetColorTarget( 2 , normalTarget );
	//m_gameCamera.SetColorTarget( 3 , albedoTarget );
	//m_gameCamera.SetColorTarget( 4 , tangentTarget );
	
	g_theRenderer->BeginCamera( m_gameCamera );
	m_gameCamera.CreateMatchingDepthStencilTarget(); 
	g_theRenderer->BindDepthStencil( m_gameCamera.GetDepthStencilTarget() );
	
	g_theRenderer->BindShader( nullptr );
	g_theRenderer->BindCubeMapTexture( nullptr );
	g_theRenderer->BindSampler( m_linear );
	
	g_theRenderer->SetRasterState( FILL_SOLID );
	
	g_theRenderer->SetCullMode( CULL_BACK );
	g_theRenderer->SetDepthTest( COMPARE_LEQUAL , true );

	g_theRenderer->SetAmbientLight( m_ambientLightColor , m_lights.ambientLight.w );
	//g_theRenderer->EnableLight( 0 , m_lights.lights[ 0 ] );
	//g_theRenderer->EnableLight( 1 , m_lights.lights[ 1 ] );
	//g_theRenderer->EnableLight( 2 , m_lights.lights[ 2 ] );
	//g_theRenderer->EnableLight( 3 , m_lights.lights[ 3 ] );
	g_theRenderer->UpdateLightsData( m_lights );
	g_theRenderer->EnableAllLights();
	g_theRenderer->SetSpecularFactor( m_lights.SPECULAR_FACTOR );
	g_theRenderer->SetSpecularPower( m_lights.SPECULAR_POWER );
	
	g_theRenderer->BindShader( m_currentShader );
	g_theRenderer->BindTexture( m_tileDiffuse );
	g_theRenderer->BindTexture( m_tileNormal , eTextureType::TEX_NORMAL );

	//g_theRenderer->EnableFog( m_fogData.nearFog , m_fogData.farFog , m_fogData.nearFogColor , m_fogData.farFogColor );
	g_theRenderer->EnableFog( m_fogData );

	BindShaderSpecificMaterialData();

 	g_theRenderer->SetModelMatrix( m_sphereMeshTransform.GetAsMatrix() );
 	g_theRenderer->DrawMesh( m_meshSphere );

	g_theRenderer->DisableFog();

 	g_theRenderer->SetModelMatrix( m_quadTransform.GetAsMatrix() );
	g_theRenderer->DrawMesh( m_quadMesh );
	
	g_theRenderer->SetModelMatrix( m_cubeMeshTransform.GetAsMatrix() );
	g_theRenderer->DrawMesh( m_cubeMesh );

	
	if ( m_isFresnelShaderActive )													{	RenderFresnelShader2ndPass();	}
		
	g_theRenderer->SetRasterState( FILL_SOLID );
	
 	//g_theRenderer->BindTexture( m_objSciFiShipMeshTex_D );
	//g_theRenderer->BindTexture( m_objSciFiShipMeshTex_N , eTextureType::TEX_NORMAL );
	//g_theRenderer->SetModelMatrix( m_objSciFiShipTransform.GetAsMatrix() );
 	//g_theRenderer->DrawMesh( m_objSciFiShipMesh );
	g_theRenderer->BindTexture( nullptr );
	g_theRenderer->BindShader( nullptr );
	
	//g_theRenderer->SetModelMatrix( Mat44::IDENTITY );
	
 	//g_theRenderer->BindMaterial( m_testMaterial );
	//m_meshSphere->GetVertexBuffer()->SetVertexBufferLayout( Vertex_PCU::LAYOUT );
  	//g_theRenderer->SetModelMatrix( m_sphereMeshTransform.GetAsMatrix() );
  	//g_theRenderer->DrawMesh( m_meshSphere );
	//m_meshSphere->GetVertexBuffer()->SetVertexBufferLayout( VertexMaster::LAYOUT );
 
 	g_theRenderer->BindMaterial( nullptr );

	g_theRenderer->BindShader( m_cubeMapTest );
	g_theRenderer->BindCubeMapTexture( m_cubeMapex );
	g_theRenderer->SetCullMode( CULL_NONE );
	g_theRenderer->BindSampler( m_cubeSampler );
	//g_theRenderer->SetModelMatrix( m_quadTransform.GetAsMatrix() );
	g_theRenderer->SetModelMatrix( Mat44::IDENTITY );
	g_theRenderer->DrawMesh( m_unitCubeMesh );
	g_theRenderer->SetCullMode( CULL_BACK );
	g_theRenderer->BindCubeMapTexture( nullptr );
	g_theRenderer->BindShader( nullptr );
	
	g_theRenderer->BindTexture( nullptr );
	g_theRenderer->SetDepthTest( COMPARE_ALWAYS , true );
	g_theRenderer->SetCullMode( CULL_NONE );
	g_theRenderer->DisableLight( 0 );
	
	g_theRenderer->EndCamera( m_gameCamera );

	// 1. we're going to setup camera that identity
	// 2. bind a shader for the effect
	// 3. bind source as an input
	// 4. bind other inputs we may need
	// 5. render a full screen image( quad / triangle )
	// 6. end the effect camera.

	//Shader* shader = g_theRenderer->GetOrCreateShader( "Data/Shaders/imageEffect.hlsl" );

	//g_theRenderer->GetOrCreateShader( "Data/Shaders/blur.hlsl" );
	
	if ( m_isblurShaderActive )
	{
		Shader* blurShader = g_theRenderer->GetOrCreateShader( "Data/Shaders/blur.hlsl" );;
		Texture* blurredBloom = g_theRenderer->GetOrCreatematchingRenderTarget( bloomTarget , "BlurBloomTarget" );
		g_theRenderer->StartEffect( blurredBloom , bloomTarget , blurShader );
		g_theRenderer->BindTexture( bloomTarget , TEX_USER_TYPE );
		g_theRenderer->EndEffect();
		
		Shader* combineImageShader = g_theRenderer->GetOrCreateShader( "Data/Shaders/combineImage.hlsl" );;
		Texture* finalImage = g_theRenderer->GetOrCreatematchingRenderTarget( colorTarget );
		g_theRenderer->StartEffect( finalImage , colorTarget , combineImageShader );
		g_theRenderer->BindTexture( blurredBloom , TEX_USER_TYPE );
		g_theRenderer->BindTexture( colorTarget , TEX_USER_TYPE , 1 );
		g_theRenderer->EndEffect();
		g_theRenderer->CopyTexture( backBuffer , finalImage );
		g_theRenderer->ReleaseRenderTarget( blurredBloom );
		g_theRenderer->ReleaseRenderTarget( finalImage );
	}
	else
	{
		Shader* combineImageShader = g_theRenderer->GetOrCreateShader( "Data/Shaders/combineImage.hlsl" );;
		Texture* finalImage = g_theRenderer->GetOrCreatematchingRenderTarget( colorTarget );
		g_theRenderer->StartEffect( finalImage , colorTarget , combineImageShader );
		g_theRenderer->BindTexture( bloomTarget , TEX_USER_TYPE );
		g_theRenderer->BindTexture( colorTarget , TEX_USER_TYPE , 1 );
		g_theRenderer->EndEffect();
		g_theRenderer->CopyTexture( backBuffer , finalImage );
		g_theRenderer->ReleaseRenderTarget( finalImage );
	}
	if ( m_isToneMapShaderActive )
	{
		Texture* toneMapTarget = g_theRenderer->GetOrCreatematchingRenderTarget( colorTarget , "ToneMapTarget" );
		Texture* currentView   = g_theRenderer->GetOrCreatematchingRenderTarget( backBuffer );
		g_theRenderer->CopyTexture( currentView , backBuffer );
		g_theRenderer->StartEffect( toneMapTarget , currentView , m_toneMapShader );
		g_theRenderer->BindMaterialData( ( void* ) &m_toneMapTransform , sizeof( m_toneMapTransform ) );
		g_theRenderer->EndEffect();
		g_theRenderer->CopyTexture( backBuffer , toneMapTarget );
		g_theRenderer->ReleaseRenderTarget( currentView );
		g_theRenderer->ReleaseRenderTarget( toneMapTarget );
	}
	//g_theRenderer->CopyTexture( backBuffer , colorTarget );

	g_theRenderer->ReleaseRenderTarget( bloomTarget );
	g_theRenderer->ReleaseRenderTarget( colorTarget );
	
	m_gameCamera.SetColorTarget( backBuffer );
	
	GUARANTEE_OR_DIE( g_theRenderer->GetTotalRenderTargetPoolSize() < 8 , "LEAKING RENDER TARGETS" );
	
	DebugRenderWorldToCamera( &m_gameCamera );
	DebugRenderScreenTo( nullptr );

	g_debugUI->Render();

}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::BindShaderSpecificMaterialData() const
{
	if ( LitShaderTypes::TRIPLANAR_LIT == m_currentShaderIndex || LitShaderTypes::TRIPLANAR_UNLIT == m_currentShaderIndex )
	{
		g_theRenderer->BindTexture( m_triplanarShaderTextures[ 0 ] , eTextureType::TEX_USER_TYPE , 0 );
		g_theRenderer->BindTexture( m_triplanarShaderTextures[ 1 ] , eTextureType::TEX_USER_TYPE , 1 );
		g_theRenderer->BindTexture( m_triplanarShaderTextures[ 2 ] , eTextureType::TEX_USER_TYPE , 2 );
		g_theRenderer->BindTexture( m_triplanarShaderTextures[ 3 ] , eTextureType::TEX_USER_TYPE , 3 );
		g_theRenderer->BindTexture( m_triplanarShaderTextures[ 4 ] , eTextureType::TEX_USER_TYPE , 4 );
		g_theRenderer->BindTexture( m_triplanarShaderTextures[ 5 ] , eTextureType::TEX_USER_TYPE , 5 );
	}

	if ( LitShaderTypes::DISSOLVE == m_currentShaderIndex )
	{
		g_theRenderer->BindTexture( m_dissolveShaderPatternTexture , eTextureType::TEX_USER_TYPE , 0 );
		g_theRenderer->BindMaterialData( ( void* ) &m_dissolveShaderData , sizeof( m_dissolveShaderData ) );
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::RenderFresnelShader2ndPass() const
{
	g_theRenderer->BindShader( m_lightShaders[ LitShaderTypes::FRESNEL ] );
	g_theRenderer->BindMaterialData( ( void* ) &m_fresnelShaderData , sizeof( m_fresnelShaderData ) );
	g_theRenderer->SetBlendMode( ALPHA );

	g_theRenderer->SetModelMatrix( m_sphereMeshTransform.GetAsMatrix() );
	g_theRenderer->DrawMesh( m_meshSphere );

	g_theRenderer->SetModelMatrix( m_quadTransform.GetAsMatrix() );
	g_theRenderer->DrawMesh( m_quadMesh );

	g_theRenderer->SetModelMatrix( m_cubeMeshTransform.GetAsMatrix() );
	g_theRenderer->DrawMesh( m_cubeMesh );
	g_theRenderer->SetBlendMode( SOLID );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::RenderUI() const
{

}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateCamera()
{

}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::AddScreenShakeIntensity( float deltaShakeIntensity )
{
	m_screenShakeIntensity += deltaShakeIntensity;
	//clamp it!
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::GarbageCollection()
{
	GarbageDeletion();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::GarbageDeletion()
{

}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::Die()
{

}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::AddLightDevConsoleCommands( DevConsole* devConsole )
{
	EventArgs consoleArgs;
	devConsole->CreateCommand( "ChangeLightColor" , "Ex - ChangeLightColor idx = 0 |color = 255 , 255 , 255 , 255" , consoleArgs );
	g_theEventSystem->SubscribeToEvent( "ChangeLightColor" , ChangeLightColorViaConsole );

	devConsole->CreateCommand( "ChangeLightAttenuation" ,
		"Update Light Attenuation of Ambient Light .Ex - ChangeLightAttenuation idx = 0 |attenuation = 1.f , 0.f , 2.3f" ,
		consoleArgs );
	g_theEventSystem->SubscribeToEvent( "ChangeLightAttenuation" , ChangeLightAttenuationViaConsole );

	devConsole->CreateCommand( "ChangeAmbientLightColor" ,
	                           "Update Color of Ambient Light .Ex - ChangeAmbientLightColor color = 255 , 255 , 255 , 255" ,
	                           consoleArgs );
	g_theEventSystem->SubscribeToEvent( "ChangeAmbientLightColor" , ChangeAmbientLightColorViaConsole );

	devConsole->CreateCommand( "ChangeAmbientLightIntensity" ,
		"Update Intensity of Ambient Light .Ex -  ChangeAmbientLightIntensity intensity = 0.3f" ,
		consoleArgs );
	g_theEventSystem->SubscribeToEvent( "ChangeAmbientLightIntensity" , ChangeAmbientLightIntensityViaConsole );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

STATIC bool Game::ChangeLightColorViaConsole( EventArgs& args )
{
	int		lightIndex		= args.GetValue( "idx " , 0 );
	Rgba8	color			= args.GetValue( "color " , WHITE );
	Vec4	normalizedColor = color.GetAsNormalizedFloat4();
			lightIndex	   %= TOTAL_LIGHTS;
			m_lights.lights[ lightIndex ].color = Vec3( normalizedColor.x , normalizedColor.y , normalizedColor.z );

	return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

STATIC bool Game::ChangeLightAttenuationViaConsole( EventArgs& args )
{
	int		lightIndex	= args.GetValue( "idx " , 0 );
	Vec3	attenuation = args.GetValue( "attenuation " , Vec3::UNIT_VECTOR_ALONG_J_BASIS );
			lightIndex %= TOTAL_LIGHTS;
			m_lights.lights[ lightIndex ].attenuation = attenuation;

	return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

STATIC bool Game::ChangeAmbientLightColorViaConsole( EventArgs& args )
{
	Rgba8	color			= args.GetValue( "color " , WHITE );
	Vec4	normalizedColor = color.GetAsNormalizedFloat4();

	m_lights.ambientLight	= Vec4( normalizedColor.x , normalizedColor.y , normalizedColor.z , m_lights.ambientLight.w );
	m_ambientLightColor.SetColorFromNormalizedFloat( m_lights.ambientLight );
	return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

STATIC bool Game::ChangeAmbientLightIntensityViaConsole( EventArgs& args )
{
	float	intensity		= args.GetValue( "intensity " , 1.f );
			intensity		= ClampZeroToOne(intensity);
	m_lights.ambientLight.w = intensity;
	return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::AddShaderDevConsoleCommands( DevConsole* devConsole )
{
	EventArgs consoleArgs;
	
	devConsole->CreateCommand( "UpdateFresnelShader" ,
		"Update Fresnel Shader.Ex -  UpdateFresnelShader color = 255, 0 , 255 , 255| power = .9f| factor = .3f" ,
		consoleArgs );
	g_theEventSystem->SubscribeToEvent( "UpdateFresnelShader" , UpdateFresnelShaderMaterialDataViaConsole );

	devConsole->CreateCommand( "UpdateDissolveShader" ,
		"Ex -  UpdateDissolveShader startColor = 255, 0 , 255 , 255| endColor = 255, 0 , 255 , 255| edgeWidth = .9f| burnValue = .3f" ,
		consoleArgs );
	g_theEventSystem->SubscribeToEvent( "UpdateDissolveShader" , UpdateDissolveShaderMaterialViaConsole );

	devConsole->CreateCommand( "UpdateDissolveShaderPattern" ,
		"Ex -  UpdateDissolveShaderPattern patternTexPath = Data\\Images\\..." ,
		consoleArgs );
	g_theEventSystem->SubscribeToEvent( "UpdateDissolveShaderPattern" , UpdateDissolveShaderPatternViaConsole );
	
}

//--------------------------------------------------------------------------------------------------------------------------------------------

STATIC bool Game::UpdateFresnelShaderMaterialDataViaConsole( EventArgs& args )
{
	Rgba8 curColor;
	curColor.SetColorFromNormalizedFloat( Vec4( m_fresnelShaderData.fresnelColor , 1.f ) );
	Rgba8 color = args.GetValue( "color " , curColor );
	m_fresnelShaderData.fresnelColor = color.GetAsNormalizedFloat3();

	float power = args.GetValue( "power " , m_fresnelShaderData.fresnelPower );
	m_fresnelShaderData.fresnelPower = power;

	float factor = args.GetValue( "factor " , m_fresnelShaderData.fresnelfactor );
	m_fresnelShaderData.fresnelfactor = factor;

	return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

STATIC bool Game::UpdateDissolveShaderMaterialViaConsole( EventArgs& args )
{
	Rgba8 curStartColor;
	curStartColor.SetColorFromNormalizedFloat( Vec4( m_dissolveShaderData.startColor , 1.f ) );
	Rgba8 startColor = args.GetValue( "startColor " , curStartColor );
	m_dissolveShaderData.startColor = startColor.GetAsNormalizedFloat3();

	Rgba8 curEndColor;
	curEndColor.SetColorFromNormalizedFloat( Vec4( m_dissolveShaderData.endColor , 1.f ) );
	Rgba8 endColor = args.GetValue( "endColor " , curStartColor );
	m_dissolveShaderData.startColor = endColor.GetAsNormalizedFloat3();

	float burnEdgeWidth = args.GetValue( "edgeWidth " , m_dissolveShaderData.burnEdgeWidth );
	m_dissolveShaderData.burnEdgeWidth = burnEdgeWidth;

	float burnValue = args.GetValue( "burnValue " , m_dissolveShaderData.burnValue );
	m_dissolveShaderData.burnValue = burnValue;

	return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

STATIC bool Game::UpdateDissolveShaderPatternViaConsole( EventArgs& args )
{
	std::string patternTexPath		= args.GetValue( "patternTexPath " ,"" );
	m_dissolveShaderPatternTexture	= g_theRenderer->GetOrCreateTextureFromFile( patternTexPath.c_str() );

	return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::AddFogCommandsToDevConsole( DevConsole* devConsole )
{
	EventArgs consoleArgs;
	devConsole->CreateCommand( "DisableFog" , "Disables the Fog" , consoleArgs );
	g_theEventSystem->SubscribeToEvent( "DisableFog" , DisableFog );

	devConsole->CreateCommand( "EnableFog" ,
		"Ex - EnableFog nearFog = 0.f |farFog = 15.f |nearFogColor = 100,100,100,100 |farFogColor = 255,100,0,100" ,
		consoleArgs );
	g_theEventSystem->SubscribeToEvent( "EnableFog" , UpdateFog );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

STATIC bool Game::UpdateFog( EventArgs& args )
{
	m_fogData.nearFog		= args.GetValue( "nearFog "			, m_fogData.nearFog );
	m_fogData.farFog		= args.GetValue( "farFog "			, m_fogData.farFog );
	 
	Rgba8 nearFogColor		= args.GetValue( "nearFogColor "	, GRAY );
	Rgba8 farFogColor		= args.GetValue( "farFogColor "		, Rgba8( 37 , 70 , 87 , 127 ) );
	
	m_fogData.nearFogColor	= nearFogColor.GetAsNormalizedFloat3();
	m_fogData.farFogColor	= farFogColor.GetAsNormalizedFloat3();

	return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

STATIC bool Game::DisableFog( EventArgs& args )
{
	UNUSED( args );
	m_fogData.nearFog			= INFINITY;
	m_fogData.farFog			= INFINITY;
	return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateFromKeyBoard( float deltaSeconds )
{
	if ( g_theDevConsole->IsOpen() )
	{
		return;
	}
	//DebugLineStripDrawModeTest();
	
	if ( g_theInput->WasKeyJustPressed( 'H' ) )
	{
		m_isHUDEnabled = !m_isHUDEnabled;
	}

	CameraPositionUpdateOnInput( deltaSeconds );
	UpdateLightsFromKeyBoard( deltaSeconds );
	UpdateMaterialShaderFromUserInput( deltaSeconds );
	UpdateBlurEffectsOnUserInput();
	UpdateToneMapEffectsOnUserInput( deltaSeconds );
	
	if ( g_theInput->WasKeyJustPressed( 'I' ) )
	{
		g_theInput->ShowSystemCursor();
	}

	if ( g_theInput->WasKeyJustPressed( 'K' ) )
	{
		g_theInput->HideSystemCursor();
	}

	if ( g_theInput->WasKeyJustPressed( 'P' ) )
	{
		g_theInput->ClipSystemCursor( MOUSE_IS_WINDOWLOCKED );
	}

	if ( g_theInput->WasKeyJustPressed( 'L' ) )
	{
		g_theInput->ClipSystemCursor( MOUSE_IS_UNLOCKED );
	}

	//CreateDebugObjectsFromUserInput();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateLightsFromKeyBoard( float deltaSeconds )
{
	SwitchCurrentSelectedLightFromKeyBoard();
	UpdateCurrentSelectedLightFromKeyBoard();
	UpdateLightPositionOnUserInput();
	UpdateCurrentShaderFromUserInput();
	UpdateAmbientLightFromUserInput( deltaSeconds );
	UpdateSpecularLightFromUserInput( deltaSeconds );
	UpdateLightAttenuationFromUserInput();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::SwitchCurrentSelectedLightFromKeyBoard()
{
	if ( g_theInput->WasKeyJustPressed( '1' ) )
	{
		m_currentLightIndex = 0 % TOTAL_LIGHTS;
	}

	if ( g_theInput->WasKeyJustPressed( '2' ) )
	{
		m_currentLightIndex = 1 % TOTAL_LIGHTS;
	}

	if ( g_theInput->WasKeyJustPressed( '3' ) )
	{
		m_currentLightIndex = 2 % TOTAL_LIGHTS;
	}

	if ( g_theInput->WasKeyJustPressed( '4' ) )
	{
		m_currentLightIndex = 3 % TOTAL_LIGHTS;
	}

	if ( g_theInput->WasKeyJustPressed( '5' ) )
	{
		m_currentLightIndex = 4 % TOTAL_LIGHTS;
	}

	if ( g_theInput->WasKeyJustPressed( '6' ) )
	{
		m_currentLightIndex = 5 % TOTAL_LIGHTS;
	}

	if ( g_theInput->WasKeyJustPressed( '7' ) )
	{
		m_currentLightIndex = 6 % TOTAL_LIGHTS;
	}

	if ( g_theInput->WasKeyJustPressed( '8' ) )
	{
		m_currentLightIndex = 7 % TOTAL_LIGHTS;
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateCurrentSelectedLightFromKeyBoard()
{
	if ( g_theInput->WasKeyJustPressed( 'M' ) )
	{
		m_lightType[ m_currentLightIndex ] = ( LightType ) ( ( m_lightType[ m_currentLightIndex ] + 1 ) % TOTAL_LIGHT_TYPES );

		switch ( m_lightType[ m_currentLightIndex ] )
		{
		case POINT_LIGHT:
									{	m_lights.lights[ m_currentLightIndex ].directionFactor = 0.f;
										//m_lights.lights[ m_currentLightIndex ].direction       = Vec3::UNIT_VECTOR_ALONG_K_BASIS;
										m_lights.lights[ m_currentLightIndex ].dotInnerAngle = -1.f;
										m_lights.lights[ m_currentLightIndex ].dotOuterAngle = -1.f;
									}	break;

			case DIRECTIONAL_LIGHT:
									{
										m_lights.lights[ m_currentLightIndex ].directionFactor = 1.f;
										Vec3 direction = m_gameCamera.GetCameraTransform().GetAsMatrix().GetKBasis3D();
										m_lights.lights[ m_currentLightIndex ].direction = -direction;
										m_lights.lights[ m_currentLightIndex ].dotInnerAngle = -1.f;
										m_lights.lights[ m_currentLightIndex ].dotOuterAngle = -1.f;
									}	break;

			case SPOT_LIGHT:
									{
										m_lights.lights[ m_currentLightIndex ].directionFactor = 0.f;
										Vec3 direction = m_gameCamera.GetCameraTransform().GetAsMatrix().GetKBasis3D();
										m_lights.lights[ m_currentLightIndex ].direction = -direction;						
										m_lights.lights[ m_currentLightIndex ].dotInnerAngle = CosDegrees( 15.f );/*ConvertDegreesToRadians( 55.f );*/
										m_lights.lights[ m_currentLightIndex ].dotOuterAngle = CosDegrees( 30.f );/*ConvertDegreesToRadians( 60.f );*/
									}	break;
		}
	}

	//if ( g_theInput->WasKeyJustPressed( 'Q' ) )
	//{
	//	g_theRenderer->EnableLight( m_currentLightIndex , m_lights.lights[ m_currentLightIndex ] );
	//	m_lights.lights[ m_currentLightIndex ].intensity = 1.f;
	//}
	//
	//if ( g_theInput->WasKeyJustPressed( 'E' ) )
	//{
	//	g_theRenderer->DisableLight( m_currentLightIndex );
	//	m_lights.lights[ m_currentLightIndex ].intensity = 0.f;
	//}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateCurrentShaderFromUserInput()
{
	if ( g_theInput->WasKeyJustPressed( KEY_LEFTARROW ) )
	{
		m_currentShaderIndex -= 1;

		if ( m_currentShaderIndex < 0 )
		{
			m_currentShaderIndex = LitShaderTypes::TOTAL_LITSHADERS - 1;
		}

		m_currentShaderIndex %= LitShaderTypes::TOTAL_LITSHADERS;
		m_currentShader = m_lightShaders[ m_currentShaderIndex ];

		if ( LitShaderTypes::FRESNEL == m_currentShaderIndex )
		{
			m_isFresnelShaderActive = true;
			m_currentShader = m_lightShaders[ LitShaderTypes::LIT ];
		}
		else
		{
			m_isFresnelShaderActive = false;
		}

		//if ( LitShaderTypes::FOG == m_currentShaderIndex )
		//{
		//	m_isFogShaderActive = true;
		//	m_currentShader = m_lightShaders[ LitShaderTypes::LIT ];
		//}
		//else
		//{
		//	m_isFogShaderActive = false;
		//}
	}

	if ( g_theInput->WasKeyJustPressed( KEY_RIGHTARROW ) )
	{
		m_currentShaderIndex += 1;
		m_currentShaderIndex %= LitShaderTypes::TOTAL_LITSHADERS;
		m_currentShader = m_lightShaders[ m_currentShaderIndex ];

		if ( LitShaderTypes::FRESNEL == m_currentShaderIndex )
		{
			m_isFresnelShaderActive = true;
			m_currentShader = m_lightShaders[ LitShaderTypes::LIT ];
		}
		else
		{
			m_isFresnelShaderActive = false;
		}

		//if ( LitShaderTypes::FOG == m_currentShaderIndex )
		//{
		//	m_isFogShaderActive = true;
		//	m_currentShader = m_lightShaders[ LitShaderTypes::LIT ];
		//}
		//else
		//{
		//	m_isFogShaderActive = false;
		//}
		
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateMaterialShaderFromUserInput( float deltaSeconds )
{
	if ( LitShaderTypes::DISSOLVE == m_currentShaderIndex )
	{
		if ( g_theInput->IsKeyHeldDown( 'C' ) )
		{
			m_dissolveShaderData.burnEdgeWidth -= deltaSeconds;
			//m_dissolveShaderData.burnEdgeWidth = ClampZeroToOne( m_dissolveShaderData.burnEdgeWidth );
			m_dissolveShaderData.burnEdgeWidth = Clamp( m_dissolveShaderData.burnEdgeWidth , 0.f , ( 1 + 2 * m_dissolveShaderData.burnValue ) );

			m_testMaterial->m_uboIsDirty = true;
		}

		if ( g_theInput->IsKeyHeldDown( 'V' ) )
		{
			m_dissolveShaderData.burnEdgeWidth += deltaSeconds;
			//m_dissolveShaderData.burnEdgeWidth = ClampZeroToOne( m_dissolveShaderData.burnEdgeWidth );
			m_dissolveShaderData.burnEdgeWidth = Clamp( m_dissolveShaderData.burnEdgeWidth , 0.f , ( 1 + 2 * m_dissolveShaderData.burnValue ) );

			m_testMaterial->m_uboIsDirty = true;
		}

		if ( g_theInput->IsKeyHeldDown( 'Z' ) )
		{
			m_dissolveShaderData.burnValue -= deltaSeconds;
			//		m_dissolveShaderData.burnValue = Clamp( m_dissolveShaderData.burnValue , 0.f , ( 1 + 2 * m_dissolveShaderData.burnEdgeWidth ) );
			m_dissolveShaderData.burnValue = ClampZeroToOne( m_dissolveShaderData.burnValue );

			m_testMaterial->m_uboIsDirty = true;
		}

		if ( g_theInput->IsKeyHeldDown( 'X' ) )
		{
			m_dissolveShaderData.burnValue += deltaSeconds;
			//		m_dissolveShaderData.burnValue = Clamp( m_dissolveShaderData.burnValue , 0.f , ( 1 + 2 * m_dissolveShaderData.burnEdgeWidth ) );
			m_dissolveShaderData.burnValue = ClampZeroToOne( m_dissolveShaderData.burnValue );

			
			m_testMaterial->m_uboIsDirty = true;
		}
	}

	if ( LitShaderTypes::FRESNEL == m_currentShaderIndex )
	{
		if ( g_theInput->IsKeyHeldDown( 'C' ) )
		{
			m_fresnelShaderData.fresnelPower -= deltaSeconds;
			m_fresnelShaderData.fresnelPower  = ClampZeroToOne( m_fresnelShaderData.fresnelPower );
		}

		if ( g_theInput->IsKeyHeldDown( 'V' ) )
		{
			m_fresnelShaderData.fresnelPower += deltaSeconds;
			m_fresnelShaderData.fresnelPower  = ClampZeroToOne( m_fresnelShaderData.fresnelPower );
		}

		if ( g_theInput->IsKeyHeldDown( 'Z' ) )
		{
			m_fresnelShaderData.fresnelfactor -= deltaSeconds;
			m_fresnelShaderData.fresnelfactor = ClampZeroToOne( m_fresnelShaderData.fresnelfactor );
		}

		if ( g_theInput->IsKeyHeldDown( 'X' ) )
		{
			m_fresnelShaderData.fresnelfactor += deltaSeconds;
			m_fresnelShaderData.fresnelfactor = ClampZeroToOne( m_fresnelShaderData.fresnelfactor );
		}
	}
	
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateAmbientLightFromUserInput( float deltaSeconds )
{
	if ( g_theInput->IsKeyHeldDown( '9' ) )
	{
		m_lights.ambientLight.w -= deltaSeconds;
		m_lights.ambientLight.w = ClampZeroToOne( m_lights.ambientLight.w );
	}

	if ( g_theInput->IsKeyHeldDown( '0' ) )
	{
		m_lights.ambientLight.w += deltaSeconds;
		m_lights.ambientLight.w = ClampZeroToOne( m_lights.ambientLight.w );
	}

	if ( g_theInput->IsKeyHeldDown( KEY_MINUS ) )
	{
		m_lights.lights[ m_currentLightIndex ].intensity -= deltaSeconds;
	}

	if ( g_theInput->IsKeyHeldDown( KEY_PLUS ) )
	{
		m_lights.lights[ m_currentLightIndex ].intensity += deltaSeconds;
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateSpecularLightFromUserInput( float deltaSeconds )
{
	if ( g_theInput->IsKeyHeldDown( 'U' ) )
	{
		m_lights.SPECULAR_POWER -= 20.f * deltaSeconds;
		m_lights.SPECULAR_POWER = Clamp( m_lights.SPECULAR_POWER , 1.f , INFINITY );
		return;
	}

	if ( g_theInput->IsKeyHeldDown( 'J' ) )
	{
		m_lights.SPECULAR_POWER += 20.f * deltaSeconds;
		m_lights.SPECULAR_POWER = Clamp( m_lights.SPECULAR_POWER , 1.f , INFINITY );
		return;
	}

	if ( g_theInput->IsKeyHeldDown( KEY_LEFT_SQ_BRACKET ) )
	{
		m_lights.SPECULAR_FACTOR -= deltaSeconds;
		m_lights.SPECULAR_FACTOR = ClampZeroToOne( m_lights.SPECULAR_FACTOR );
	}

	if ( g_theInput->IsKeyHeldDown( KEY_RIGHT_SQ_BRACKET ) )
	{
		m_lights.SPECULAR_FACTOR += deltaSeconds;
		m_lights.SPECULAR_FACTOR = ClampZeroToOne( m_lights.SPECULAR_FACTOR );
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateLightAttenuationFromUserInput()
{
	if ( g_theInput->WasKeyJustPressed( 'T' ) )
	{
		m_lights.lights[ m_currentLightIndex ].attenuation = Vec3::ZERO;
	}

	if ( g_theInput->WasKeyJustPressed( 'R' ) )
	{
		m_lights.lights[ m_currentLightIndex ].attenuation = Vec3::UNIT_VECTOR_ALONG_I_BASIS;
	}

	if ( g_theInput->WasKeyJustPressed( 'G' ) )
	{
		m_lights.lights[ m_currentLightIndex ].attenuation = Vec3::UNIT_VECTOR_ALONG_J_BASIS;
	}

	if ( g_theInput->WasKeyJustPressed( 'Y' ) )
	{
		m_lights.lights[ m_currentLightIndex ].attenuation = Vec3::UNIT_VECTOR_ALONG_K_BASIS;
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateLightPositionOnUserInput()
{
	if ( g_theInput->WasKeyJustPressed( KEY_F5 ) )
	{
		m_isLightFollowingTheCamera = false;
		m_isLightAnimated = false;
		m_lights.lights[ m_currentLightIndex ].worldPosition = Vec3::ZERO;

		if ( m_lightType[m_currentLightIndex] != POINT_LIGHT )
		{
			Vec3 direction = m_gameCamera.GetCameraTransform().GetAsMatrix().GetKBasis3D();
			m_lights.lights[ m_currentLightIndex ].direction = -direction;
		}
	}

	if ( g_theInput->WasKeyJustPressed( KEY_F6 ) )
	{
		m_isLightFollowingTheCamera = false;
		m_isLightAnimated = false;
		m_lights.lights[ m_currentLightIndex ].worldPosition = m_gameCamera.GetPosition();

		if ( m_lightType[ m_currentLightIndex ] != POINT_LIGHT )
		{
			Vec3 direction = m_gameCamera.GetCameraTransform().GetAsMatrix().GetKBasis3D();
			m_lights.lights[ m_currentLightIndex ].direction = -direction;
		}		
	}

	if ( g_theInput->WasKeyJustPressed( KEY_F7 ) )
	{
		m_isLightFollowingTheCamera = true;
		m_isLightAnimated = false;
	}

	if ( g_theInput->WasKeyJustPressed( KEY_F9 ) )
	{
		m_isLightFollowingTheCamera = false;
		m_isLightAnimated = true;
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateBlurEffectsOnUserInput()
{
	if ( g_theInput->WasKeyJustPressed( KEY_F2 ) )
	{
		m_isblurShaderActive = !m_isblurShaderActive;
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::UpdateToneMapEffectsOnUserInput( float deltaSeconds )
{
	if ( g_theInput->WasKeyJustPressed( KEY_F3 ) )
	{
		m_isToneMapShaderActive = !m_isToneMapShaderActive;
	}

	if ( g_theInput->IsKeyHeldDown( KEY_SHIFT ) && g_theInput->WasKeyJustPressed( KEY_UPARROW ) )
	{
		m_toneMapTransform.Tw += deltaSeconds * 3.f;
		m_toneMapTransform.Tw = ClampZeroToOne( m_toneMapTransform.Tw );
		return;
	}

	if ( g_theInput->IsKeyHeldDown( KEY_SHIFT ) && g_theInput->WasKeyJustPressed( KEY_DOWNARROW ) )
	{
		m_toneMapTransform.Tw -= deltaSeconds * 3.f;
		m_toneMapTransform.Tw  = ClampZeroToOne( m_toneMapTransform.Tw );
		return;
	}
	
	if ( g_theInput->WasKeyJustPressed( KEY_UPARROW ) )
	{
		m_currentToneMap = ToneMap( ( m_currentToneMap + 1 ) % ToneMap::TOTAL_TONEMAPS );
	}

	if ( g_theInput->WasKeyJustPressed( KEY_DOWNARROW ) )
	{
		m_currentToneMap = ToneMap( ( m_currentToneMap - 1 ) );
		
		if ( m_currentToneMap < 0 )
		{
			m_currentToneMap = ToneMap( ToneMap::TOTAL_TONEMAPS - 1 );
			return;
		}
		
		m_currentToneMap = ToneMap( m_currentToneMap % ToneMap::TOTAL_TONEMAPS );
	}

	if ( g_theInput->IsKeyHeldDown( 'B' ) )
	{
		m_tonePower -= deltaSeconds * 2.f;
		m_tonePower  = Clamp( m_tonePower , 0.f , INFINITY );
	}

	if ( g_theInput->IsKeyHeldDown( 'N' ) )
	{
		m_tonePower += deltaSeconds * 2.f;
		m_tonePower  = Clamp( m_tonePower , 0.f , INFINITY );
	}
	
	switch ( m_currentToneMap )
	{
		case ToneMap::NO_TONE :
		{
			m_toneMapTransform = Mat44::IDENTITY;
		}break;

		case ToneMap::GRAYSCALE:
		{
			Vec3 grayScale = Vec3( 0.2126f , 0.7152f , 0.0722f );
			m_toneMapTransform.SetBasisVectors3D( grayScale , grayScale , grayScale );
		}break;

		case ToneMap::SEPHIA:
		{
			Vec3 newRed		= Vec3( 0.393f , 0.769f , 0.189f );
			Vec3 newGreen	= Vec3( 0.394f , 0.686f , 0.168f );
			Vec3 newBlue	= Vec3( 0.272f , 0.534f , 0.131f );
			m_toneMapTransform.SetBasisVectors3D( newRed , newGreen , newBlue );
		}break;
	}

	m_toneMapTransform.ScaleUniform3D( m_tonePower );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::CreateDebugObjectsFromUserInput()
{
	if ( g_theInput->WasKeyJustPressed( 'U' ) )
	{
		m_debugRenderMode = DEBUG_RENDER_ALWAYS;
	}

	if ( g_theInput->WasKeyJustPressed( 'J' ) )
	{
		m_debugRenderMode = DEBUG_RENDER_USE_DEPTH;
	}

	if ( g_theInput->WasKeyJustPressed( 'N' ) )
	{
		m_debugRenderMode = DEBUG_RENDER_XRAY;
	}

	if ( m_lineStripMode )
	{
		return;
	}

	if ( g_theInput->WasKeyJustPressed( '1' ) )
	{
		DebugAddWorldPoint( m_gameCamera.GetPosition() , 1.f , Rgba8( 0 , 255 , 0 , 127 ) , Rgba8( 0 , 255 , 0 , 127 ) ,
			5.f , m_debugRenderMode );
	}

	if ( g_theInput->WasKeyJustPressed( '2' ) )
	{
		DebugAddWorldLine( m_gameCamera.GetPosition() , GREEN , PINK , m_gameCamera.GetPosition() + Vec3::ONE ,
			PURPLE , ORANGE , 5.f , m_debugRenderMode , 0.5f );
		//DebugAddWorldLine( m_gameCamera.GetPosition() , m_gameCamera.GetPosition() + Vec3::ONE , PURPLE ,15.f , DEBUG_RENDER_ALWAYS , 0.5f );
	}

	if ( g_theInput->WasKeyJustPressed( '3' ) )
	{
		//DebugAddWorldArrow( m_gameCamera.GetPosition() , GREEN , RED , m_gameCamera.GetPosition() - Vec3::ONE ,
		//					PURPLE , ORANGE , 5.f , m_debugRenderMode , 0.5f );

		DebugAddWorldArrow( m_gameCamera.GetPosition() ,
			m_gameCamera.GetPosition() - m_gameCamera.GetCameraTransform().GetAsMatrix().GetKBasis3D() ,
			GREEN , BLUE , PURPLE , ORANGE ,
			CYAN , PINK , YELLOW , MAGENTA , 5.f ,
			m_debugRenderMode , 0.5f , 0.55f );

		//DebugAddWorldLine( m_gameCamera.GetPosition() , m_gameCamera.GetPosition() + Vec3::ONE , PURPLE ,5.f , DEBUG_RENDER_ALWAYS , 0.5f );
	}

	if ( g_theInput->WasKeyJustPressed( '4' ) )
	{
		DebugAddWorldWireBounds(
			AABB3( m_gameCamera.GetPosition() ,
				m_gameCamera.GetPosition() + m_gameCamera.GetCameraTransform().GetAsMatrix().GetJBasis3D() ) ,
			WHITE , 5.0f , m_debugRenderMode );
	}

	if ( g_theInput->WasKeyJustPressed( '5' ) )
	{
		DebugAddWorldWireSphere( m_gameCamera.GetPosition() , 0.5f , YELLOW , 5.0f , m_debugRenderMode );
	}

	if ( g_theInput->WasKeyJustPressed( '6' ) )
	{
		DebugAddWorldBasis( m_gameCamera.GetCameraTransform().GetAsMatrix() , 5.f , m_debugRenderMode );
		//DebugAddWorldBasis( m_gameCamera.GetCameraTransform().GetAsMatrix() , CYAN , PINK , 10.f );
	}

	if ( g_theInput->WasKeyJustPressed( '7' ) )
	{
		//DebugAddWorldBasis( m_gameCamera.GetCameraTransform().GetAsMatrix() , 10.f );
		DebugAddWorldTextf( m_gameCamera.GetCameraTransform().GetAsMatrix() , Vec2::ZERO , ORANGE ,
			10.f , m_debugRenderMode , " Hello %d %f" , 5 , 22.f );
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::DebugLineStripDrawModeTest()
{
	if ( g_theInput->IsKeyHeldDown( KEY_SHIFT ) && g_theInput->WasLeftMouseButtonJustPressed() )
	{
		m_lineStripMode = true;
	}

	if ( g_theInput->IsKeyHeldDown( KEY_SHIFT ) && g_theInput->WasRightMouseButtonJustPressed() )
	{
		if ( m_lineStripPoints.size() > 0 )
		{
			DebugAddWorldLineStrip( ( uint ) m_lineStripPoints.size() , &m_lineStripPoints[ 0 ] , GREEN , WHITE , PINK ,
				BLUE , 20.f , m_debugRenderMode );
		}
		m_lineStripPoints.clear();
		m_lineStripMode = false;
	}

	if ( m_lineStripMode && g_theInput->WasLeftMouseButtonJustPressed() )
	{
		DebugAddWorldPoint( m_gameCamera.GetPosition() , 0.25 , ORANGE ,
			YELLOW , 10.f , m_debugRenderMode );
		m_lineStripPoints.push_back( m_gameCamera.GetPosition() );
	}

	if ( m_lineStripMode && g_theInput->WasKeyJustPressed( KEY_ESC ) )
	{
		m_lineStripMode = false;
		m_lineStripPoints.clear();
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void Game::CameraPositionUpdateOnInput( float deltaSeconds )
{
	Vec3 rotation = Vec3::ZERO;

	Mat44 cameraTransform	= m_gameCamera.GetCameraTransform().GetAsMatrix();
	Vec3 forwardVector		= -cameraTransform.GetKBasis3D();
	Vec3 rightVector		= cameraTransform.GetIBasis3D();
	Vec3 UpVector			= Vec3::UNIT_VECTOR_ALONG_J_BASIS;

	float speed = 4.0f;

	if ( g_theInput->IsKeyHeldDown( KEY_SHIFT ) )
	{
		speed = 20.f;
	}

	if ( g_theInput->IsKeyHeldDown( 'A' ) )
	{
		m_gameCamera.SetPosition( m_gameCamera.GetPosition() - rightVector * speed * deltaSeconds );
	}
	if ( g_theInput->IsKeyHeldDown( 'D' ) )
	{
		m_gameCamera.SetPosition( m_gameCamera.GetPosition() + rightVector * speed * deltaSeconds );
	}
	if ( g_theInput->IsKeyHeldDown( 'W' ) )
	{
		m_gameCamera.SetPosition( m_gameCamera.GetPosition() + forwardVector * speed * deltaSeconds );
	}
	if ( g_theInput->IsKeyHeldDown( 'S' ) )
	{
		m_gameCamera.SetPosition( m_gameCamera.GetPosition() - forwardVector * speed * deltaSeconds );
	}
	if ( g_theInput->IsKeyHeldDown( 'Q' ) )
	{
		m_gameCamera.SetPosition( m_gameCamera.GetPosition() - UpVector * speed * deltaSeconds );
	}
	if ( g_theInput->IsKeyHeldDown( 'E' ) )
	{
		m_gameCamera.SetPosition( m_gameCamera.GetPosition() + UpVector * speed * deltaSeconds );
	}

	if ( g_theInput->WasKeyJustPressed( 'O' ) )
	{
		m_cameraPosition	= Vec3::ZERO;
		m_yaw				= 0.f;
		m_pitch				= 0.f;
	}

	Vec2 mousePos		= g_theInput->GetRelativeMovement();

	m_pitch -= mousePos.y * speed * deltaSeconds;
	m_yaw	-= mousePos.x * speed * deltaSeconds;

	 m_pitch = Clamp( m_pitch , -180.f , 180.f );
	//float finalRoll		= ;//Clamp( m_cameraRotation.y , -85.f , 85.f );

	m_gameCamera.SetPitchYawRollRotation( m_pitch , m_yaw , 0.f );
	//m_gameCamera.SetPitchYawRollRotation( m_cameraRotation.x , m_cameraRotation.z , m_cameraRotation.y );
}


//--------------------------------------------------------------------------------------------------------------------------------------------