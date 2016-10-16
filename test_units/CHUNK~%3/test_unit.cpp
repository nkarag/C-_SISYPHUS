#include <string>
//#include <vector>
//#include <map>
#include <iostream>

class ChunkID{
        string cid;
public:
        ChunkID(string& s): cid(s){}

        const int getChunkNumOfDim(bool& isroot) const;		
};

main(){
        string in;

        cout<<"Give me a chunk id: ";
        cin>>in;

        ChunkID chnkid(in);
        bool isroot;
        int numdim = chnkid.getChunkNumOfDim(isroot);
        if(numdim == -1){
                cerr<<"Error!!!"<<endl;
                exit(1);
        }
        else if(!numdim){
                cerr<<"root!"<<endl;
                exit(1);
        }
        cout<<"No dims = "<<numdim<<endl;
}	

const int ChunkID::getChunkNumOfDim(bool& isroot) const
// precondition:
//      this->cid, contains a valid chunk id && isroot is an output parameter
// postcondition:
//      case1: the number of dimensions derived from the chunk id is returned, therefore
//      the returned value is >0. Also, isroot is false, meaning that this is not the root chunk.
//      case2: isroot == true (&& returned value == 0), then no valid number of dimensions is returned
//      because this cannot be derived from the chunk id for a root chunk.
//      case3: isroot == false && returned value ==-1, this means that an error has been encountered
//      in the chunk id and the number of dims couldn't be derived.
{
        isroot = false; //flag initialization
	if(cid == "root"){
	        //if this is the root chunk
		isroot = true; //turn on flag		
		return 0; // no way to figure out the no of dims from this chunk id
	}

	int numDims = 1;
        bool first_domain = true;
	string::size_type begin = 0;	
        string::size_type end;
	do{
       	        //get next domain
                end = cid.find(".", begin); // get next "."
		//get the appropriate substring		
		// if end==npos then no "." found, i.e. this is the last domain
		// end-begin == the length of the domain substring => substring cid[begin]...cid[begin+(end-begin)-1]		
		string domain = (end == string::npos) ?
		                        string(cid, begin, cid.length()-begin) : string(cid, begin, end-begin);				
                //now we' ve got a domain
                int tmpNumDims = 1;		
                //search the domain for "|" denoting different dimensions
               	for (int i = 0; i<domain.length(); i++){
        		if (domain[i] == '|')
        			tmpNumDims++; //add one more dim			
        	}//end for
		if(first_domain){
		        //get the number of dims calculated from the 1st domain
		        numDims = tmpNumDims;
		        first_domain = false;
		}
                else //assert that the num of dims derived from the rest of the domains, is the same		
                     //as the one calculated from the 1st domain
                        if(numDims != tmpNumDims) //then some error must exist in the chunk id
                                return -1;		
		begin = end+1;			
	}while(end != string::npos); //while there are still (at least one more) domains
	
	return numDims;
}// end of ChunkID::getChunkNumOfDim

