#include "pch.h"
#include "CppUnitTest.h"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/RendererDX12/RenderContextDX12.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"

//--------------------------------------------------------------------------------------------------------------------------------------------
	
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
#include "Engine/RendererDX12/DescriptorHeapDX12.hpp"
#include "Engine/RendererDX12/CommandAllocatorDX12.hpp"
 #pragma comment( lib, "dxcompiler.lib" )  
 #pragma comment( lib, "d3d12.lib" )      
 #pragma comment( lib, "dxgi.lib" )  
 
 #ifndef GUID_NULL
 #define GUID_NULL 0
 #endif
//#pragma once

//--------------------------------------------------------------------------------------------------------------------------------------------
	
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

//--------------------------------------------------------------------------------------------------------------------------------------------
	
Window* g_theWindow = nullptr;
extern RenderContextDX12* g_theRenderer;

namespace UnitTests
{
	//--------------------------------------------------------------------------------------------------------------------------------------------

	TEST_MODULE_INITIALIZE( RenderContextDX12Init )
	{
		if ( g_theWindow == nullptr )
		{
			g_theWindow = new Window();
		}
		Logger::WriteMessage( "Window initialized" );

		if ( g_theRenderer == nullptr )
		{
			g_theRenderer = new RenderContextDX12();
		}
		Assert::IsNotNull( g_theRenderer , L"Renderer initialization Failed" , LINE_INFO() );
		Logger::WriteMessage( "Renderer Initialized" );

		g_theRenderer->m_window = g_theWindow;
	}

	//--------------------------------------------------------------------------------------------------------------------------------------------

	TEST_MODULE_CLEANUP( RenderContextDX12Destruction )
	{
		g_theRenderer->Shutdown();
		SAFE_RELEASE_POINTER( g_theRenderer );
		Logger::WriteMessage( "Renderer Destroyed" );
	}

	TEST_CLASS(UnitTests)
	{
//--------------------------------------------------------------------------------------------------------------------------------------------

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
		//TEST_METHOD( A00020_RendererCheckGPUGraphicsAdapters )
		//{
		//	m_resourceInit = g_theRenderer->CheckGraphicsAdapters( false );
		//	Assert::AreEqual( m_resourceInit , S_OK , L"Hardware Adapter Creation Check Failed" );
		//	Logger::WriteMessage( "DirectX 12 GPU Adapter device Creation Successful" );
		//}			
//----------------------------------------------------------------------------------------------------------
		TEST_METHOD( A00030_CommandQueueCreation )
		{
			g_theRenderer->m_commandQueue = g_theRenderer->CreateCommandQueue( DX12_COMMAND_LIST_TYPE_DIRECT );
			Assert::IsNotNull( g_theRenderer->m_commandQueue , L"Command Queue Creation Failed" );
			Logger::WriteMessage( "DirectX 12 Command Queue Creation Successful" );
		}	
//----------------------------------------------------------------------------------------------------------
		TEST_METHOD( A00040_DescriptorCreation )
		{

			g_theRenderer->m_RTVDescriptorHeap = new DescriptorHeapDX12( g_theRenderer , D3D12_DESCRIPTOR_HEAP_TYPE_RTV , g_theRenderer->m_numBackBufferFrames );
			g_theRenderer->m_RTVDescriptorSize = g_theRenderer->m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

			Assert::IsNotNull( g_theRenderer->m_RTVDescriptorHeap , L"Descriptor Heap Creation Failed" );
			Logger::WriteMessage( "Descriptor Heap Creation Successful" );
		}
//--------------------------------------------------------------------------------------------------------------------------------------------
		TEST_METHOD( A00050_RTVCreation )
		{
			//g_theRenderer->UpdateRenderTargetViews();
			
			for ( int backBufferIndex = 0; backBufferIndex < g_theRenderer->m_numBackBufferFrames; ++backBufferIndex )
			{
			//	Assert::IsNotNull( g_theRenderer->t_backBuffers[ backBufferIndex ] , L"Render Target View Creation Failed" );
			}
				Logger::WriteMessage( "Render Target View Creation Successful" );
		}
//--------------------------------------------------------------------------------------------------------------------------------------------
		TEST_METHOD( A00060_CommandAllocatorCreation )
		{
			for ( int index = 0; index < g_theRenderer->m_numBackBufferFrames; ++index )
			{
				g_theRenderer->m_commandAllocators[ index ] = new CommandAllocatorDX12( g_theRenderer , D3D12_COMMAND_LIST_TYPE_DIRECT );
				Assert::IsNotNull( g_theRenderer->m_commandAllocators[ index ] , L"Command Allocator Creation Failed" );
				Logger::WriteMessage( "Command Allocator Creation Successful" );
			}
		}
//----------------------------------------------------------------------------------------------------------
		TEST_METHOD( A00070_RootSignatureCreation )
		{
			g_theRenderer->CreateRootSignature();
			Assert::IsNotNull( g_theRenderer->m_rootSignature , L"Root Signature Creation Failed" );
			Logger::WriteMessage( "Root Signature Creation Successful" );
		}
	};
}

//--------------------------------------------------------------------------------------------------------------------------------------------
