#pragma once
#include "Engine/Platform/Window.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include <string>

//--------------------------------------------------------------------------------------------------------------------------------------------

class RenderContext;
struct ID3D12Resource;
struct ID3D12VertexShader;
struct ID3D12PixelShader;
struct ID3D12ComputeShader;
struct ID3D12Blob;

struct ID3D12InputLayout;
struct ID3D12RasterizerState;
struct buffer_attribute_t;

//--------------------------------------------------------------------------------------------------------------------------------------------

enum SHADER_STAGE_TYPE
{
	SHADER_STAGE_VERTEX ,
	SHADER_STAGE_FRAGMENT ,
	SHADER_STAGE_COMPUTE ,
};

//--------------------------------------------------------------------------------------------------------------------------------------------

class ShaderStage
{
public:
	ShaderStage();
	~ShaderStage();

	bool Compile( RenderContext* ctx , std::string const& filename , void const* source ,
				  size_t const sourceByteLen , SHADER_STAGE_TYPE stageType );


	inline bool IsValid() const { return nullptr != m_handle; }

	void ReleaseShaderStageResources();

	void const* GetByteCode() const;
	size_t GetByteCodeLength() const;


public:
	// a stage could be any one of these types,
	// so we use a enum to identify it, and a union to store it;
	std::string				m_user;
	SHADER_STAGE_TYPE		m_stage;
	ID3D10Blob*				m_byteCode = nullptr ;

	union
	{
		ID3D12Resource*			m_handle;
		ID3D12VertexShader*		m_vertexShader;
		ID3D12PixelShader*		m_fragmentShader; 
		ID3D12ComputeShader*	m_computeShader;
	};
};

//--------------------------------------------------------------------------------------------------------------------------------------------

class Shader
{
public:
	Shader( RenderContext* context );
	~Shader();

	bool CreateFromFile( RenderContext* ctx , std::string const& filename );    
	bool CreateFromString( RenderContext* ctx , std::string const& stringName );   
	// for hooking IA (input assembler) to the VS (vertex shader), 
	// needs to vertex shader and vertex format to make the binding
	//ID3D12InputLayout* GetOrCreateInputLayout( VertexBuffer* vbo );            

	void ReleaseShaderResources();
	bool RecompileShader( std::string const& filename );

	ID3D12InputLayout* GetOrCreateInputLayout( buffer_attribute_t const* attribs );
	   
public:
	//static Shader*				s_errorShader;
	std::string					m_user;
	ShaderStage					m_vertexStage;
	ShaderStage					m_fragmentStage;
	ShaderStage					m_computeStage;
	//ID3D12RasterizerState*		m_rasterState			= nullptr;
	buffer_attribute_t const*	m_lastBufferAttribute	= nullptr;

	// A02 temp
	RenderContext*				m_owner;
	ID3D12InputLayout*			m_inputLayout			= nullptr; // for now, we'll have 1, but in the future you could have one for each different vertex type you use with this; 
//	ID3D12RasterizerState*		m_defaultRasterState	= nullptr;
};

//--------------------------------------------------------------------------------------------------------------------------------------------