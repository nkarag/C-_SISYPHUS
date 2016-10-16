#include <climits>
#include <iostream>

main(){

#if UINT_MAX == 4294967295
        typedef unsigned int memSz;
#else
        typedef unsigned long int memSz;
#endif

        cout << "memSz = : "<< sizeof(memSz) << " bytes"<<endl;
}