#pragma once
#pragma once
#include <string>
#include "Engine/RendererDX12/D3D12Common.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

struct	IDxcBlob;
class	DX12RenderContext;
class	RenderContextDX12;

//--------------------------------------------------------------------------------------------------------------------------------------------

enum eShaderType
{
	SHADER_TYPE_VERTEX ,
	SHADER_TYPE_FRAGMENT ,
	SHADER_TYPE_COMPUTE ,
};

//--------------------------------------------------------------------------------------------------------------------------------------------

class ShaderStageDX12
{
public:
	~ShaderStageDX12();
	bool Compile( std::string const& filename , void const* source , size_t const sourceByteLen , eShaderType stage );

	bool					IsValid() const																			{ return ( nullptr != m_byteCode ); }
	void const*				GetByteCode() const;
	size_t					GetByteCodeLength() const;
	D3D12_SHADER_BYTECODE	GetAsD3D12ByteCode();

public:
	eShaderType				m_type;
	IDxcBlob*				m_byteCode = nullptr;

};

//--------------------------------------------------------------------------------------------------------------------------------------------

class ShaderDX12
{

public:
	ShaderDX12( RenderContextDX12* ctx , std::string name );
	~ShaderDX12();
	bool CreateFromFile( RenderContextDX12* ctx , std::string const& filename );
	bool CreateFromString( RenderContextDX12* ctx , std::string const& stringName );

	void ReleaseShaderResources();
	bool RecompileShader( std::string const& filename );
	//ID3D11InputLayout* GetOrCreateInputLayout();
	//ID3D12Inputlayout* GetOrCreateInputLayout(const buffer_attribute_t* list);

public:
	RenderContextDX12*	m_owner;
	std::string			m_name;
	ShaderStageDX12		m_vertexStage;
	ShaderStageDX12		m_fragmentStage; 

	// m_inputLayout = nullptr;
	// m_defaultRasterState = nullptr;
};