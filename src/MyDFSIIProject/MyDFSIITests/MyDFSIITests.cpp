#include "pch.h"
#include "CppUnitTest.h"
#include <Worker.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MyDFSIITests
{
	TEST_CLASS(MyDFSIITests)
	{
	public:
		
		TEST_METHOD(TestSomeWork)
		{
			Worker worker;
			std::uint32_t result = worker.doIt(1);
			Assert::AreEqual(10u, result);
		}
		TEST_METHOD(TestSomeMoreWork)
		{
			Worker worker;
			std::uint32_t result = worker.doIt(2);
			Assert::AreEqual(20u, result);
		}
	};
}
