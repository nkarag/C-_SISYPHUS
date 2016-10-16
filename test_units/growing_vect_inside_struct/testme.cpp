/*
  Test vector reallocation when the vector
  is a data member of a class. If I have a pointer
  to this struct does it become invalid after rallocation of the vector?
  How is the class/struct members aligned in memory after a reallocation?

*/
#include <iostream>
#include <vector>

class TestClass {
        vector<int> v;
public:
        TestClass(int size, int val): v(size, val){}
        vector<int>& getv() {return v;}
};

main(){
        TestClass* tp = new TestClass(3,5);

        cout << "Before allocation: " << tp->getv().front() << endl;
        cout << "size = " << tp->getv().size() << endl;
        cout << "capacity = " << tp->getv().capacity() << endl;
        cout << "&tp->v[0] = " << reinterpret_cast<int>(&(tp->getv())[0]) << endl;

        for(int i=0; i<1000000; i++)
                tp->getv().push_back(i);

        cout << "After allocation: " << tp->getv().front() << endl;
        cout << "size = " << tp->getv().size() << endl;
        cout << "capacity = " << tp->getv().capacity() << endl;
        cout << "&tp->v[0] = " << reinterpret_cast<int>(&(tp->getv())[0]) << endl;

}