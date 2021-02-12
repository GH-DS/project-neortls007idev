#pragma once

typedef const wchar_t* LPCWSTR;
//--------------------------------------------------------------------------------------------------------------------------------------------

class RenderContext;

//--------------------------------------------------------------------------------------------------------------------------------------------

class D3D12PerformanceMarker
{
public:

	// Will only be created when special Profilling configuration are created and pre-processor are added to the configurations in project settings
	D3D12PerformanceMarker( RenderContext* renderContext );
	~D3D12PerformanceMarker();

	void BeginPerformanceMarker( LPCWSTR name );
	void EndPerformanceMarker();
};

//--------------------------------------------------------------------------------------------------------------------------------------------
