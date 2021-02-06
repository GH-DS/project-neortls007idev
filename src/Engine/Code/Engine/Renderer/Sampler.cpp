#include "Engine/Renderer/Sampler.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Renderer/D3DCommon.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

Sampler::Sampler( RenderContext* ctx , eSamplerType type , eCompareOp comparisonOp /*= COMPARE_NEVER*/ )
{
	m_handle = nullptr;
	m_owner = ctx;
	ID3D12Device* device = ctx->m_device;
	
	D3D12_SAMPLER_DESC desc;

	if ( type == SAMPLER_POINT )
	{
		//desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		desc.Filter = D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
	}
	else if ( type == SAMPLER_BILINEAR )
	{
		desc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	}
	else if( type == SAMPLER_LINEAR )
	{
		//desc.Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
		desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
	}
	
	desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

	desc. MipLODBias = 0.0f;
	desc.MaxAnisotropy = 0;
	desc.ComparisonFunc = GetD3D12ComparisonFunc( comparisonOp );
	desc.BorderColor[ 0 ] = 0.0f;
	desc.BorderColor[ 1 ] = 0.0f;
	desc.BorderColor[ 2 ] = 0.0f;
	desc.BorderColor[ 3 ] = 0.0f;
	desc.MinLOD = 0.0f;
	desc.MaxLOD = 0.0f;

	if ( type == SAMPLER_CUBEMAP )
	{
		ZeroMemory( &desc , sizeof( desc ) );
		desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		desc.MinLOD = 0.f;
		desc.MaxLOD = D3D12_FLOAT32_MAX;	
	}
	
	device->CreateSamplerState( &desc , &m_handle );

	std::string debugName = "RenderContext Resource";
	SetDebugName( m_handle , &debugName );

	m_type = type;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

Sampler::~Sampler()
{
	DX_SAFE_RELEASE( m_handle );
}

//--------------------------------------------------------------------------------------------------------------------------------------------