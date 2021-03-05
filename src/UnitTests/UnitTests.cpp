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
		Assert::IsNotNull( g_theRenderer , L"Renderer initialization Failed" , LINE_INFO() );
		Logger::WriteMessage( "Renderer Initialized" );
	}

	//--------------------------------------------------------------------------------------------------------------------------------------------

	TEST_CLASS_CLEANUP( RenderContextDX12Destruction )
	{
		g_theRenderer->Shutdown();
		SAFE_RELEASE_POINTER( g_theRenderer );
		Logger::WriteMessage( "Renderer Destroyed" );
	}

			HRESULT m_resourceInit;
	public:
//----------------------------------------------------------------------------------------------------------			
		TEST_METHOD( A00000_Init )
		{
			m_resourceInit = g_theRenderer->CreateDevice();
			Assert::AreEqual( m_resourceInit , S_OK , L"DirectX 12 Device Creation Failed" );
			Logger::WriteMessage( "DirectX 12 Device Creation Successful" );
		}
//----------------------------------------------------------------------------------------------------------
 		TEST_METHOD( A00010_RendererCheckWARPGraphicsAdapters )
 		{
 			m_resourceInit = g_theRenderer->CheckGraphicsAdapters( true );
 			Assert::AreEqual( m_resourceInit , S_OK , L"WARP Adapter Creation Check Failed" );
			Logger::WriteMessage( "DirectX 12 WARP Adapter device Creation Successful" );
 		}
//----------------------------------------------------------------------------------------------------------
		TEST_METHOD( A00020_RendererCheckGPUGraphicsAdapters )
		{
			m_resourceInit = g_theRenderer->CheckGraphicsAdapters( false );
			Assert::AreEqual( m_resourceInit , S_OK , L"Hardware Adapter Creation Check Failed" );
			Logger::WriteMessage( "DirectX 12 GPU Adapter device Creation Successful" );
		}			
//----------------------------------------------------------------------------------------------------------
		TEST_METHOD( A00030_CommandQueueCreation )
		{
			g_theRenderer->m_commandQueue = g_theRenderer->CreateCommandQueue( DX12_COMMAND_LIST_TYPE_DIRECT );
			Assert::IsNotNull( g_theRenderer->m_commandQueue , L"Command Queue Creation Failed" );
			Logger::WriteMessage( "DirectX 12 Command Queue Creation Successful" );
		}	
//----------------------------------------------------------------------------------------------------------
		TEST_METHOD( A00040_SwapChainCreation )
		{
			m_resourceInit |= g_theRenderer->CreateSwapChain( g_theRenderer->m_commandQueue , g_theRenderer->m_numBackBufferFrames );
			Assert::AreEqual( m_resourceInit , S_OK , L"Hardware Adapter Creation Check Successful" );
			Logger::WriteMessage( "DirectX 12 Swap Chain Creation Successful" );
		}
	};
}

//--------------------------------------------------------------------------------------------------------------------------------------------
