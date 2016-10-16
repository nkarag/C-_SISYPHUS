#include <string>
#include <vector>
#include <iostream>
#include <strstream>

#include "Exceptions.h"

class LevelMember {
public:
        static const int PSEUDO_CODE = -1;
};
struct Coordinates {
	int numCoords; //number of coordinates
	vector<int> cVect;  // a coordinate is represented by a signed integer, because
	                   // there is also the case of Level_Member::PSEUDO_CODE, who
	                   // can have a negative value (e.g., -1)
	/**
	 * default constructor
	 */
	Coordinates() : numCoords(0), cVect(){}

	/**
	 * constructor
	 */
	Coordinates(int nc, vector<int>& cds) : numCoords(nc), cVect(cds){}

	/**
	 * copy constructor
	 */
	Coordinates(Coordinates const& c) : numCoords(c.numCoords), cVect(c.cVect) {}

	/**
	 * It fills in another Coordinates struct the coordinates that this->cVect contains
	 * excluding the pseudo coordinates (i.e. coordinates that correspond to pseudo-levels)
	 *
	 * @param coords        the initially empty coord struct that will be filled with the coordinate values
	 */
        //void excludePseudoCoords(Coordinates & coords)const;

        /**
         * returns true if vector with coordinates is empty
         */
        bool empty() const {return cVect.empty();}
};//end struct Coordinates


class ChunkID {

private:
	string cid;

public:
	/**
	 * Default Constructor of the ChunkID
	 */
	ChunkID() : cid(""){ }

	/**
	 * Constructor of the ChunkID
	 */
	ChunkID(const string& s) : cid(s) {}
	/**
	 * copy constructor
	 */
	ChunkID(const ChunkID& id) : cid(id.getcid()) {}
	/**
	 * Destructor of the ChunkID
	 */
	~ChunkID() { }

	/**
	 * Adds a domain as a suffix to the chunk id
	 */
	 void addSuffixDomain(const string& suffix){
	 	//assert that this suffix is valid: corresponds to the same number of dimensions with the this->cid
	 	ChunkID testid(suffix);
	 	bool dummy;
                //cout<<"id:"<<getChunkNumOfDim(dummy)<<endl;
                //cout<<"suffix:"<<testid.getChunkNumOfDim(dummy)<<endl;
	 	if(testid.getChunkNumOfDim(dummy) != getChunkNumOfDim(dummy))
	 		throw GeneralError("ChunkID::addSuffixDomain ==> inappropriate suffix");
	 	//add suffix to chunk id
	 	cid += "." + suffix;
	 }//addSuffixDomain()



	// get/set chunk id
	const string& getcid() const { return cid; }
	void setcid(const string& id) { cid = id; }

	// empty chunk id
	bool empty() {return cid.empty();}

	/**
	 * Derive the number of dimensions from the chunk id. If the chunk id has errors
	 * and the result cannot be derived, then -1 is returned. Also, if the chunk id corresponds
	 * to the root chunk, then 0 is returned and a flag is set to true.
	 *
	 * @param isroot        a boolean output parameter denoting whether the
	 *                      encountered chunk id corresponds to the root chunk
	 */
        const int getChunkNumOfDim(bool& isroot) const;

};//ChunkID

main(){

        string id;
        cout<<"Give me a chunk id: ";
        cin>>id;
        cout<<endl;

        ChunkID cid(id);

        string sufx;
        cout<<"Give me a suffix: ";
        cin>>sufx;
        cout<<endl;

        try {
                cid.addSuffixDomain(sufx);
        }
        catch(GeneralError& e){
                GeneralError error("main ==> ");
                error += e;
                cerr << error;
                exit(0);
        }

        cout<<cid.getcid()<<endl;
}//main


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
                end = cid.find(".", begin);

		//get the appropriate substring
		string::size_type end = cid.find(".", begin); // get next "."
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