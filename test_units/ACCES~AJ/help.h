#ifndef HELP_H
#define HELP_H


#include <string>
#include <vector>
#include <strstream>
#include <map>

#include "Exceptions.h"

struct DiskChunkHeader {
	/**
	 * Define the type of an order-code
	 */
        typedef int ordercode_t;

};

class LevelMember {
public:
	static const short PSEUDO_CODE = -1;
};//LevelMember

/**
 * This is a simple range structure over the order-codes of the members of a level
 * @author: Nikos Karayannidis
 */
struct LevelRange {
	// Note: the condition to check for a null range is: ?(leftEnd==rightEnd)
	static const DiskChunkHeader::ordercode_t NULL_RANGE = -1;
	string dimName;
	string lvlName;
	DiskChunkHeader::ordercode_t leftEnd;
	DiskChunkHeader::ordercode_t rightEnd;

	LevelRange(): dimName(""), lvlName(""), leftEnd(NULL_RANGE), rightEnd(NULL_RANGE){}
	LevelRange(const string& dn, const string& ln, DiskChunkHeader::ordercode_t le, DiskChunkHeader::ordercode_t re):
		dimName(dn),lvlName(ln),leftEnd(le),rightEnd(re){}
	/**
	 * assignment operator, because the compiler complains that it cant use
	 * the default operator=, due to the const member
	 */
	LevelRange const& operator=(LevelRange const& other);
	/**
	 * copy constructor
	 */
	 LevelRange::LevelRange(const LevelRange& l);
};//end struct LevelRange

/**
 * This is a simple coordinates structure
 * @author: Nikos Karayannidis
 */
struct Coordinates {
	int numCoords; //number of coordinates
	vector<DiskChunkHeader::ordercode_t> cVect;  // a coordinate is represented by a signed integer, because
	                   // there is also the case of LevelMember::PSEUDO_CODE, who
	                   // can have a negative value (e.g., -1)
	/**
	 * default constructor
	 */
	Coordinates() : numCoords(0), cVect(){}

	/**
	 * constructor
	 */
	Coordinates(int nc, const vector<DiskChunkHeader::ordercode_t>& cds) : numCoords(nc), cVect(cds){}

	/**
	 * copy constructor
	 */
	Coordinates(Coordinates const& c) : numCoords(c.numCoords), cVect(c.cVect) {}

        /**
         * returns true if vector with coordinates is empty
         */
        bool empty() const {return cVect.empty();}
};//end struct Coordinates

/**
 * This class represents a chunk id. i.e. the unique string identifier that is
 * derived from the interleaving of the member-codes of the members of the pivot-set levels
 * that define this chunk.
 * @author: Nikos Karayannidis
 */
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
	 * This operator is required in order to define map containers (see STL)
	 * with a ChunkID as a key.
	 */
	friend bool operator<(const ChunkID& c1, const ChunkID& c2)
        {
        	return (c1.cid < c2.cid);
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
	 * Derive the chunk depth from the chunk id
	 */
	const int getChunkDepth() const;

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

struct ChunkHeader {
	/**
	 * The chunk-id
	 */
	ChunkID	id;

	/**
	 * the chunking (global) depth
	 */
	short depth;

	/**
	 * the chunking local depth, used for resolving the storage of large chunks.
	 */
	short localDepth;

	/**
	 * Flag indicating whether there is another level of chunking following, when
	 * local depth chunking is used.
	 */
	bool nextLocalDepth;

	/**
	 * the number of dimensions
	 */
	unsigned short numDim;

	/**
	 * Vector with order-code ranges on each dimension
	 * ***NOTE*** In the current version the order in which the dimensions appear in
	 * this vector, coincides with the "interleaving order" used for the chunk id construction.
	 * I.e., (dim1,dim2,dim3) in the vectRange means that we  have chunk ids of the
	 * form: dim1|dim2|dim3.dim1|dim2|dim3...
	 * Also note that this order is indirectly specified through the order that the dimensions
	 * appear in the input file *.dld (i.e., the file with the dimension data).
	 */
	vector<LevelRange> vectRange;

	/**
	 * Total number of cells in this chunk, i.e. the cross product of the chunk ranges
	 */
	unsigned short totNumCells;

	/**
	 * Total number of existing cells in this chunk. For DirChunks: rlNumCells == totNumCells
	 * while for DataChunks: rlNumCells <= totNumCells
	 */
	unsigned short rlNumCells;

	/**
	 * size of the chunk in bytes. NOTE: ONLY the chunk NOT its children chunks!
	 * This is the size (in bytes) for storing a dir/data chunk in
	 * a DiskBucket. Therefore, this size corresponds to the structure DiskDataChunk/DiskDirChunk
	 * and NOT the DataChunk/DirChunk.
	 */
	size_t size;

	/**
	 * This pointer points to a vector of maps. Each map corresponds to a dimension and keeps
	 * the 2-level hierarchy, when an artificial chunking takes place, (e.g., in the case of
	 * a large data chunk). The map stores the order-code of the new members and the associated
	 * range in the grain level of each dimension.
	 */
	vector<map<int, LevelRange> >*	artificialHierarchyp;

	/**
	 * default constructor
	 */
	ChunkHeader();
	/**
	 * copy constructor
	 */
	ChunkHeader(const ChunkHeader& h)
		: id(h.id), depth(h.depth), localDepth(h.localDepth), nextLocalDepth(h.nextLocalDepth), numDim(h.numDim), vectRange(h.vectRange), totNumCells(h.totNumCells),
		  rlNumCells(h.rlNumCells), size(h.size), artificialHierarchyp(h.artificialHierarchyp) {}
	~ChunkHeader() {
		if(artificialHierarchyp)
			delete artificialHierarchyp;
	}
};  // end of struct ChunkHeader

class CellMap; //fwd declaration

class Chunk {
public:
	/**
	 * this is the depth at the root chunk level
	 */
	static const unsigned short MIN_DEPTH = 0;

	/**
	 * Definition of NULL local depth.
	 */
	static const short NULL_DEPTH = -1;

	/**
	 * Definition of minimum order code (origin) for a level..
	 */
	static const short MIN_ORDER_CODE = 0;

	/**
	 * Chunk default constructor
	 */
	Chunk(){}

	/**
	 * Chunk constructor
	 */
	Chunk(const ChunkHeader& h) : hdr(h){}

	/**
	 * Copy constructor
	 */
	Chunk(const Chunk & c) : hdr(c.gethdr()) {}

	/**
	 * Chunk destructor
	 */
	virtual ~Chunk(){}

         /**
         * This function scans the input fact file to find prefix matches of the chunk ids in the file
         * with the chunk id of a specific source chunk (dir or data). It then creates a CellMap structure
         * containing the chunk ids (cells of the source chunk) that were located in the file.
         *
         * @param	factFile	the input file with fact values
         * @param	prefix		the prefix (id of source chunk) that we use as a matching pattern - input parameter
         * @param	isDataChunk	a flag indicating whether the prefix corresponds to a data chunk or a directory chunk. - input parameter
         */
         static CellMap* scanFileForPrefix(const string& factFile,const string& prefix, bool isDataChunk = false);

	/** get/set */
	const ChunkHeader& gethdr() const {return hdr;}
	void sethdr(const ChunkHeader& h) {hdr = h;}

protected:
	/**
	 * The chunk header.
	 */
	ChunkHeader hdr;
}; //end of class Chunk

struct BucketID {
        int rid;
};

struct DirEntry {
	/**
	 * This is the bucket id that the pointed to chunk resides
	 */
	BucketID bcktId;
	/**
	 * This index correpsonds to the chunk slot, that we can use in order to access
	 * the pointed to chunk, through the interns; directory of each bucket (see struct DiskBucket).
	 */
	unsigned int chnkIndex;

	/**
	 * default constructor
	 */
	DirEntry() : bcktId(), chnkIndex(0) {}
	/**
	 * constructor
	 */
	DirEntry(BucketID const & b, unsigned int i) : bcktId(b), chnkIndex(i) {}
	/**
	 * copy constructor
	 */
	DirEntry(DirEntry const& e) : bcktId(e.bcktId), chnkIndex(e.chnkIndex) {}
};

typedef float measure_t;

/**
 * This is a single entry in a Data Chunk.
 * @author: nikos Karayannidis
 */
struct DataEntry {
	/**
	 * Number of facts in a cell
	 */
	unsigned int numFacts;
	
	/**
	 * Vector of facts. In the current implementation only floats are supported.
	 */
	vector<measure_t> fact;

	/**
	 * default constructor
	 */
	DataEntry() {}
	/**
	 * constructor
	 */
	DataEntry(unsigned int n, vector<measure_t> const & f) : numFacts(n), fact(f) {}
	/**
	 * copy constructor
	 */
	DataEntry(DataEntry const & e) : numFacts(e.numFacts), fact(e.fact) {}	
};


class DirChunk : public Chunk {
public:
	/**
	 * default constructor
	 */
	DirChunk(){}
	/**
	 * constructor
	 */
	DirChunk(const ChunkHeader& h, const vector<DirEntry>& ent) : Chunk(h), entry(ent) {}
	/**
	 * copy constructor
	 */
	DirChunk(const DirChunk& c) : Chunk(c.gethdr()), entry(c.getentry()) {}

	~DirChunk() {}


	/**
	 * This function calculates the size (in bytes) for storing a directory chunk in
	 * a DiskBucket. Therefore, the structure used for this calculation is the DiskDirChunk
	 * and NOT the DirChunk.
	 *
	 * @param depth		the depth of the chunk in question
	 * @param maxDepth	the maximun chunking depth of the cube in question
	 * @param numDim	the number of dimensions of the cube in question
	 * @param totNumCels	the total No of cells (i.e.,entries) of this chunk
	 *			(empty cells included)
	 * @param local_depth 	the local depth of the chunk in question
	 */
	static size_t calculateStgSizeInBytes(short int depth, short int maxDepth,
					unsigned int numDim, unsigned int totNumCells,
					short int local_depth = Chunk::NULL_DEPTH);

//	/**
//	 * This function calculates the size (in bytes) for storing a directory chunk in
//	 * a DiskBucket. Therefore, the structure used for this calculation is the DiskDirChunk
//	 * and NOT the DirChunk.
//	 * It computes the size and updates the corresponding ChunkHeader member.
//	 *
//	 * @param hdrp	pointer to the ChunkHeader corresponding to the dir. chunk that we want to calc. its size
//	 */
//	static void calculateSize(ChunkHeader* hdrp);

	//get/set
	const vector<DirEntry>& getentry() const {return entry;}
	void setentry(const vector<DirEntry>& e) { entry = e; }

private:
	/**
	 * vector of directory entries. Num of entries == totNumCells
	 */
	vector<DirEntry> entry;
}; // end of class DirChunk

/**
 * This is a data chunk
 * @author: Nikos Karayannidis
 */
class DataChunk : public Chunk {
public:
	/**
	 * default constructor
	 */
	DataChunk() {}

	/**
	 * constructor
	 */
	DataChunk(const ChunkHeader& h, const bit_vector& bmap, const vector<DataEntry>& ent)
		: Chunk(h), comprBmp(bmap),entry(ent) {}
	/**
	 * copy constructor
	 */
	DataChunk(const DataChunk& c)
		: Chunk(c.gethdr()), comprBmp(c.getcomprBmp()), entry(c.getentry()) {}

	~DataChunk(){}


	/**
	 * This function calculates the size (in bytes) for storing a data chunk in
	 * a DiskBucket. Therefore, the structure used for this calculation is the DiskDataChunk
	 * and NOT the DataChunk.
	 *
	 * @param depth		the depth of the chunk in question
	 * @param maxDepth	the maximun chunking depth of the cube in question
	 * @param numDim	the number of dimensions of the cube in question
	 * @param totNumCells	the total No of cells (i.e.,entries) of this chunk
	 *			(empty cells included)
	 * @param rlNumCells	the real number of cells (not including empty cells)
	 * @param numFacts	the number of facts (i.e.,measures) contained inside a single data entry
	 * @param local_depth 	the local depth of the chunk in question
	 */
	static size_t calculateStgSizeInBytes(short int depth, short int maxDepth,
				unsigned int numDim,unsigned int totNumCells, unsigned int rlNumCells,
				unsigned int numfacts, short int local_depth = Chunk::NULL_DEPTH);

//	/**
//	 * This function calculates the size (in bytes) for storing a data chunk in a DiskBucket.
//	 * Therefore, the structure used for this calculation is the DiskDataChunk
//	 * and NOT the DataChunk.
//	 * It computes the size and updates the corresponding ChunkHeader member.
//	 *
//	 * @param hdrp	pointer to the ChunkHeader corresponding to the data. chunk that we want to calc. its size
//	 * @param numfacts  number of facts in a data entry (i.e. a cell)
//	 */
//	static void calculateSize(ChunkHeader* hdrp, unsigned int numfacts);

	//get/set
	const bit_vector& getcomprBmp() const { return comprBmp;}
	void setcomprBmp(const bit_vector &  bmp) {comprBmp = bmp;}
	const vector<DataEntry>& getentry() const {return entry;}
	void setentry(const vector<DataEntry>& e) { entry = e; }

private:
	/**
	 * The compression bitmap is used in order to avoid
	 * a full allocation of the cells of a data chunk and still
	 * be able to compute the offset efficiently.
	 */
	bit_vector comprBmp;

	/**
	 * vector of data entries. Num of entries != totNumCells (== num of NOT NULL cells)
	 */
	vector<DataEntry> entry;
}; // end of class DataChunk


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
	 * overload assignment operator
	 */
	CellMap const& operator=(CellMap const& other);

	/**
	 * Returns true if cell map is empty
	 */
         bool empty() const{
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
	 * desired data points have coordinates within the ranges defined by the box input parameter.
	 * A pointer to a new CellMap containing the found data points is returned.In the returned CellMap
	 * a data point is represented by a chunk id composed of the prefix (input parameter) as a prefix
	 * and the domain corresponding to the data point's cooordinates as a suffix. If no data point is
	 * found  it is returned NULL.
	 *
	 * @param box	input parameter defining the rectangle into which the desired data point fall
	 * @param prefix input parameter reoresenting the prefix of the returned chunk ids.
	 */
	CellMap* searchMapForDataPoints(const vector<LevelRange>& box, const ChunkID& prefix) const;


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


class CubeInfo {

	/**
	 * the maximum chunking depth for this cube
	 */
	unsigned int maxDepth;

	/**
	 * Number of facts in a cell
	 */
	unsigned int numFacts;

public:
	/**
	 * CubeInfo constructor.
	 */
	CubeInfo(){}
	~CubeInfo() {}

	// get/set

	unsigned int getnumFacts() const {return numFacts;}
	void setnumFacts(unsigned int n) {numFacts = n;}

	const unsigned int getmaxDepth() const {return maxDepth;}
	void setmaxDepth(unsigned int d) {maxDepth = d;}

};//CubeInfo




/**
 * This base class represents a chunk cell.
 * @author: Nikos Karayannidis
 */
class Cell {
public:
	Cell(const Coordinates& c, const vector<LevelRange>& b): coords(c), boundaries(b){
	        //if coordinates and boundaries have different dimensionality
	        if(coords.numCoords != boundaries.size())
	                throw GeneralError("Cell::Cell ==> different dimensionality between coordinates and boundaries");
	}//Cell::Cell()

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

/**
 * For output of a cell to ostream objects (e.g. cout, cerr)
 */
ostream& operator<<(ostream& stream, const Cell& cell);

#endif // HELP_H