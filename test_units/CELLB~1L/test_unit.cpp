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
	 * This routine changes the current state of *this object to
	 * reflect the "next" cell in the lexicographic order of the
	 * coordinates vector. The cell can take any coordinates included in the
	 * data space defined by the boundaries data member. When a cell reaches
	 * the greatest possible coordinates, a following call to this routine will
	 * yield the cell with the smallest possible coordinate. Note: the routine
	 * supports the existence of pseudo levels among the dimensions (i.e., pseudo code
	 * among the coordinates); these coordinates remain unchanged.
	 *
	 */
	 void becomeNextCell();
	
	 /**
	  * Returns true if no coordinates exist
	  */
	 bool isCellEmpty(){
		return coords.cVect.empty();
	 }//isCellEmpty
	
	 /**
	  * Returns true only if the current coordinates of the cell correspond to the
	  * right boundaries of all dimensions. If all coordinates are pseudo codes, it
	  * returns false
	  */
	 bool isFirstCell(){
	 	if(isCellEmpty())
	 		return false;
	 	bool allPseudo = true;
	 	for(int coordIndex = 0; coordIndex < coords.numCoords; coordIndex++){
        		//if coord is not a pseudo
        		if(coords.cVect[coordIndex] != LevelMember::PSEUDO_CODE){
        			allPseudo = false;	 	
        			if(coords.cVect[coordIndex] != boundaries[coordIndex].leftEnd)
        				return false;	 	
        		}//end if
	 	}//end for
	 	
	 	//if not all pseudo then we are indeed at the first cell
	 	return (!allPseudo) ? true : false;
	 }//isFirstCell

	 /**
	  * Returns true only if the current coordinates of the cell correspond to the
	  * left boundaries of all dimensions. If all coordinates are pseudo codes, it
	  * returns false
	  */	 	
	 bool isLastCell(){
	 	if(isCellEmpty())
	 		return false;
	
		bool allPseudo = true;	
	 	for(int coordIndex = 0; coordIndex < coords.numCoords; coordIndex++){
        		//if coord is not a pseudo
        		if(coords.cVect[coordIndex] != LevelMember::PSEUDO_CODE){
        			allPseudo = false;	 		 	
         			if(coords.cVect[coordIndex] != boundaries[coordIndex].rightEnd)
	       				return false;	 	
	       		}//end if
	 	}//end for
	 	
	 	//if not all pseudo then we are indeed at the last cell
	 	return (!allPseudo) ? true : false;
	 }//isLastCell
	
	 /**
	  * Returns true only if *this cell coordinates are within the limits defined
	  * by the boundaries data member. If all coordinates are pseudo codes, it
	  * returns false
	  */
	 bool cellWithinBoundaries(){
	 	if(isCellEmpty())
	 		return false;
	
	 	bool allPseudo = true;
	 	for(int coordIndex = 0; coordIndex < coords.numCoords; coordIndex++){
        		//if coord is not a pseudo
        		if(coords.cVect[coordIndex] != LevelMember::PSEUDO_CODE){
        			allPseudo = false;	 		 	  	 	
          			if(coords.cVect[coordIndex] < boundaries[coordIndex].leftEnd
          				||
          			   coords.cVect[coordIndex] > boundaries[coordIndex].rightEnd)
        				return false;	 	
        		}//end if
	 	}//end for
	 	//if not all pseudo then we are indeed within boundaries
	 	return (!allPseudo) ? true : false;
	 }//cellWithinBoundaries()
	
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
}; //end class Cell

/**
 * For output of a cell to ostream objects (e.g. cout, cerr)
 */
ostream& operator<<(ostream& stream, const Cell& cell);

main(){
        //create coordinates
        int numDims = 5;
        vector<int> v(numDims,50);
        //v[0] = 0;//LevelMember::PSEUDO_CODE;
        //v[1] = 1;
        //v[2] = LevelMember::PSEUDO_CODE;
        //v[3] = LevelMember::PSEUDO_CODE;
        //v[4] = 4;
        /*v[5] = 5;
        v[6] = 6;
        v[7] = 7;
        v[8] = 8;
        v[9] = 9;*/
        //v[5] = 1;
        //v[0] = 5000;
        //v[0] = 10;//LevelMember::PSEUDO_CODE;
        //v[1] = 11;
        v[2] = LevelMember::PSEUDO_CODE;
        v[3] = LevelMember::PSEUDO_CODE;
        //v[4] = 14;

        Coordinates coords(numDims, v);

        //create boundaries
        vector<LevelRange> boundaries;
        boundaries.reserve(numDims);
        for(int i = 0; i<numDims; i++){
                LevelRange rng(string(""), string(""), 50,50);//i, 10+i);
                boundaries.push_back(rng);
        }//for
        //boundaries[0].rightEnd = 40;


        //create a new cell
        Cell cell(coords, boundaries);

        try{
                cell.becomeNextCell();
        }
	catch(GeneralError& error) {
		GeneralError e("main ==> ");
		error += e;
		cerr<<error<<endl;
		exit(0);
        }
        cout << cell << endl;

/*        int countcells = 0;
        do{
                //print coordinates to screen
                cout<<cell;
                countcells++;
                try{
                        cell.becomeNextCell();
                }
	        catch(GeneralError& error) {
		        GeneralError e("main ==> ");
        		error += e;
	        	cerr<<error<<endl;
		        exit(0);
                }
        }while(!cell.isFirstCell());

        cout<<"total Number of cells = "<< countcells<<endl;
*/
        //print coordinates to screen
        //cout<<cell;

}//end main

void Cell::becomeNextCell()
//precondition:
//	*this is a Cell instance contining valid coordinates that fall within the data space
//	defined by the boundaries data member
//processing:
//	finds the rightmost (i.e., less significant) no pseudo coordinate value c where c < MaxBoundaryVal. Then
//	it increases c by one and resets (i.e., sets to the corresponding MinBoundaryVal) all (no pseudo) coordinates
//	to the right of c.If c cannot be found, then all coordinates are reset.
//postcondition:
//	*this is now the "next cell" in the lexicographic order
{
       	if(isCellEmpty())
       		throw GeneralError("Cell::becomeNextCell ==> empty cell\n");

	//assert that current coordinates are within boundaries
	if(!cellWithinBoundaries())
		throw GeneralError("Cell::becomeNextCell() ==> cell out of boundaries\n");
	
	//find righmost coordinate cVect[i] such that cVect[i] < Boundaries[i].rightEnd
	//for each coordinate starting from the right to the left
	bool foundCoord = false;
	int desiredCoordIndex;
	for(int coordIndex = coords.numCoords - 1; coordIndex >= 0; coordIndex--){
		//if coord is not a pseudo
		if(coords.cVect[coordIndex] != LevelMember::PSEUDO_CODE){
			//if coord value precedes its corresponding right boundary
			if(coords.cVect[coordIndex] < boundaries[coordIndex].rightEnd) {
				//then we have found desired coordinate
				foundCoord = true;
				desiredCoordIndex = coordIndex;
				//exit loop
				break;
			}//end if
		}//end if
	}//end for
	
	//if such cell cannot be found
	if(!foundCoord) {
		//reset all coords
		reset();
		//return
		return;
	}//end if
	
	//Else,	increase c by one
	coords.cVect[desiredCoordIndex]++;
	
	//reset all coords to the right of c, if c is not the rightmost coordinate
	if(desiredCoordIndex < coords.numCoords - 1)
		reset(++desiredCoordIndex);	
	return;	
}// end Cell::becomeNextCell()


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
