/***************************************************************************
                          Chunk.h  -  description
                             -------------------
    begin                : Thu Sep 28 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/

#ifndef CHUNK_H
#define CHUNK_H

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <strstream>

//#include <sm_vas.h>
#include "Bucket.h"
#include "DiskStructures.h"
#include "Exceptions.h"
#include "definitions.h"
//#include "AccessManager.h"


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
	LevelRange& operator=(LevelRange const& other);
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
	 * It fills in another Coordinates struct the coordinates that this->cVect contains
	 * excluding the pseudo coordinates (i.e. coordinates that correspond to pseudo-levels)
	 *
	 * @param coords        the initially empty coord struct that will be filled with the coordinate values
	 */
        void excludePseudoCoords(Coordinates & coords)const;	

        /**
         * returns true if vector with coordinates is empty
         */
        bool empty() const {return cVect.empty();}
};//end struct Coordinates

/**
 * This is a single entry in a Directory Chunk.
 * @author: Nikos Karayannidis
 */
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

/**
 * This base class represents a chunk cell.
 * @author: Nikos Karayannidis
 */
class Cell {
public:
	Cell(const Coordinates& c, const vector<LevelRange>& b);
	
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
	 bool isFirstCell() const;

	 /**
	  * Returns true only if the current coordinates of the cell correspond to the
	  * left boundaries of all dimensions. If all coordinates are pseudo codes, it
	  * returns false
	  */	 	
	 bool isLastCell() const;
	
	 /**
	  * Returns true only if *this cell coordinates are within the limits defined
	  * by the boundaries data member. If all coordinates are pseudo codes, it
	  * returns false
	  */
	 bool cellWithinBoundaries() const;
	
	 /**
	  * This routine resets all coordinates from coordIndex  and to the right.
	  * If an a coordinate index value is not provided it resets all coordinates.
	  * "Reset" means to set all coordinates to their corresponding leftEnd boundary.
	  * If input index is out of range a GeneralError exception is thrown.If all coordinates are pseudo codes, it
	  * leaves all coordinates unchanged
	  *
	  * @param startCoordIndex	index of coordinate right of which resetting will take place (itself included)
	  */
	 void reset(int startCoordIndex = 0);
	
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


/**
 * This class represents a cell of a directory chunk.
 * @author: Nikos Karayannidis
 */

/*class DirCell : public Cell {
public:
	DirCell(Coordinates const & c, DirEntry const & dir)
		: Cell(c), dirent(dir){}

	~DirCell() {}

	const DirEntry& getentry() {return dirent;}
	void setentry(const DirEntry& e) {dirent = e;}

private:
  	DirEntry dirent;

};*/

/**
 * This class represents a cell of a data chunk.
 * @author: Nikos Karayannidis
 */

/*class DataCell : public Cell {
public:
	DataCell(Coordinates const & c, DataEntry const & data)
		: Cell(c), dataent(data){}

	~DataCell() {}

	const DataEntry& getentry() {return dataent;}
	void setentry(const DataEntry& e) {dataent = e;}

private:
  	DataEntry dataent;

};*/

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
	 void addSuffixDomain(const string& suffix);
	
	/**
	 * Returns the position in the hierarchy of the levels corresponding to this chunk id, i.e. the PIVOT levels
	 * Position 0 corresponds to the most aggregated level in the hierarchy.
	 */
	unsigned int getPivotLevelPos() const;

	/**
	 * This function retrieves from a chunk id the member code that corresponds to
	 * a specific dimension of the chunk, e.g.15|0|50.23|6|102 => 50.102 (for dim at pos 2 - starting at 0).		
	 *
	 * @param dim_pos	the position of the dimension in the interleaving order in the chunk id == the position of dimension in the CubeInfo vector
	 */
	string extractMbCode(const unsigned int dim_pos) const;
	
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
       	static void coords2domain(const Coordinates& coords, string& domain);
       	
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
	 * Derive the global chunk depth from the chunk id. This is derived from the id's domains
	 * The local dept of this chunk can be passed as an input argument in order not to count
	 * the domains corresponding to artificial chunking (i.e., to local depth and not to global depth)
	 * If no local depth is given then by default local depth is considered Chunk::NULL_DEPTH.
	 * On an empty chunk id it returns -1. Also if an invalid local_depth is given (i.e., one
	 * that will result to an invalid global depth) then -1 is returned again. If the root chunk is
	 * encountered then it returns Chunk::MIN_DEPTH
	 *
	 * NOTE: this routine should NOT be called upon a chunk id of a data cell! Because the global depth of a data cell is undefined
	 * ^^^^
	 */
	const int getChunkGlobalDepth(int local_depth) const; //default value for argument declared after Chunk definition (see further down)
	
	/**
	 * It returns the number of domains in the chunk id. A domain is defined as
	 * the substring between two consecutive dots.If no dots exist then there is
	 * one domain. If the root chunk is encountered then it returns 0. If it
	 * encounters an empty chunk id, it returns -1.
	 */
	const int getNumDomains() const;
	
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

inline void ChunkID::addSuffixDomain(const string& suffix){
	//assert that this suffix is valid: corresponds to the same number of dimensions with the this->cid
	ChunkID testid(suffix);
	bool dummy;
	if(testid.getChunkNumOfDim(dummy) != getChunkNumOfDim(dummy))
		throw GeneralError(__FILE__, __LINE__, "ChunkID::addSuffixDomain ==> suffix and chunk id dimensionality mismatch");
	//add suffix to chunk id
	cid += "." + suffix;
}//addSuffixDomain()

class Chunk; //fwd declaration
/**
 * This is the header of a chunk, containing information about the chunk
 *
 * @author: Nikos Karayannidis
 */
struct ChunkHeader {
public:
	/**
	 * The chunk-id
	 */
	ChunkID	id;

	/**
	 * the chunking (global) depth
	 */
	short int depth;

	/**
	 * the chunking local depth, used for resolving the storage of large chunks.
	 */		
	short int localDepth;
	
	/**
	 * Flag indicating whether there is another level of chunking following, when
	 * local depth chunking is used.
	 */
	bool nextLocalDepth;

	/**
	 * the number of dimensions
	 */
	unsigned short int numDim;

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
	unsigned int totNumCells;

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
		  rlNumCells(h.rlNumCells), size(h.size){
		
		  	if(h.artificialHierarchyp)
		  		artificialHierarchyp = new vector<map<int, LevelRange> >(*h.artificialHierarchyp);
		  	else
		  		artificialHierarchyp = 0;
	}
		
	~ChunkHeader() {
		if(artificialHierarchyp)
			delete artificialHierarchyp;
	}
	
       	/**
       	 * Assignment operator
       	 */
       	ChunkHeader & operator=(const ChunkHeader & other);					
		
	/**
	 * This method receives an empty vector of level ranges and fills it with the ranges
	 * included in sourcevectRange, in the same order,  WITHOUT the NULL ranges.
	 *
	 * @param sourcevectRange	source range vector - input parameter
	 * @param newvectRange	the initially empty vector of ranges that will be filled - output parameter
	 */
	static void excludeNULLRanges(const vector<LevelRange>& sourcevectRange, vector<LevelRange>& newvectRange);		

};  // end of struct ChunkHeader

class CubeInfo; //fwd declaration
class CostNode;
class CellMap;
/**
 * This is a base class representing a chunk.
 * @author: Nikos Karayannidis
 */
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
       	 * Assignment operator
       	 */
       	Chunk& operator=(const Chunk& other);				
	
	/**
	 * virtual method that returns the offset (i.e. index in the chunk vector of entries)
         * in order to access the cell.
	 */
	//virtual unsigned int calcCellOffset(const Coordinates& coords) {return 0;}
	/**
	 * virtual method that returns the contents of a Cell
	 */
	//virtual void readCellEntry()=0;
	/**
	 * pure virtual method that updates the contents of a Cell
	 */
	//virtual void updateCellEntry()=0;
	/**
	 * pure virtual method that retrieves a pointer to a cell
	 */
	//virtual void getCellp()=0;

	/**
	 * This function initializes the members of the root chunk's header
	 *
	 * @param rootHdrp	The root chunk header pointer
	 * @param info		The CubeInfo instance of the parent cube of this chunk
	 */
	static void createRootChunkHeader(ChunkHeader* rootHdrp, const CubeInfo& info);

	/**
	 * This function initializes the members of the chunk's header
	 *
	 * @param rootHdrp	The root chunk header pointer
	 * @param cinfo		The CubeInfo instance of the parent cube of this chunk
	 * @param chunkid	The chunk id of the chunk
	 */
	static void createChunkHeader(ChunkHeader* hdrp, const CubeInfo& cinfo, const ChunkID& chunkid);

	/**
	 * This function scans the input file to locate the existing cells of the input
	 * chunk and creates a CellMap structure. It also updates the ChunkHeader with the
	 * size of this chunk and the number of existing cells. It packs it all up in a new
	 * CostNode instance and then it recursively calls createCostTree for each one of the existing
	 * cells (child chunks). It returns a CostNode, which is the root of a costNode tree.
	 *
	 * @param chunkHeader	the header of the chunk we wish to expand.
	 * @param cbinfo		The CubeInfo instance of the parent cube of this chunk
	 * @param factFile	the file with the input fact data. We assume that each line contains a chunk id and one or more values.
	 */
	static CostNode* createCostTree(ChunkHeader* chunkHdrp, const CubeInfo& cbinfo, const string& factFile);

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
	//void sethdr(const ChunkHeader& h) {hdr = h;}

protected:
	/**
	 * The chunk header.
	 */
	ChunkHeader hdr;	
}; //end of class Chunk

// declaration of default argument value is put here, because the definition of Chunk is required.
// I use the "extern" keyword to denote that this is just a declaration and not a definition.
const int ChunkID::getChunkGlobalDepth(int localdepth = Chunk::NULL_DEPTH) const;

/**
 * This is a directory chunk
 * @author: Nikos Karayannidis
 */
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
	 * static method for calculating the cell offset within a DirChunk. Returns the offset
	 * in the range [0..totNumCells-1]
	 *
	 * @param coords	input set of coordinates
	 * @param vectRange	vector of order code ranges corresponding to the dir chunk - input parameter
	 */
	static unsigned int calcCellOffset(const Coordinates& coords, const vector<LevelRange>& vectRange);//const ChunkHeader& hdr);
	
	/**
 	 * virtual method for reading the value of a single cell.
	 */
	//const DirEntry& readCellEntry(unsigned int offset) const;
	/**
 	 * virtual method for updating the value of a single cell.
	 */
	//void updateCellEntry(unsigned int offset, const DirEntry& ent);
	/**
 	 * virtual method for getting a pointer to a single Cell
	 */
	//const DirCell getCell(unsigned int offset) const;

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
	 * @param nextLDflag	next local depth flag, used for artificially chunked dir chunks
	 *			to indicate existence of a child chunk.
	 * @param noMembers	array of size numDim storing the number of newly inserted members
	 *			for each dimension, for an artificially chunked dir chunk
	 */	
	static size_t calculateStgSizeInBytes(int depth, unsigned int maxDepth,
					unsigned int numDim, unsigned int totNumCells,
					int local_depth = Chunk::NULL_DEPTH,
					bool nextLDflag = false,
					const unsigned int* const noMembers = 0);
	
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
	
       	/**
       	 * Assignment operator declared private and left undefined in order to prevent its use.
       	 * (see Effective C++ Item 27)
       	 */
       	//DirChunk& operator=(const DirChunk& other);				
	
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
	DataChunk(const ChunkHeader& h, const deque<bool>& bmap, const vector<DataEntry>& ent)
		: Chunk(h), comprBmp(bmap),entry(ent) {}
	/**
	 * copy constructor
	 */
	DataChunk(const DataChunk& c)
		: Chunk(c.gethdr()), comprBmp(c.getcomprBmp()), entry(c.getentry()) {}

	~DataChunk(){}
		
	/**
	 * Static method for calculating the cell offset within a DataChunk. Returns the offset
	 * in the range [0..realNumCells-1]
	 *
	 * @param coords	input set of coordinates
	 * @param bmp		input compression bitmap
	 * @param vectRange	vector of order code ranges corresponding to the data chunk - input parameter	
	 * @param isEmpty	returned flag. Set on when requested cell is empty (i.e.,0 bit in bitmap)
	 */
	static unsigned int calcCellOffset(const Coordinates& coords, const deque<bool>& bmp,
			const vector<LevelRange>& hdr, bool& isEmpty);
	
	/**
 	 * virtual method for reading the value of a single cell.
	 */
	//const DataEntry& readCellEntry(unsigned int offset) const;
	/**
 	 * virtual method for updating the value of a single cell.
	 */
	//void updateCellEntry(unsigned int offset, const DataEntry& ent);
	/**
 	 * virtual method for getting a pointer to a single Cell
	 */
	//const DataCell getCell(unsigned int offset) const;

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
	 * @param nextLDflag	next local depth flag, used for artificially chunked data chunks
	 *			to indicate existence of a child chunk.	
	 */			
	static size_t calculateStgSizeInBytes(int depth, unsigned int maxDepth,
				unsigned int numDim,unsigned int totNumCells, unsigned int rlNumCells,
				unsigned int numfacts, int local_depth = Chunk::NULL_DEPTH,
									bool nextLDflag = false);	

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
	const deque<bool>& getcomprBmp() const { return comprBmp;}
	void setcomprBmp(const deque<bool> &  bmp) {comprBmp = bmp;}
	const vector<DataEntry>& getentry() const {return entry;}
	void setentry(const vector<DataEntry>& e) { entry = e; }

private:
	/**
	 * The compression bitmap is used in order to avoid
	 * a full allocation of the cells of a data chunk and still
	 * be able to compute the offset efficiently.
	 * NOTE: the use of bit_vector (i.e., vector<bool>) is depreciated
	 * (@see Effective STL item 18) and bitset or deque<bool> proposed as
	 * an alternative. However the compiler required by SSM
	 * (gcc version egcs-2.91.66 19990314 (egcs-1.1.2 release)) does not
	 * support bitset, therefore we go for the latter.
	 */
	 deque<bool> comprBmp;
	//bit_vector comprBmp;

	/**
	 * vector of data entries. Num of entries != totNumCells (== num of NOT NULL cells)
	 */
	vector<DataEntry> entry;
       	
       	/**
       	 * Assignment operator declared private and left undefined in order to prevent its use.
       	 * (see Effective C++ Item 27)
       	 */
       	//DataChunk& operator=(const DataChunk& other);				
	
}; // end of class DataChunk




#endif // CHUNK_H