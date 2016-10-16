#include <vector>
#include <new>


#if UINT_MAX == 4294967295
typedef unsigned int memSize_t; //4GB of memory space max
#else
typedef unsigned long int memSize_t;
#endif


template<class T> memSize_t persistentReserveForSTLVector(
        vector<T>& v,
        memSize_t szRequest,
        float reduction_factor,
        memSize_t minSize
        ) throw(std::bad_alloc);

int someFunction();