#ifndef CLASSES_H
#define CLASSES_H

#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <map>
#include <algorithm>
#include <queue>

typedef int serial_t;
enum TreeTraversal_t {depthFirst,breadthFirst};

//***************** class definitions ***************************//

/**
 * This struct plays the role of the physical id of a bucket. In the current
 * design this is equivalent with the logical id of a shore record.
 * @author: Nikos Karayannidis
 */
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
	BucketID(const serial_t& r) : rid(r) {}
	/**
	 * copy constructor
	 */
	BucketID(const BucketID& bid) : rid(bid.rid) {}

	/**
	 * Check for a null bucket id
	 */
	bool isnull() const {return (rid == 0);}

	friend bool operator==(const BucketID& b1, const BucketID& b2) {
	 	return (b1.rid == b2.rid);
	}

	friend bool operator!=(const BucketID& b1, const BucketID& b2) {
	 	return (b1.rid != b2.rid);
	}

	friend bool operator<(const BucketID& b1, const BucketID& b2) {
	 	return (b1.rid < b2.rid);
	}

	static BucketID createNewID();				
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

	LevelRange(): dimName(""), lvlName(""), leftEnd(NULL_RANGE), rightEnd(NULL_RANGE){}
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

	/**
	 * It fills in another Coordinates struct the coordinates that this->cVect contains
	 * excluding the pseudo coordinates (i.e. coordinates that correspond to pseudo-levels)
	 *
	 * @param coords        the initially empty coord struct that will be filled with the coordinate values
	 */
        void excludePseudoCoords(Coordinates & coords)const;
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
	 * This function updates a Coordinates struct with the cell
	 * coordinates that extracts from the last domain of a chunk id.
	 *
	 * @param	c	the Coordinates struct to be updated
	 */
	void extractCoords(Coordinates& c) const;

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
	
	// get/set chunk id
	const string& getcid() const { return cid; }
	void setcid(const string& id) { cid = id; }
}; //ChunkID

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
	CellMap();
	~CellMap();

	/**
	 * copy constructor
	 */
	CellMap(CellMap const & map);

	/**
	 * overload assignment operator
	 */
	CellMap const& operator=(CellMap const& other);

	/**
	 * This function inserts a new id in the Chunk Id vector. If the id
	 * already exists then it returns false
	 *
	 * @param	id	the chunk id string
	 */
	 bool insert(const string& id);

	/**
	 * get/set
	 */
	const vector<ChunkID>* getchunkidVectp() const {return chunkidVectp;}
	void setchunkidVectp(vector<ChunkID>* const chv);
	//const vector<ChunkID>& getchunkidVectp() const {return chunkidVect;}
	//void setchunkidVect(const vector<ChunkID>& chv);


private:
	vector<ChunkID>* chunkidVectp;
}; //CellMap


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
	unsigned int rlNumCells;

	/**
	 * size of the chunk in bytes. NOTE: ONLY the chunk NOT its children chunks!
	 */
	size_t size;
	
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
	void excludeNULLRanges(vector<LevelRange>& newvectRange)const;		
};  // end of struct ChunkHeader

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
	static unsigned int calcCellOffset(const Coordinates& coords, const ChunkHeader& hdr);
	
	static size_t calculateStgSizeInBytes(unsigned int depth, unsigned int maxDepth,
					unsigned int numDim, unsigned int totNumCells);	

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
	 * virtual method that returns the offset (i.e. index in the chunk vector of entries)
         * in order to access the cell.
	 */
	unsigned int calcCellOffset(const Coordinates& coords);

       	/**
	 * Static method for calculating the cell offset within a DataChunk. Returns the offset
	 * in the range [0..realNumCells-1]
	 *
	 * @param coords	input set of coordinates
	 * @param bmp		input compression bitmap
	 * @param hdr		the chunk header of the data chunk in question
	 * @param isEmpty	returned flag. Set on when requested cell is empty (i.e.,0 bit in bitmap)
	 */
	static unsigned int calcCellOffset(const Coordinates& coords, const bit_vector& bmp, const ChunkHeader& hdr, bool& isEmpty);

	static size_t calculateStgSizeInBytes(unsigned int depth, unsigned int maxDepth,
				unsigned int numDim,unsigned int totNumCells, unsigned int rlNumCells,
				unsigned int numfacts);

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
 * This class represents a "chunk node" in the chunk hierarchy. It contains chunk header information
 * as well as a CellMap indicating which cells have NOT NULL values
 *
 * @author Nikos Karayannidis
 */
class CostNode {
public:
	/**
	 * Default constructor
	 */
	 CostNode() : chunkHdrp(0),cMapp(0),child(){}
	//CostNode() : chunkHdrp(0),cMapp(0),child(0){}
	/**
	 * constructor
	 */
	 CostNode(ChunkHeader* const hdr, CellMap* const map): chunkHdrp(hdr),cMapp(map),child() {}
	 //CostNode(ChunkHeader* const hdr, CellMap* const map, vector<CostNode>* const c): chunkHdrp(hdr),cMapp(map),child(c) {}
	~CostNode();
	/**
	 * overloaded assignment operator
	 */
	const CostNode & operator=(const CostNode & other);

	static void countDirChunksOfTree(const CostNode* const costRoot,
					 unsigned int maxdepth,
					 unsigned int& total);
					
	static  void countDataChunksOfTree(const CostNode* const costRoot,
						unsigned int maxdepth,
						unsigned int& total);
						
	static void countChunksOfTree(const CostNode* const costRoot,
					unsigned int maxdepth,
					unsigned int& total);

        static void printTree(CostNode* root, ofstream& out);
	/**
	 * get/set
	 */
	const ChunkHeader* const getchunkHdrp() const {return chunkHdrp;}
	void setchunkHdrp(ChunkHeader* const chdr) {chunkHdrp = chdr;}

	const CellMap* const getcMapp() const {return cMapp;}
	void setcMapp(CellMap* const map) {cMapp = map;}

	const vector<CostNode*>& getchild() const {return child;}
	void setchild(const vector<CostNode*> & chld) {child = chld;}
	//const vector<CostNode>*const getchild() const {return child;}
	//void setchild(vector<CostNode>* const chld) {child = chld;}

private:
	/**
	 * pointer to chunk header
	 */
	ChunkHeader* chunkHdrp;
	/**
	 * pointer to CellMap
	 */
	CellMap* cMapp;
	/**
	 * pointers to children nodes. Note: the order of the childs is identical
	 * to the one in the cMapp.
	 */
	 //vector<CostNode>* child;
	vector<CostNode*> child;
}; //end class CostNode

class Level_Member{
public:
	enum {PSEUDO_CODE = -1};
};

#endif // CLASSES_H