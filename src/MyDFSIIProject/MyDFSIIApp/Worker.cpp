
#include "Worker.h"

std::uint32_t Worker::doIt(std::uint32_t input)
{
	uint32_t value = 0;
	switch (input)
	{
	case 1: value = 10; break;
	case 2: value = 20; break;
	case 3: value = 31; break;
	default: value = 100; break;
	}
	return value;
}
