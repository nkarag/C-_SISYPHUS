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
	 * This function updates a Coordinates struct with the cell
	 * coordinates that extracts from the last domain of a chunk id.
	 *
	 * @param	c	the Coordinates struct to be updated
	 */
	void extractCoords(Coordinates& c) const;


	/**
	 * Receives a  domain  and returns the corresponding coordinates. If input domain
	 * is empty, it returns empty coordinates
	 *
	 * @param domain	input domain
	 * @param coords	output coordinates
	 */
       	static void domain2coords(const string& domain, Coordinates& coords){
		if(domain.empty()){
			coords = Coordinates(); //empty coordinates
			return;
		}
		//create a chunk id corresponding to the input domain
		ChunkID id(domain);
		id.extractCoords(coords);
       	}//domain2coords

	// get/set chunk id
	const string& getcid() const { return cid; }
	void setcid(const string& id) { cid = id; }

	// empty chunk id
	bool empty() {return cid.empty();}


};//ChunkID

main(){

        string domain;
        cout<<"Give me a domain: ";
        cin>>domain;

        Coordinates coords;
        ChunkID::domain2coords(domain, coords);

        //print coordinates
        for(int i = 0; i < coords.numCoords; i++){
                cout<<coords.cVect[i]<<", ";
        }//for
        cout<<endl;

}//main


void ChunkID::extractCoords(Coordinates& c) const
// precondition:
//	this->cid member contains a valid chunk id
// postcondition:
//	each coordinate from the last domain of the chunk id (member cid) has been stored
//	in the vector of the Coordinates struct in the same order as the interleaving order of the
//	chunk id: major-to-minor from left-to-right.
{
	if(cid == "root")
		throw GeneralError("ChunkID::extractCoords ==> Can't extract coords from \"root\"\n");

	// 1. get last domain
	string::size_type pos = cid.rfind("."); //find last "."
	string lastdom;
	if (pos == string::npos){ //then no "." found. This must be a child of the root chunk.
		lastdom = cid;
	}
	else {
		pos += 1; // move on to the first char of the last domain
		string::size_type ldom_length = cid.length()-pos;
		lastdom = cid.substr(pos, ldom_length); // read substring cid[pos]..cid[pos+ldom_length -1]
	}

	// 2. update c
	string::size_type begin = 0;
	string::size_type end = lastdom.find("|", begin); // get 1st "|", i.e. coordinate for 1st dimension
	while(end != string::npos){
        	string coordstr(lastdom, begin, end-begin); // substring lastdom[begin]...lastdom[begin+(end-begin)-1]
        	int coord = atoi(coordstr.c_str());
        	c.cVect.push_back(coord);
        	c.numCoords += 1;
        	// read on
        	begin = end + 1;
        	end = lastdom.find("|", begin);
        } //end while
        // repeat once more for the last dimension
       	string coordstr(lastdom, begin, lastdom.length()-begin); // substring lastdom[begin]...lastdom[begin+l-1], l=lastdom.length()-begin
       	int coord = atoi(coordstr.c_str());
       	c.cVect.push_back(coord);
       	c.numCoords += 1;
} // end of ChunkID::extractCoords

