#include <string>
#include <vector>
#include <iostream>
#include <strstream>
#include <algorithm>

#include "Exceptions.h"

class LevelMember{
public:
        static const int PSEUDO_CODE = -1;
};

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
	// LevelRange::LevelRange(const LevelRange& l);
};//end struct LevelRange


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
	Coordinates(int nc, const vector<int>& cds) : numCoords(nc), cVect(cds){}

	/**
	 * copy constructor
	 */
	Coordinates(Coordinates const& c) : numCoords(c.numCoords), cVect(c.cVect) {}

        /**
         * returns true if vector with coordinates is empty
         */
        bool empty() const {return cVect.empty();}
};//end struct Coordinates


class Cell {
public:
	Cell(const Coordinates& c, const vector<LevelRange>& b)
		: coords(c), boundaries(b){
	        //if coordinates and boundaries have different dimensionality
	        if(coords.numCoords != boundaries.size())
	                throw GeneralError("Cell::Cell ==> different dimensionality between coordinates and boundaries");	
	}
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
	 * among the coordinates)
	 *
	 */
	 void becomeNextCell();

	 /**
	  * Returns true if no coordinates exist
	  */
	 bool isCellEmpty() const {
	        //either empty coordinates or boundaries means the same: an empty cell!
		return coords.cVect.empty() || boundaries.empty();
	 }//isCellEmpty

	 /**
	  * Returns true only if the current coordinates of the cell correspond to the
	  * right boundaries of all dimensions. If all coordinates are pseudo codes, it
	  * returns false
	  */
	 bool isFirstCell() const {
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
	 bool isLastCell() const {
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
	 bool cellWithinBoundaries() const {
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
	 * Required in order to use the find standard algortithm (see STL)
	 * for ChunkIDs
	 */
	friend bool operator==(const ChunkID& c1, const ChunkID& c2)
        {
        	return (c1.cid == c2.cid);
        }

	/**
	 * Adds a domain as a suffix to the chunk id
	 */
	 void addSuffixDomain(const string& suffix){
	 	//assert that this suffix is valid: corresponds to the same number of dimensions with the this->cid
	 	ChunkID testid(suffix);
	 	bool dummy;
	 	if(testid.getChunkNumOfDim(dummy) != getChunkNumOfDim(dummy))
	 		throw GeneralError("ChunkID::addSuffixDomain ==> suffix and chunk id dimensionality mismatch");
	 	//add suffix to chunk id
	 	cid += "." + suffix;
	 }//addSuffixDomain()


	/**
	 * This function updates a Coordinates struct with the cell
	 * coordinates that extracts from the last domain of a chunk id.
	 *
	 * @param	c	the Coordinates struct to be updated
	 */
	void extractCoords(Coordinates& c) const;

	/**
	 * Receives an instance of coordinates and returns the corresponding domain
	 * If input coodinates are empty then it returns an empty domain
	 *
	 * @param coords	input coordinates
	 * @param domain	output domain
	 */
       	static void coords2domain(const Coordinates& coords, string& domain){
                if(coords.empty()){
                        domain = string("");
                        return;
                }

                //create an output string stream
                ostrstream dom;
                //for each coordinate
                for(int cindex = 0; cindex < coords.numCoords; cindex++){
                        //if this is not the last coordinate
                        if(cindex < coords.numCoords - 1)
                                dom<<coords.cVect[cindex]<<"|";
                        else
                                dom<<coords.cVect[cindex];
                }//end for
                dom<<ends;
                domain = string(dom.str());
       	}//coords2domain

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
	bool empty() const {return cid.empty();}

	// get prefix domain of chunk id. if chunk id is empty it returns an empty string
	const string get_prefix_domain() const;

	// get suffix domain of chunk id. f chunk id is empty it returns an empty string
	const string get_suffix_domain() const;


	/**
	 * Derive the number of dimensions from the chunk id. If the chunk id has errors
	 * and the result cannot be derived, then -1 is returned. Also, if the chunk id corresponds
	 * to the root chunk, then 0 is returned and a flag is set to true.
	 *
	 * @param isroot        a boolean output parameter denoting whether the
	 *                      encountered chunk id corresponds to the root chunk
	 */
        const int getChunkNumOfDim(bool& isroot) const;
};//end class ChunkID


/**
 * This class shows which cells inside a chunk have non-NULL values
 *
 * @author Nikos Karayannidis
 */
class CellMap {

public:
	/**
	 * default constructor, initializes an empty vector
	 */
	CellMap::CellMap() : chunkidVectp(new vector<ChunkID>) {}
	CellMap::~CellMap() { delete chunkidVectp; }

	/**
	 * copy constructor
	 */
	CellMap(CellMap const & map){
        	// copy the data
		chunkidVectp = new vector<ChunkID>(*(map.getchunkidVectp()));
	}

	/**
	 * Returns true if cell map is empty
	 */
         bool empty(){
                return chunkidVectp->empty();
         }//end empty()

	/**
	 * This function inserts a new id in the Chunk Id vector. If the id
	 * already exists then it returns false without inserting the id.
	 *
	 * @param	id	the chunk id string
	 */
	 bool insert(const string& id);

	/**
	 * This  routine searches for data points (i.e., chunk ids) in *this CellMap. The
	 * desired data points have coordinates within the ranges defined by the qbox input parameter.
	 * A pointer to a new CellMap containing the found data points is returned.In the returned CellMap
	 * a data point is represented by a chunk id composed of the prefix (input parameter) as a prefix
	 * and the domain corresponding to the data point's cooordinates as a suffix. If no data point is
	 * found  it is returned NULL.
	 *
	 * @param qbox	input parameter defining the rectangle into which the desired data point fall
	 * @param prefix input parameter reoresenting the prefix of the returned chunk ids.
	 */
	CellMap* searchMapForDataPoints(const vector<LevelRange>& qbox, const ChunkID& prefix);


	/**
	 * get/set
	 */
	const vector<ChunkID>* getchunkidVectp() const {return chunkidVectp;}
	void setchunkidVectp(vector<ChunkID>* const chv){chunkidVectp = chv;}

	//const vector<ChunkID>& getchunkidVectp() const {return chunkidVect;}
	//void setchunkidVect(const vector<ChunkID>& chv);


private:
	vector<ChunkID>* chunkidVectp;
	//vector<ChunkID>& chunkidVect;
}; //end of class CellMap


main(){
        //create a CellMap
        CellMap map;
        map.insert(string("5|14.9|30"));
        map.insert(string("5|14.9|33"));
        map.insert(string("5|14.10|30"));
        map.insert(string("5|14.10|31"));
        map.insert(string("5|14.11|32"));
        map.insert(string("5|14.11|33"));
        map.insert(string("5|14.12|29"));
        map.insert(string("5|14.12|31"));
        map.insert(string("5|14.12|34"));
        map.insert(string("5|14.13|32"));
        map.insert(string("5|14.14|31"));
        map.insert(string("5|14.14|33"));
        map.insert(string("5|14.15|31"));
        map.insert(string("5|14.15|32"));
        map.insert(string("5|14.16|30"));

        //create a query box
        vector<LevelRange> qbox(2);
        qbox[0].leftEnd = 0;
        qbox[0].rightEnd = 200;

        qbox[1].leftEnd = 0;
        qbox[1].rightEnd = 300;

//       qbox[2].leftEnd = 200;
//       qbox[2].rightEnd = 300;

        //create a prefix id
        ChunkID prefix(string("5|14.0|0"));

        //call search map
        CellMap* newmapp;
       	try{
       		newmapp = map.searchMapForDataPoints(qbox, prefix);
       	}
 	catch(GeneralError& error){
 		GeneralError e("main ==> ");
 		error += e;
          	cerr << error << endl;
                exit(0);          	
 	}							       		       		       		       		

        if(newmapp) {
                //print new map
                for(vector<ChunkID>::const_iterator citer = newmapp->getchunkidVectp()->begin(); citer != newmapp->getchunkidVectp()->end(); citer++){
                        cout << citer->getcid() << endl;
                }//end for
        }//if
        else
                cout << "No data points found" << endl;
}//main

/**
 * This  routine searches for data points (i.e., chunk ids) in *this CellMap. The
 * desired data points have coordinates within the ranges defined by the qbox input parameter.
 * A pointer to a new CellMap containing the found data points is returned.In the returned CellMap
 * a data point is represented by a chunk id composed of the prefix (input parameter) as a prefix
 * and the domain corresponding to the data point's cooordinates as a suffix. If no data point is
 * found  it is returned NULL.
 *
 * @param qbox	input parameter defining the rectangle into which the desired data point fall
 * @param prefix input parameter reoresenting the prefix of the returned chunk ids.
 */
CellMap* CellMap::searchMapForDataPoints(const vector<LevelRange>& qbox, const ChunkID& prefix)
// precondition:
//      *this is an non-empty CellMap and qbox a non-empty query box and prefix a non-empty Chunk id
// processing:
//      iterate through all data points of Cell Map and check wether they fall into the query box
//postcondition:
//      return pointer to new CellMap with retrieved data points. If no data points found return NULL (i.e.,0)
{
	//assert that CellMap is not empty
	if(empty())
		throw GeneralError("CellMap::searchMapForDataPoints ==> CellMap is empty!\n");
	
	// if qbox is empty
	if(qbox.empty())
	        throw GeneralError("CellMap::searchMapForDataPoints ==> query box is empty!\n");

	//create new cell map
	CellMap* newmapp = new CellMap;

	//for each chunk id stored in the cell map
	for(vector<ChunkID>::const_iterator id_iter = chunkidVectp->begin(); id_iter != chunkidVectp->end(); id_iter++){
		//get suffix domain
		string suffix = id_iter->get_suffix_domain();
		//turn domain to coordinates
		Coordinates c;
		ChunkID::domain2coords(suffix, c);
		//create corresponding cell
		Cell dataPoint(c, qbox);
		//if (cell is within qbox)
		if(dataPoint.cellWithinBoundaries()){ //then this is a data point of interest
			//create new id: add suffix domain to input prefix
			ChunkID newid(prefix);
       			//and add the new domain as suffix
       			try{
       				newid.addSuffixDomain(suffix);
       			}
         		catch(GeneralError& error){
         			GeneralError e("CellMap::searchMapForDataPoints ==> ");
         			error += e;
                          	throw error;
         		}
			//insert new id into new cell map
                	if(!newmapp->insert(newid.getcid())) {
                	        string msg = string("CellMap::searchMapForDataPoints ==> double entry in cell map: ") + newid.getcid();             	
                		throw GeneralError(msg.c_str());
                	}// end if
		}//end if
	}//end for

	//if new cellmap is empty: no data points where found within qbox
	if(newmapp->empty()){
		delete newmapp;//free up space
		return 0;
	}//end if
	else{
		return newmapp;//return pointer to new cell map
	}//end else
	
}// end CellMap::searchMapForDataPoints


// pointer version
bool CellMap::insert(const string& id)
{
	if(id.empty())
		throw GeneralError("Error inside CellMap::insert : empty chunk id!\n");
	// check if the chunk id already exists
	ChunkID newId(id);
	vector<ChunkID>::iterator result = find(chunkidVectp->begin(), chunkidVectp->end(), newId);
	/*#ifdef DEBUGGING
		cerr<<"CellMap::insert ==> result =  "<<result<<", chunk id to insert = "<<newId.getcid()<<endl;
	#endif*/
	if(result == chunkidVectp->end()){
         	// OK its a new one
         	chunkidVectp->push_back(newId);
         	#ifdef DEBUGGING
         		cerr<<"CellMap::insert ==> Just inserted into Cellmap id : "<<chunkidVectp->back().getcid()<<endl;
         	#endif

		return true;
	}
	return false;
}

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

const string ChunkID::get_prefix_domain() const
{
	//if chunk id is empty
	if(empty())
		return string(); //return empty string
		
	// Find character "." in cid starting from left
	string::size_type pos = cid.find(".");
	if (pos == string::npos)
	// If the character does not exist then we have a chunk of chunking depth D = Chunk::MIN_DEPTH
	// and the prefix domain is the string itself
		return cid;
	else
	// This is the normal case (D > Chunk::MIN_DEPTH). The prefix domain is the substring before the first "."
		return cid.substr(0, pos);
}//ChunkID::get_prefix_domain()

const string ChunkID::get_suffix_domain() const
{
	//if chunk id is empty
	if(empty())
		return string(); //return empty string
		
	// Find character "." in cid starting from right
	string::size_type pos = cid.rfind(".");
	if (pos == string::npos)
	// If the character does not exist then we have a chunk of chunking depth D = Chunk::MIN_DEPTH
	// and the suffix domain is the string itself
		return cid;
	else
	// The suffix domain is the substring after the last "."
		return cid.substr(pos+1, string::npos);
}//string ChunkID::get_suffix_domain()
