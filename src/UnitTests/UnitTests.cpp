#include "pch.h"
#include "CppUnitTest.h"
#include "Engine/Core/Rgba8.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{
	TEST_CLASS(UnitTests)
	{
	public:
		
		TEST_METHOD(ARECOLORSEQUAL)
		{
			Rgba8 testColor = RED;
			Assert::AreEqual( testColor == RED , true , L"They match" );

			testColor = GREEN;
			Assert::AreEqual( testColor == GREEN , true , L"They match" );

			testColor = BLUE;
			Assert::AreEqual( testColor == BLUE , true , L"They match" );
		}
	};
}
