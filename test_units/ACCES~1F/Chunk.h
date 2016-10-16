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

/**
 * This struct plays the role of the physical id of a bucket. In the current
 * design this is equivalent with the logical id of a shore record.
 * @author: Nikos Karayannidis
 */
typedef unsigned int serial_t;

struct BucketID {
  	/**
	 * SSM Logical Volume id where this record's file resides
	 */
	//lvid_t vid; //you can retrieve this from the SystemManager
  	/**
	 * SSM logical record id
	 */
	serial_t rid;

	/**
	 * Default constructor
	 */
//	BucketID() : vid(), rid(serial_t::null) {}
//	BucketID(const lvid_t& v, const serial_t& r) : vid(v), rid(r) {}
	/**
	 * copy constructor
	 */
//	BucketID(const BucketID& bid) : vid(bid.vid),rid(bid.rid) {}

	/**
	 * Default constructor
	 */
	BucketID() : rid(0) {}
};


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
 * This is a single entry in a Directory Chunk.
 * @author: Nikos Karayannidis
 */
struct DirEntry {
	/**
	 * This is the bucket id that the pointed to chunk resides
	 */
	BucketID bcktId;
	/**
	 * This is the vector index, that we can use in order to access
	 * the pointed to chunk, through the vector of chunks of each bucket.
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
	const int get_chunk_depth() const;
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
	 * @param hdr		chunk header of the dir chunk in question
	 */
	//static unsigned int calcCellOffset(const Coordinates& coords, const ChunkHeader& hdr);
	
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
	 * This function calculates the size (in bytes) for storing a directory chunk
	 * It computes the size and updates the ChunkHeader argument
	 *
	 * @param hdrp	pointer to the ChunkHeader corresponding to the dir. chunk that we want to calc. its size
	 */
	//static void calculateSize(ChunkHeader* hdrp);
	
	//get/set
	const vector<DirEntry>& getentry() const {return entry;}
	void setentry(const vector<DirEntry>& e) { entry = e; }

private:
	/**
	 * vector of directory entries. Num of entries == totNumCells
	 */
	vector<DirEntry> entry;
}; // end of class DirChunk


const int ChunkID::get_chunk_depth() const
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