#include <vector>

#include "header.h"

main(){
        vector<char> v;
        memSize_t m = persistentReserveForSTLVector<char>(v, 5000, 0.6, 100);
        cout << m << " bytes returned" << endl;
        cout << someFunction() << "some function" << endl;
}

