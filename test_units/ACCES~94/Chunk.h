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
#include "Bucket.h"
#include "defintion.h"
//#include "AccessManager.h"


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
	/**
	 * assignment operator, because the compiler complains that it can use
	 * the default, due to the const member
	 */
	LevelRange const& operator=(LevelRange const& other);
	/**
	 * copy constructor
	 */
	 LevelRange::LevelRange(const LevelRange& l);
};

/**
 * This is a simple coordinates structure
 * @author: Nikos Karayannidis
 */
struct Coordinates {
	int numCoords; //number of coordinates
	vector<int> cVect;		
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
};


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
	Cell(const Coordinates& c)
		: coords(c){}
	virtual ~Cell() {}

	//get/set
	const Coordinates& getcoords() const {return coords;}
	void setcoords(const Coordinates&  c) {coords = c;}
protected:
	Coordinates coords;
};

/**
 * This class represents a cell of a directory chunk.
 * @author: Nikos Karayannidis
 */

class DirCell : public Cell {
public:
	DirCell(Coordinates const & c, DirEntry const & dir)
		: Cell(c), dirent(dir){}

	~DirCell() {}

	const DirEntry& getentry() {return dirent;}
	void setentry(const DirEntry& e) {dirent = e;}

private:
  	DirEntry dirent;
};

/**
 * This class represents a cell of a data chunk.
 * @author: Nikos Karayannidis
 */

class DataCell : public Cell {
public:
	DataCell(Coordinates const & c, DataEntry const & data)
		: Cell(c), dataent(data){}

	~DataCell() {}

	const DataEntry& getentry() {return dataent;}
	void setentry(const DataEntry& e) {dataent = e;}

private:
  	DataEntry dataent;
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

	friend bool operator==(const ChunkID& c1, const ChunkID& c2);
	
	friend bool operator<(const ChunkID& c1, const ChunkID& c2);
	
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
	void extractCoords(Coordinates& c);

	// get/set chunk id
	const string& getcid() const { return cid; }
	void setcid(const string& id) { cid = id; }

	// get prefix domain of chunk id
	const string get_prefix_domain() const;

	// get suffix domain of chunk id
	const string get_suffix_domain() const;

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
		: depth(h.depth), numDim(h.numDim), vectRange(h.vectRange), totNumCells(h.totNumCells),
		  rlNumCells(h.rlNumCells), size(h.size) {}
	~ChunkHeader() {}
		
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
	static const unsigned int MIN_DEPTH = 1;
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
	 * CostNode instance and then it recursively calls expandchunk for each one of the existing
	 * cells (child chunks). It returns a CostNode, which is the root of a costNode tree.
	 *
	 * @param chunkHeader	the header of the chunk we wish to expand.
	 * @param cbinfo		The CubeInfo instance of the parent cube of this chunk
	 * @param factFile	the file with the input fact data. We assume that each line contains a chunk id and one or more values.
	 */
	static CostNode* expandChunk(ChunkHeader* chunkHdrp, const CubeInfo& cbinfo, const string& factFile);

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
	 * virtual method that returns the offset (i.e. index in the chunk vector of entries)
         * in order to access the cell.
	 */
	static unsigned int calcCellOffset(const Coordinates& coords);
	/**
 	 * virtual method for reading the value of a single cell.
	 */
	const DirEntry& readCellEntry(unsigned int offset) const;
	/**
 	 * virtual method for updating the value of a single cell.
	 */
	void updateCellEntry(unsigned int offset, const DirEntry& ent);
	/**
 	 * virtual method for getting a pointer to a single Cell
	 */
	const DirCell getCell(unsigned int offset) const;

	/**
	 * This function calculates the size (in bytes) for storing a directory chunk
	 * It computes the size and updates the ChunkHeader argument
	 *
	 * @param hdrp	pointer to the ChunkHeader corresponding to the dir. chunk that we want to calc. its size
	 */
	static void calculateSize(ChunkHeader* hdrp);
	
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
	DataChunk(const ChunkHeader& h, const vector<DataEntry>& ent, const bit_vector& bmap)
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
	static unsigned int calcCellOffset(const Coordinates& coords);
	
	static unsigned int calcCellOffset(const Coordinates& coords, const bit_vector& bmp);
	/**
 	 * virtual method for reading the value of a single cell.
	 */
	const DataEntry& readCellEntry(unsigned int offset) const;
	/**
 	 * virtual method for updating the value of a single cell.
	 */
	void updateCellEntry(unsigned int offset, const DataEntry& ent);
	/**
 	 * virtual method for getting a pointer to a single Cell
	 */
	const DataCell getCell(unsigned int offset) const;

	/**
	 * This function calculates the size (in bytes) for storing a data chunk
	 * It computes the size and updates the ChunkHeader argument
	 *
	 * @param hdrp	pointer to the ChunkHeader corresponding to the data. chunk that we want to calc. its size
	 * @param numfacts  number of facts in a data entry (i.e. a cell)
	 */
	static void calculateSize(ChunkHeader* hdrp, unsigned int numfacts);

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




#endif // CHUNK_H