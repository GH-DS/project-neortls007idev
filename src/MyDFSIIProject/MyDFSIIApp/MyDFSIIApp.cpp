
#include <iostream>
#include "Worker.h"


int main()
{
    Worker worker;

    std::cout << "We did some work and result was: " << worker.doIt(2) << std::endl;
}
