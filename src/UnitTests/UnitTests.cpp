#include "pch.h"
#include "CppUnitTest.h"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/RendererDX12/RenderContextDX12.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

//--------------------------------------------------------------------------------------------------------------------------------------------
	
Window* g_theWindow = nullptr;
extern RenderContextDX12* g_theRenderer;


//--------------------------------------------------------------------------------------------------------------------------------------------

namespace UnitTests
{
	TEST_CLASS(UnitTests)
	{
//--------------------------------------------------------------------------------------------------------------------------------------------
	
	TEST_CLASS_INITIALIZE( RenderContextDX12Init )
	{
		if ( g_theRenderer == nullptr )
		{
			g_theRenderer = new RenderContextDX12();
		}
		Assert::IsNotNull( g_theRenderer , L"Renderer Was initialized" , LINE_INFO() );
	}

	//--------------------------------------------------------------------------------------------------------------------------------------------

	TEST_CLASS_CLEANUP( RenderContextDX12Destruction )
	{
		g_theRenderer->Shutdown();
		SAFE_RELEASE_POINTER( g_theRenderer );
	}

			HRESULT m_resourceInit;
	public:
//----------------------------------------------------------------------------------------------------------			
		TEST_METHOD( A00000_Init )
		{
			m_resourceInit = g_theRenderer->CreateDevice();
			Assert::AreEqual( m_resourceInit , S_OK , L"DirectX 12 Device Creation Successful" );
		}
//----------------------------------------------------------------------------------------------------------
 		TEST_METHOD( A00010_RendererCheckWARPGraphicsAdapters )
 		{
 			m_resourceInit = g_theRenderer->CheckGraphicsAdapters( true );
 			Assert::AreEqual( m_resourceInit , S_OK , L"WARP Adapter Creation Check Successful" );
 		}
//----------------------------------------------------------------------------------------------------------
		TEST_METHOD( A00020_RendererCheckGPUGraphicsAdapters )
		{
			m_resourceInit = g_theRenderer->CheckGraphicsAdapters( false );
			Assert::AreEqual( m_resourceInit , S_OK , L"Hardware Adapter Creation Check Successful" );
		}			
	};
}

//--------------------------------------------------------------------------------------------------------------------------------------------
