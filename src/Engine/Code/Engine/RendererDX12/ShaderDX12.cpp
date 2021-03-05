#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/D3D11Common.hpp"
#include "Engine/RendererDX12/RenderContextDX12.hpp"
#include "Engine/RendererDX12/ShaderDX12.hpp"
#include <stdio.h>
#include <initguid.h>
#include <cguid.h>
#include <dxcapi.h>
#include <atlcomcli.h>
#include <wchar.h>
#include <d3dcompiler.h>
 
//#include <wrl/client.h>
//#include <ks.h>
#include <dxgi1_4.h>
#include <dxcapi.h>
#pragma comment( lib, "dxcompiler.lib" )  
#pragma comment( lib, "d3d12.lib" )      
#pragma comment( lib, "dxgi.lib" )  

#ifndef GUID_NULL
	#define GUID_NULL 0
#endif
//#pragma once
//#include "Engine/Renderer/ErrorShaderCode.hpp"
extern char const* g_errorShaderCode;
//--------------------------------------------------------------------------------------------------------------------------------------------
	
static char const* GetDefaultEntryPointForStage(eShaderType type) 
{
	switch (type)
	{
		case SHADER_TYPE_VERTEX:	return "VertexFunction";
		case SHADER_TYPE_FRAGMENT:	return "FragmentFunction";
		case SHADER_TYPE_COMPUTE:	return "ComputeFunction";
		default:					ERROR_AND_DIE( "Un-Supported Shader Stage" );
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

static char const* GetShaderModelForStage(eShaderType type) 
{
	switch (type)
	{
		case SHADER_TYPE_VERTEX:	return "vs_6_0";
		case SHADER_TYPE_FRAGMENT:	return "ps_6_0";
		case SHADER_TYPE_COMPUTE:	return "cs_6_0";
		default:					ERROR_AND_DIE( "Un-Supported Shader Stage" );
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

ShaderStageDX12::~ShaderStageDX12()
{
	DX_SAFE_RELEASE( m_byteCode );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

bool ShaderStageDX12::Compile( std::string const& filename , void const* source , size_t const sourceByteLen , eShaderType stage )
{
	UNUSED( sourceByteLen );
	UNUSED( source );

	char const* entrypoint = GetDefaultEntryPointForStage( stage );
	char const* shadeModel = GetShaderModelForStage( stage );
	UINT compile_Flags = 0U;
	
	UNUSED( compile_Flags );

	CComPtr<IDxcLibrary>  library;
	HRESULT result = DxcCreateInstance( CLSID_DxcLibrary , IID_PPV_ARGS( &library ) );

	if ( FAILED( result ) )
	{
		ERROR_AND_DIE( "DX CREATE INSTANCE for Shader compilation failed" );
	}

	CComPtr<IDxcCompiler>  compiler;
	result = DxcCreateInstance( CLSID_DxcCompiler , IID_PPV_ARGS( &compiler ) );

	if ( FAILED( result ) )
	{
		ERROR_AND_DIE( "Create compiler for shader compile failed" );
	}

	CComPtr<IDxcBlobEncoding> sourceBlod;
	unsigned int codePage = 0;
	std::wstring ws( filename.begin() , filename.end() );
	result = library->CreateBlobFromFile( ws.c_str() , &codePage , &sourceBlod );
	if ( FAILED( result ) )
	{
		ERROR_AND_DIE("Create blob for shader compile failed");
	}

#if defined( DEBUG_SHADERS )
	compileFlags  |= D3DCOMPILE_DEBUG;
	compileFlags  |= D3DCOMPILE_SKIP_OPTIMIZATION;
	compile_Flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#else
	//compile_Flags |= 0;
#endif

	ID3DBlob* byteCode	= nullptr;
	ID3DBlob* errors	= nullptr;

	UNUSED( byteCode );

#define _CRT_SECURE_NO_WARNINGS
	size_t strLength;
	mbsrtowcs_s(&strLength, nullptr,NULL, &entrypoint,20,nullptr );
	wchar_t* entryPointwchar =new wchar_t[strLength +1]();
	mbsrtowcs_s(nullptr, entryPointwchar, strLength, &entrypoint, 20, nullptr);

	mbsrtowcs_s(&strLength, nullptr, NULL, &shadeModel, 20, nullptr);

	wchar_t* shadeModelwChar = new wchar_t[strLength + 1]();
	mbsrtowcs_s(nullptr, shadeModelwChar, strLength, &shadeModel, 20, nullptr);

	CComPtr<IDxcOperationResult> operationResult;
	HRESULT hr = compiler->Compile(sourceBlod,
									ws.c_str(),
									entryPointwchar,
									shadeModelwChar,
									NULL, 0,
									NULL, 0,
									NULL,
									&operationResult );
	if ( SUCCEEDED( hr ) )
	{
		operationResult->GetStatus( &hr );
	}
	if ( FAILED( hr ) )
	{
		//report errors
		if ( operationResult )
		{
			CComPtr<IDxcBlobEncoding> errorBlob;
			hr = operationResult->GetErrorBuffer( &errorBlob );

			if ( SUCCEEDED( hr ) && errorBlob )
			{
				char* errorString = ( char* ) errorBlob->GetBufferPointer();
				DebuggerPrintf( "Failed to compile [%s]. Compile give the follownig output;\n %s" , filename.c_str() , errorString );
			}
		}
		DEBUGBREAK();
	}
	else 
	{
		//link stage step
		operationResult->GetResult(&m_byteCode);

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		switch (stage)
		{
			case SHADER_TYPE_VERTEX: 
				psoDesc.VS.BytecodeLength = m_byteCode->GetBufferSize();
				psoDesc.VS.pShaderBytecode = m_byteCode->GetBufferPointer();
				break;
			case SHADER_TYPE_FRAGMENT:
				psoDesc.PS.BytecodeLength = m_byteCode->GetBufferSize();
				psoDesc.PS.pShaderBytecode = m_byteCode->GetBufferPointer();
				break;
			case SHADER_TYPE_COMPUTE:
				__debugbreak();
				break;
			default:	ERROR_AND_DIE( "Un-Implemented Shader Stage" ) 
						break;
		}
	}
	DX_SAFE_RELEASE( errors );

	m_type = stage;
	return IsValid();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void const* ShaderStageDX12::GetByteCode() const
{
	return m_byteCode->GetBufferPointer();
}

//--------------------------------------------------------------------------------------------------------------------------------------------

size_t ShaderStageDX12::GetByteCodeLength() const
{
	return m_byteCode->GetBufferSize();
}

//--------------------------------------------------------------------------------------------------------------------------------------------
	
D3D12_SHADER_BYTECODE ShaderStageDX12::GetAsD3D12ByteCode()
{
	D3D12_SHADER_BYTECODE result;
	result.BytecodeLength = m_byteCode->GetBufferSize();
	result.pShaderBytecode = reinterpret_cast< UINT8* > ( m_byteCode->GetBufferPointer() );
	return result;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

ShaderDX12::ShaderDX12( RenderContextDX12* ctx , std::string name ) : 
																		m_owner( ctx ), 
																		m_name( name )
{

}

//--------------------------------------------------------------------------------------------------------------------------------------------

ShaderDX12::~ShaderDX12()
{

}

//--------------------------------------------------------------------------------------------------------------------------------------------

bool ShaderDX12::CreateFromFile( RenderContextDX12* ctx , std::string const& filename )
{
	size_t fileSize = 0;
	void* src = FileReadToNewBuffer( filename , &fileSize );
	if ( src == nullptr )
	{
		return false;
	}
		
	m_vertexStage.Compile( filename , src , fileSize , SHADER_TYPE_VERTEX );
	m_fragmentStage.Compile( filename , src , fileSize , SHADER_TYPE_FRAGMENT );
	
	delete[] src;
	
	if ( ( m_vertexStage.IsValid() && m_fragmentStage.IsValid() ) /*|| m_computeStage.IsValid()*/ )
	{
		return true;
	}
	else
	{
		CreateFromString( ctx , g_errorShaderCode );
		return false;
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------

bool ShaderDX12::CreateFromString( RenderContextDX12* ctx , std::string const& stringName )
{
	UNUSED( ctx );
	m_name = stringName;

	size_t fileSize = stringName.length();
	void* source = ( void* ) stringName.c_str();
	if ( nullptr == source )
	{
		return false;
	}

	m_vertexStage.Compile( stringName , source , fileSize , SHADER_TYPE_VERTEX );
	m_fragmentStage.Compile( stringName , source , fileSize , SHADER_TYPE_FRAGMENT );
	//m_computeStage.Compile( m_owner , stringName , source , fileSize , SHADER_STAGE_COMPUTE );

	return m_vertexStage.IsValid() && m_fragmentStage.IsValid(); /*&& m_computeStage.IsValid()*/
}

//--------------------------------------------------------------------------------------------------------------------------------------------
	