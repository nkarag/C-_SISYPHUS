#ifndef CHUNK_H
#define CHUNK_H

#include <vector>

#include "help.h"

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

#endif // CHUNK_H