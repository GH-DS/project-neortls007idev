#include "pch.h"
#include "CppUnitTest.h"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/MathUtils.hpp"

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

		TEST_METHOD( VectorLength )
		{
			Vec2 A( 8.f , 6.f );
			Assert::AreEqual( A.GetLength() , 10.f , L"They match" );

			Vec2 B( 3.f , 4.f );
			Assert::AreEqual( B.GetLength() , 5.f , L"They match" );

			Assert::AreEqual( RoundDownToInt( A.GetAngleDegrees() ) , 36 , L"They match" );
			Assert::AreEqual( RoundDownToInt( B.GetAngleDegrees() ) , 53 , L"They match" );
		}
		
	};
}
