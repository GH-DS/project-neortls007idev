#include "Engine/Profilling/D3D12PerformanceMarker.hpp"
#include "Engine/Renderer/RenderContext.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------------------------
//				PROFILING D3D POINTER FOR THE SPECIFIC CONFIGURATIONS
//--------------------------------------------------------------------------------------------------------------------------------------------


#if defined ( _DEBUG_PROFILE ) || defined ( _FASTBREAK_PROFILE ) || defined ( _RELEASE_PROFILE )
#include <cguid.h>
#include <atlbase.h>
#include <D3D12_1.h>
CComPtr<ID3DUserDefinedAnnotation> pPerfMarker;
#endif

D3D12PerformanceMarker* g_D3D12PerfMarker = nullptr;

//--------------------------------------------------------------------------------------------------------------------------------------------

D3D12PerformanceMarker::D3D12PerformanceMarker( RenderContext* renderContext )
{
#if defined ( _DEBUG_PROFILE ) || defined ( _FASTBREAK_PROFILE ) || defined ( _RELEASE_PROFILE )

	HRESULT objectCreationSuccessful = renderContext->m_context->QueryInterface( __uuidof( pPerfMarker ) , reinterpret_cast< void** >( &pPerfMarker ) );

	if( FAILED( objectCreationSuccessful ) )
	{
		__debugbreak();
	}
#endif
	UNUSED( renderContext );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

D3D12PerformanceMarker::~D3D12PerformanceMarker()
{

}

//--------------------------------------------------------------------------------------------------------------------------------------------

void D3D12PerformanceMarker::BeginPerformanceMarker( LPCWSTR name )
{
#if defined ( _DEBUG_PROFILE ) || defined ( _FASTBREAK_PROFILE ) || defined ( _RELEASE_PROFILE )
	pPerfMarker->BeginEvent( name );
#endif
	UNUSED( name );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void D3D12PerformanceMarker::EndPerformanceMarker()
{
#if defined ( _DEBUG_PROFILE ) || defined ( _FASTBREAK_PROFILE ) || defined ( _RELEASE_PROFILE )
	pPerfMarker->EndEvent();
#endif
}

//--------------------------------------------------------------------------------------------------------------------------------------------