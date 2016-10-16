#include <iostream>
#include <vector>
#include <basics.h> //needed before vec_t.h
#include <vec_t.h>


////////////////////////////////////////////////////////////////////////////
// Testing of vec_t, cvec_t SSM data vector classes (in library COMMON)
//
// Nikos Karayannidis 16/3/2002
//
// compile:
//   g++ -g -I/usr/local/shore2/installed/include/ -I/usr/local/shore2/installed/include/common  -I/usr/local/shore2/installed/include/fc -o testme testme.cpp -L/usr/local/shore2/installed/lib/ -lCOMMON -lFC
////////////////////////////////////////////////////////////////////////////

int main()
{
        vector<int> vi(3,5);

        //create a vec_t from vi
        vec_t ssmvect(&vi[0], vi.size()*sizeof(int)); //Effective STL, Item 16

       vector<char> vc(2, 'c');

       //add vc at the end of ssmvect
       ssmvect.put(&vc[0], vc.size()*sizeof(char));

       vector<double> vd(10, 4.4);

       //add vc at the end of ssmvect
       ssmvect.put(&vd[0], vd.size()*sizeof(double));


       cout<< "length of int vect is :" << ssmvect.len(0) <<endl;
       cout<< "length of char vect is :" << ssmvect.len(1) <<endl;
       cout<< "length of double vect is :" << ssmvect.len(2) <<endl;

       //give me a pointer to the int vector
       int* intp = (int*)ssmvect.ptr(0);
       //print it
       for (int i = 0; i < ssmvect.len(0)/sizeof(int); i++)
                cout << intp[i];
       cout << endl;

       //give me a pointer to the char vector
       char* charp = (char*)ssmvect.ptr(1);
       //print it
       for (int i = 0; i < ssmvect.len(1)/sizeof(char); i++)
                cout << charp[i];
       cout << endl;

       //give me a pointer to the double vector
       double* dp = (double*)ssmvect.ptr(2);
       //print it
       for (int i = 0; i < ssmvect.len(2)/sizeof(double); i++)
                cout << dp[i] <<", ";
       cout << endl;

       cout << "\nsize is : " << ssmvect.size() <<endl;
       cout << "count is : " << ssmvect.count() <<endl;
       cout <<"\nssmvect:\n"<<ssmvect<<endl;

       // test vec_t constructor with byte offset parameter
       vec_t newv(ssmvect, 0, 6);
       //print it
 //      for (int i = 0; i < newv.len(0); i++)
 //               cout << charp[i];
 //      cout << endl;
        cout <<"\nnewv:"<< newv <<endl;


}//main