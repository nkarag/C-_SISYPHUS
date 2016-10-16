#include <iostream>
#include <vector>

main(){
        //int i,j;
        vector<int> j(10,5);
        vector<int>::iterator iter;
        for(int i =0, iter=j.begin(); i<10 && iter!=j.end(); i++, iter++)
                cout<<"i = "<<i<<", j = "<<(*iter)<<endl;
}