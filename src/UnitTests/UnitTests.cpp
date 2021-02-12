#include "pch.h"
#include "CppUnitTest.h"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/RendererDX12/RenderContextDX12.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

Window* g_theWindow = nullptr;

extern RenderContextDX12* g_theRenderer;

namespace UnitTests
{
	TEST_CLASS(UnitTests)
	{
	public:
			
		HRESULT resourceInit;
		TEST_METHOD( Init )
		{
// 			if ( g_unitTestWindow == nullptr )
// 			{
// 				g_unitTestWindow = new Window();
// 				g_unitTestWindow->Startup();
// 			}

			if ( g_theRenderer == nullptr )
			{
				g_theRenderer = new RenderContextDX12();
			}
			Assert::IsNotNull( g_theRenderer , L"Renderer Was initialized" , LINE_INFO() );
		}

// NOTE :- Checking hardware Adapter on CI fails.

		TEST_METHOD( RendererCheckWARPGraphicsAdapters )
		{
			resourceInit = g_theRenderer->CheckGraphicsAdapters( true );
			Assert::AreEqual( resourceInit , S_OK , L"WARP Adapter Creation Check Successful" );
		}
		
		//TEST_METHOD( RendererCheckGPUGraphicsAdapters )
		//{
		//	resourceInit = g_theRenderer->CheckGraphicsAdapters( false );
		//	Assert::AreEqual( resourceInit , S_OK , L"Hardware Adapter Creation Check Successful" );
		//}
		//
		//TEST_METHOD( RendererDeviceCreation )
		//{
		//	resourceInit = g_theRenderer->CreateDevice();
		//	Assert::AreEqual( resourceInit , S_OK , L"DirectX 12 Device Creation Successful" );
		//}					
	};
}
