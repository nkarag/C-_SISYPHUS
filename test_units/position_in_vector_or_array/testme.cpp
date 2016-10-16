#include <iostream>
#include <vector>

int main() {
        // TEST 1: position in an array
/*        int array[100];
        int index;
        cout << "Give me an element index (<100): ";
        cin>>index;
        int* elemp = array + index;
        int pos = int(elemp - array);
        cout<<"\nThe element resides in position: "<< pos <<endl;
*/
        // TEST 2: position in a vector
        vector<int> vect(100);
        int index;
        cout << "Give me an element index (<100): ";
        cin>>index;
        vector<int>::iterator elem_iter = vect.begin() + index;
        int pos = int(elem_iter - vect.begin());
        cout<<"\nThe element resides in position: "<< pos <<endl;

}