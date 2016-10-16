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

//#include <sm_vas.h>
//#include "Bucket.h"
//#include "defintion.h"
//#include "AccessManager.h"

//the type of a measure value
typedef float measure_t;

/**
 * This is a simple range structure over the order-codes of the members of a level
 * @author: Nikos Karayannidis
 */
struct LevelRange {
	// Note: the condition to check for a null range is: ?(leftEnd==rightEnd)
	static const int NULL_RANGE;
	string dimName;
	string lvlName;
	unsigned int leftEnd;
	unsigned int rightEnd;
	//LevelRange() : NULL_RANGE(0) {}
	LevelRange(){}
	LevelRange(const string& dn, const string& ln, unsigned int le, unsigned int re):
		dimName(dn),lvlName(ln),leftEnd(le),rightEnd(re){}
	/**
	 * assignment operator, because the compiler complains that it cant use
	 * the default, due to the const member
	 */
	LevelRange const& operator=(LevelRange const& other);
	/**
	 * copy constructor
	 */
	 LevelRange::LevelRange(const LevelRange& l);
};

const int LevelRange::NULL_RANGE=-1;

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
	friend bool operator==(const ChunkID& c1, const ChunkID& c2);

	/**
	 * This operator is required in order to define map containers (see STL)
	 * with a ChunkID as a key.
	 */	
	friend bool operator<(const ChunkID& c1, const ChunkID& c2);
	
	/**
	 * Returns the position in the hierarchy of the levels corresponding to this chunk id, i.e. the PIVOT levels
	 * Position 0 corresponds to the most aggregated level in the hierarchy.
	 */
	//unsigned int getPivotLevelPos() const;

	/**
	 * This function retrieves from a chunk id the member code that corresponds to
	 * a specific dimension of the chunk, e.g.15|0|50.23|6|102 => 50.102 (for dim at pos 2 - starting at 0).		
	 *
	 * @param dim_pos	the position of the dimension in the interleaving order in the chunk id == the position of dimension in the CubeInfo vector
	 */
	//string extractMbCode(const unsigned int dim_pos) const;
	
	/**
	 * This function updates a Coordinates struct with the cell
	 * coordinates that extracts from the last domain of a chunk id.
	 *
	 * @param	c	the Coordinates struct to be updated
	 */
	//void extractCoords(Coordinates& c) const;

	// get/set chunk id
	const string& getcid() const { return cid; }
	void setcid(const string& id) { cid = id; }

	// get prefix domain of chunk id
	//const string get_prefix_domain() const;

	// get suffix domain of chunk id
	//const string get_suffix_domain() const;

	// get chunk depth
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
};


/**
 * This is the header of a chunk, containing information about the chunk
 *
 * @author: Nikos Karayannidis
 */
struct ChunkHeader {
	/**
	 * The chunk-id
	 */
	ChunkID	id;

	/**
	 * the chunking depth
	 */
	unsigned int depth;

	/**
	 * the number of dimensions
	 */
	unsigned int numDim;

	/**
	 * Vector with order-code ranges on each dimension
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
	unsigned int rlNumCells;

	/**
	 * size of the chunk in bytes. NOTE: ONLY the chunk NOT its children chunks!
	 */
	unsigned int size;
	
	/**
	 * default constructor
	 */
	ChunkHeader() {}
	/**
	 * copy constructor
	 */
	ChunkHeader(const ChunkHeader& h)
		: id(h.id), depth(h.depth), numDim(h.numDim), vectRange(h.vectRange), totNumCells(h.totNumCells),
		  rlNumCells(h.rlNumCells), size(h.size) {}
	~ChunkHeader() {}
	
	/**
	 * This method receives an empty vector of level ranges and fills it with the ranges
	 * included in this->vectRange, in the same order,  WITHOUT the NULL ranges.
	 *
	 * @param newvectRange	the initially empty vector of ranges that will be filled
	 */
	//void excludeNULLRanges(vector<LevelRange>& newvectRange)const;		
};  // end of struct ChunkHeader

//class CubeInfo; //fwd declaration
//class CostNode;
//class CellMap;
/**
 * This is a base class representing a chunk.
 * @author: Nikos Karayannidis
 */
class Chunk {
public:
	/**
	 * this is the depth at the root chunk level
	 */
	static const unsigned int MIN_DEPTH = 0;
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
	//static void createRootChunkHeader(ChunkHeader* rootHdrp, const CubeInfo& info);

	/**
	 * This function initializes the members of the chunk's header
	 *
	 * @param rootHdrp	The root chunk header pointer
	 * @param cinfo		The CubeInfo instance of the parent cube of this chunk
	 * @param chunkid	The chunk id of the chunk
	 */
	//static void createChunkHeader(ChunkHeader* hdrp, const CubeInfo& cinfo, const ChunkID& chunkid);

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
	//static CostNode* createCostTree(ChunkHeader* chunkHdrp, const CubeInfo& cbinfo, const string& factFile);

	/**
	 * This function scans the input fact file to find prefix matches. We assume that the fact file
	 * contains in each line: <cell chunk id>\t<cell value1>\t<cell value2>...<\t><cell valueN>
	 * For directory chunks when we find a match, then we insert in a CellMap the prefix+the "next domain"
	 * from the chunk-id that matched. All following matches with this "next domain" are ignored
	 *                                                       -----------------------
	 * (i.e. no insertion takes place).
	 * For data chunks if we do find a second match with the same "next domain" following the prefix, then
	 * we throw an exception, since that would mean that we have found values for the same cell, more
	 * than once in the input file.
	 *
	 * @param	factFile	the input file with fact values
	 * @param	prefix		the prefix that we use as a matching pattern
	 * @param	isDataChunk	a flag indicating whether the prefix corresponds to a data chunk or a directory chunk.
	 */
	//static CellMap* scanFileForPrefix(const string& factFile,const string& prefix, bool isDataChunk = false);

	/** get/set */
	const ChunkHeader& gethdr() const {return hdr;}
	void sethdr(const ChunkHeader& h) {hdr = h;}

protected:
	/**
	 * The chunk header.
	 */
	ChunkHeader hdr;
}; //end of class Chunk

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
	DataChunk(const ChunkHeader& h, const bit_vector& bmap, const vector<DataEntry>& ent )
		: Chunk(h), comprBmp(bmap),entry(ent) {}
	/**
	 * copy constructor
	 */
	DataChunk(const DataChunk& c)
		: Chunk(c.gethdr()), comprBmp(c.getcomprBmp()), entry(c.getentry()) {}

	~DataChunk(){}
	
	/**
	 * virtual method that returns the offset (i.e. index in the chunk vector of entries)
         * in order to access the cell.
	 */
	//unsigned int calcCellOffset(const Coordinates& coords);
	
	/**
	 * Static method for calculating the cell offset within a DataChunk. Returns the offset
	 * in the range [0..realNumCells-1]
	 *
	 * @param coords	input set of coordinates
	 * @param bmp		input compression bitmap
	 * @param hdr		the chunk header of the data chunk in question
	 * @param isEmpty	returned flag. Set on when requested cell is empty (i.e.,0 bit in bitmap)
	 */
	//static unsigned int calcCellOffset(const Coordinates& coords, const bit_vector& bmp,
	//		const ChunkHeader& hdr, bool& isEmpty);
	
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
	 * This function calculates the size (in bytes) for storing a data chunk
	 * It computes the size and updates the ChunkHeader argument
	 *
	 * @param hdrp	pointer to the ChunkHeader corresponding to the data. chunk that we want to calc. its size
	 * @param numfacts  number of facts in a data entry (i.e. a cell)
	 */
	//static void calculateSize(ChunkHeader* hdrp, unsigned int numfacts);

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


const int ChunkID::getChunkDepth() const
{
	if(cid == "root")
		return Chunk::MIN_DEPTH;

	int depth = Chunk::MIN_DEPTH + 1;
	for (int i = 0; i<cid.length(); i++)
	{
		if (cid[i] == '.')
			depth++;
	}
	return depth;
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

LevelRange::LevelRange(const LevelRange& l)
	: dimName(l.dimName),lvlName(l.lvlName),leftEnd(l.leftEnd),rightEnd(l.rightEnd)
{
}
LevelRange const& LevelRange::operator=(LevelRange const& other)
{
	dimName = other.dimName;
	lvlName = other.lvlName;
	leftEnd = other.leftEnd;
	rightEnd = other.rightEnd;
	return (*this);
}

#endif // CHUNK_H