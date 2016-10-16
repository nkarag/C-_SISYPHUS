#include <string>
#include <vector>
#include <iostream>

#include "Exceptions.h"

class LevelMember {
public:
        static const int PSEUDO_CODE = -1;
};
/**
 * This is a simple range structure over the order-codes of the members of a level
 * @author: Nikos Karayannidis
 */
struct LevelRange {
	// Note: the condition to check for a null range is: ?(leftEnd==rightEnd)
	static const int NULL_RANGE = -1;
	string dimName;
	string lvlName;
	int leftEnd;
	int rightEnd;

	LevelRange(): dimName(""), lvlName(""), leftEnd(NULL_RANGE), rightEnd(NULL_RANGE){}
	LevelRange(const string& dn, const string& ln, int le, int re):
		dimName(dn),lvlName(ln),leftEnd(le),rightEnd(re){}
	/**
	 * assignment operator, because the compiler complains that it cant use
	 * the default operator=, due to the const member
	 */
	//LevelRange const& operator=(LevelRange const& other);
	/**
	 * copy constructor
	 */
	 //LevelRange::LevelRange(const LevelRange& l);
};//end struct LevelRange


/**
 * This is a simple coordinates structure
 * @author: Nikos Karayannidis
 */
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
        bool empty() {return cVect.empty();}
};//struct Coordinates


class Cell {
public:
	Cell(const Coordinates& c, vector<LevelRange>& b)
		: coords(c), boundaries(b){}
	virtual ~Cell() {}

	//get/set
	const Coordinates& getcoords() const {return coords;}
	void setcoords(const Coordinates&  c) {coords = c;}
	
	const vector<LevelRange>& getboundaries() const {return boundaries;}
	void setboundaries(const vector<LevelRange>& b) {boundaries = b;}
	
	 /**
	  * Returns true if no coordinates exist
	  */
	 bool isCellEmpty(){
		return coords.cVect.empty();
	 }//isCellEmpty
	
	
	 /**
	  * This routine resets all coordinates from coordIndex  and to the right.
	  * If an a coordinate index value is not provided it resets all coordinates.
	  * "Reset" means to set all coordinates to their corresponding leftEnd boundary.
	  * If input index is out of range a GeneralError exception is thrown.If all coordinates are pseudo codes, it
	  * leaves all coordinates unchanged
	  *
	  * @param startCoordIndex	index of coordinate right of which resetting will take place (itself included)
	  */
	 void reset(int startCoordIndex = 0){
	 	if(isCellEmpty())
	 		throw GeneralError("Cell::reset ==> empty cell\n");
	
	 	//assert that input index is within limits
	 	if(startCoordIndex < 0 || startCoordIndex > coords.numCoords-1)
	 		throw GeneralError("Cell::reset ==> input index out of range\n");
	 		
	 	for(int coordIndex = startCoordIndex; coordIndex < coords.numCoords; coordIndex++){
        		//if coord is not a pseudo
        		if(coords.cVect[coordIndex] != LevelMember::PSEUDO_CODE){
				coords.cVect[coordIndex] = boundaries[coordIndex].leftEnd;
        		}//end if	 	
	 	}//end for
	 }//reset
	
protected:
	/**
	 * the coordinates of the cell
	 */
	Coordinates coords;
	
	/**
	 * Ranges of order-codes depicting the boundaries of the domain on each dimension.
	 * These define the data space of the cell
	 */
	vector<LevelRange> boundaries;
};

/**
 * For output to ostream objects (e.g. cout, cerr)
 */
ostream& operator<<(ostream& stream, const Cell& error);


main(){
        //create coordinates
        int numDims = 10;
        vector<int> v(numDims,50);
        v[1] = v[9] = LevelMember::PSEUDO_CODE;
        Coordinates coords(numDims, v);

        //create boundaries
        vector<LevelRange> boundaries;
        boundaries.reserve(numDims);
        for(int i = 0; i<numDims; i++){
                LevelRange rng(string(""), string(""), i, 100+i);
                boundaries.push_back(rng);
        }//for


        //create a new cell
        Cell cell(coords, boundaries);

        //reset it at index i
        int index = 0;
        try{
                cell.reset(2);
        }
	catch(GeneralError& error) {
		GeneralError e("main ==> ");
		error += e;
		cerr<<error<<endl;
		exit(0);
        }

        //print coordinates to screen
        cout<<cell;

}//end main

ostream& operator<<(ostream& stream, const Cell& c)
{
        int numCoords = c.getcoords().numCoords;
        for(int i = 0; i<numCoords; i++) {
                stream<<c.getcoords().cVect[i]<<", ";
        }
        stream << "Boundaries: (";
        for(int i = 0; i<numCoords; i++) {
                if(i == numCoords - 1)
                        stream<<"["<<c.getboundaries()[i].leftEnd<<", "<<c.getboundaries()[i].rightEnd<<"])"<<endl;
                else
                        stream<<"["<<c.getboundaries()[i].leftEnd<<", "<<c.getboundaries()[i].rightEnd<<"], ";
        }

        return stream;
}
