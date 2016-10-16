#include <iostream>
#include <vector>

main()
{
        //create 4 vectors
        vector<char> v1(2, 'a');
        vector<char> v2(4, 'b');
        vector<char> v3(8, 'c');
        vector<char> v4(16, 'd');

        //create a vector of char-vectors
        vector<vector <char> > vv;
        vv.push_back(v1);
        vv.push_back(v2);
        vv.push_back(v3);
        vv.push_back(v4);

        //Now see if you can manipulate vv as a char array

        //get array size
	unsigned int arrsize = 0;
	for(vector<vector<char> >::const_iterator citer = vv.begin(); citer != vv.end(); citer++){
		arrsize += citer->size();
	}// end for
	
	cout << "char array size: " << arrsize << endl;
        	
	//create char array from vv (Effective STL item 16)
	const char* charp = &vv[0][0];
	
	//print the char array
	for(int i = 0; i < arrsize; i++)
	        cout << charp[i];
	cout << endl;


/*      //create a single char vector from the vv
      vector<char> v;
      vector<int> dir; //this will be the directory showing where in v the v1,v2,v3,v4 start.

      // fill v and dir
      dir.push_back(0); //the 1st offset is 0
      for(vector<vector<char> >::const_iterator citer = vv.begin(); citer != vv.end(); citer++){
                v.insert(v.end(), citer->begin(), citer->end());
                dir.push_back(v.size());
      }// end for
      dir.erase(dir.end()-1); //remove last offset, pointing at the end of charp

      //clear memory from vv (see Effective STL Item 17)
      vector<vector<char> >().swap(vv);

     //create char array from v
     const char* charp = reinterpret_cast<char*>(&v[0]);

     //print charp
     for(int i =0; i < arrsize; i++)
        cout << charp[i];
     cout<<endl;

     //print dir
     cout << "dir: ";
     for(vector<int>::iterator i = dir.begin(); i != dir.end(); i++)
        cout << *i <<", ";
     cout<<endl;
*/
}