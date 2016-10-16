//#include <vector>
#include "header.h"

export template<class T> memSize_t persistentReserveForSTLVector(
			vector<T>& v, memSize_t szRequest, float reduction_factor, memSize_t minSize) throw(std::bad_alloc)
{
        return szRequest;
}

int someFunction()
{ return 5; }