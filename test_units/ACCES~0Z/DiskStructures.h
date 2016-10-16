/***************************************************************************
                          DiskStructures.h  -  Structures for disk storage of a bucket
                             -------------------
    begin                : Thu Jun 7 2001
    copyright            : (C) 2001 by Nikos Karayannidis
    email                : 
 ***************************************************************************/

#ifndef DISK_STRUCTURES_H
#define DISK_STRUCTURES_H

//#include <sm_vas.h>
//#include <map>
//#include <pair.h>
//#include <vector>
//#include <string>
#include <new>
#include <cmath>

#include "classes.h"
#include "definitions.h"

typedef unsigned int WORD; // a bitmap will be represented by an array of WORDS

/**
 * Number of bits per word, e.g., per unsigned integer, if the
 * bitmap is represented by an array of unsigned integers
 */
const unsigned int BITSPERWORD = sizeof(WORD)*8;

/**
 * logarithm of base 2
 */
inline double log2(double x) {
        return log(x)/log(2);
}

/**
 * Used in order to locate the WORD in which a bit has been stored
 */
const unsigned int SHIFT = static_cast<unsigned int>(ceil(log2(BITSPERWORD)));


/**
 * Returns the number of words needed to stored a bitmap
 * of size no_bits bits
 */
inline unsigned int numOfWords(unsigned int no_bits) {
	//return no_bits/BITSPERWORD + 1;
	return static_cast<unsigned int>(ceil(double(no_bits)/double(BITSPERWORD)));
}

/**
 * This mask is used in order to isolate the number of LSBits from an integer i (representing a
 * bit position in a bitmap) that correspond to the position of the bit within a WORD.
 *			                     			        ~~~~~~
 */
inline WORD create_mask(){
        WORD MASK = 1;
        // turn on the #SHIFT LSBits
        for(int b = 1; b<SHIFT; b++){
                MASK |= (1 << b);
        }
        return MASK;
}


/**
 * This structure implements the header of a chunk (dir or data chunk).
 *
 * @author Nikos Karayannidis
 */
struct DiskChunkHeader {
	/**
	 * Define the type of an order-code
	 */
        typedef short ordercode_t;

        typedef struct CidDomain{
        	/**
        	 * pointer to this domain's order-codes
        	 */
        	ordercode_t* ordercodes;
        	
        	CidDomain():ordercodes(0){}
        	~CidDomain() { delete [] ordercodes;}
        } Domain_t;
       	
       	/**
	 * Define the NULL range.
	 * Note: the condition to check for a null range is: ?(leftEnd==rightEnd)
	 */
        enum {null_range = -1};

       	/**
	 * Define the type of an order-code range
	 */
        typedef struct OcRng{
		ordercode_t left;
		ordercode_t right;
		
		OcRng():left(null_range),right(null_range){}
        	} 	OrderCodeRng_t;
        		
	/**
	 * This chunk's chunking depth. Also denotes the length of the outermost
	 * vector of the chunk id (see further on)
	 */
	char depth;

	/**
	 * The number of dimensions of the chunk. Also denotes the length of the innermost
	 * vector of the chunk id and the length of the order-code range vector (see further on)
	 */
	char no_dims;
			
	/**
	 * The number of measures contained inside the cell of this chunk. If this
	 * is a directory chunk then = 0.
	 */		
	char no_measures;
	
	/**
	 * The number of entries of this chunk. If this is a data chunk
	 * then this is the total number of entries (empty cells included), i.e.
	 * it gives us the length of the bitmap.
	 */
        short unsigned int no_entries;
        	        	
	/**
	 * A chunk id is of the form: oc|oc...|oc[.oc|oc...|oc]...,
	 * where "oc" is an order-code corresponding to a specific dimension
	 * and a specific level within this dimension.
	 * We implement the chunk id with a vector of vectors of order-codes.
	 * The outmost vector corresponds to the different domains (i.e. depths)
	 * of the chunk id, while the innermost corresponds to the different dimensions.
	 * This heap structure is laid out on a byte array order-code by order-code prior to
	 * disk storage. Therefore,in order to read it back, special offset computing
	 * routines are needed.
	 */
	//vector<vector<ordercode_t> > chunk_id;
	//ordercode_t** chunk_id;
	Domain_t* chunk_id;
	
	/**
	 * This vector contains a range of order-codes for each dimension level of the
	 * depth of this chunk.Therefore the length is equal with the number of dimensions.
	 * This range denotes the order-codes covered on each dimension
	 * from this chunk.
	 * This heap structure is laid out on a byte array range by range prior to
	 * disk storage. Therefore,in order to read it back, special offset computing
	 * routines are needed.	
	 */
	 //vector<OrderCodeRng_t> oc_range;
	 OrderCodeRng_t* oc_range;
	 /**
	  * Default constructor
	  */
	 DiskChunkHeader(): chunk_id(0),oc_range(0) {}
	
	 /**
	  * copy constructor
	  */
	  DiskChunkHeader(const DiskChunkHeader& h):
	  	depth(h.depth),no_dims(h.no_dims),no_measures(h.no_measures),no_entries(h.no_entries),
	  	chunk_id(h.chunk_id),oc_range(h.oc_range) {}
	
	  /*
	   * Destructor. It has to free space pointed to by the chunk id
	   * and oc_range arrays
	   */
	 ~DiskChunkHeader() {
	 	/*for(int d = 1; d<=depth; i++){
	 		delete [] chunk_id[d-1];
	 	} */
	 	delete [] chunk_id;
	 	delete [] oc_range;
	 }	
}; //end of DiskChunkHeader

struct DiskDirChunk {
	/**
	 * Define the type of a directory chunk entry.
	 */
        typedef struct Entry {
        	BucketID bucketid;
        	unsigned short chunk_slot;} DirEntry_t;

	/**
	 * The chunk header
	 */        		
	DiskChunkHeader	hdr;
	
	/**
	 * Vector of entries.
	 * This heap structure is laid out on a byte array entry by entry prior to
	 * disk storage. Therefore,in order to read it back, special offset computing
	 * routines are needed.	
	 */
	//vector<DirEntry_t> entry; 	
	DirEntry_t* entry;
	
	/**
	 * default constructor
	 */
	DiskDirChunk():entry(0){}
	
	/**
	 * constructor
	 */
	 DiskDirChunk(const DiskChunkHeader& h): hdr(h), entry(0){}
	
	 /**
	  * constructor
	  */
	 DiskDirChunk(const DiskChunkHeader& h, DirEntry_t* ep): hdr(h), entry(ep){}
	
	 /**
	  * Destructor. Free up space from the entry array
	  */
	~DiskDirChunk() { delete [] entry;}
};

struct DiskDataChunk {
	/**
	 * Define the type of a data chunk entry.
	 */
        typedef struct Entry {
        	//vector<measure_t> measures;
        	measure_t* measures;
        	
        	Entry():measures(0){}
        	~Entry(){delete [] measures;}        	
        	} 	DataEntry_t;

	/**
	 * The chunk header
	 */        		        	
	DiskChunkHeader	hdr;
	
	/**
	 * The number of non-empty cells. I.e. the number of 1's in
	 * the bitmap.
	 */
	 unsigned int no_ace;
	
	/**
	 * This unsigned integer dynamic array will represent the compression bitmap.
	 * It will be created on the heap from the corresponding bit_vector of class DataChunk
	 * and then copied to our byte array, prior to disk storage. The total length of this bitmap
	 * can be found from the DiskChunkHeader.no_entries attribute.
	 */
	WORD* bitmap;        		
	
	/**
	 * Vector of entries. Note that this is essentially a vector of vectors.
	 * This heap structure is laid out on a byte array entry by entry and measure by measure
	 * prior to disk storage. Therefore,in order to read it back, special offset computing
	 * routines are needed.	
	 */	
	//vector<DataEntry_t> entry;
	DataEntry_t* entry;
	
	/**
	 * Default constructor
	 */	
	DiskDataChunk(): bitmap(0), entry(0){}
	
	/**
	 * constructor
	 */
	 DiskDataChunk(const DiskChunkHeader& h): hdr(h), bitmap(0), entry(0){}
	
	 /**
	  * constructor
	  */
	  DiskDataChunk(const DiskChunkHeader& h, unsigned int i, WORD* const bmp, DataEntry_t * const e):
	  	 hdr(h), no_ace(i), bitmap(bmp), entry(e){}
	  	
	/**
	 * Destructor
	 */  	
	~DiskDataChunk() { delete [] bitmap; delete [] entry; }
	
	/**
	 * Allocate WORDS for storing a bitmap of size n
	 */
	 inline allocBmp(int n){
	 	try{
			bitmap = new WORD[numOfWords(n)];
		}
		catch(bad_alloc){
			throw bad_alloc();		
		}
	}	
	
	/**
	 * turn on bit i
	 */
	inline void setbit(int i){
		WORD MASK = ::create_mask();
	        bitmap[i>>SHIFT] |= (1<<(i & MASK));
	}
	
	/**
	 * turn off bit i
	 */
	inline void clearbit(int i){
	        WORD MASK = ::create_mask();
	        bitmap[i>>SHIFT] &= ~(1<<(i & MASK));
	}
	
	/**
	 * test bit i. Returns 0 if bit i is 0 and 1 if it is 1.
	 */
	inline int testbit(int i) const {
	        WORD MASK = ::create_mask();
        	return bitmap[i>>SHIFT] & (1<<(i & MASK));	
	}
};

/**
 * Header of a DiskBucket. Contains info about the physical organization
 * of a DiskBucket.
 *
 * @author Nikos Karayannidis
 */
struct DiskBucketHeader{
	/**
	 * SSM style id of the SSM record where the bucket resides
	 */
	BucketID id;
	
	/**
	 * Id of the previous bucket.In order to exploit a "good" bucket
	 * order different than the one used by the SSM for the records of a file.
	 * ALSO used for traversing a chain of overflow buckets.
	 */
	BucketID previous;
	
	/**
	 * Id of the next bucket.In order to exploit a "good" bucket
	 * order different than the one used by the SSM for the records of a file.
	 * ALSO used for traversing a chain of overflow buckets.
	 */	
	BucketID next;

	/**
	 * Total number of chunks in this bucket. Also gives the following info:
	 * 	- number of entries in the bucket directory
	 *	- next available index entry (i.e chunk slot in the bucket)
	 *	- if equals with 1 then this should be a data chunk
	 */
	unsigned short no_chunks;
	
	/**
	 * The next available byte offset to store a chunk.
	 * NOTE: the type of this field should be the same
	 * as DiskBucket::direntry_t.
	 */
	unsigned int 	next_offset;
	
	/**
	 * Number of free bytes (contiguous space)
	 */
	size_t freespace;	
	
	/**
	 * The number of chunk-subtrees stored in this bucket
	 * (at least 1)
	 */
	 char no_subtrees;		
	
	 /**
	  * Maximum no of subtrees in a single bucket
	  */
	 enum {subtreemaxno = 100};
	
	 /**
	  * Each entry of this array stores the entry in the bucket directory (i.e. chunk slot)
	  * that points to the first chunk of a subtree. Subtrees are stored
	  * in this array, in the same order that subtrees are stored in the bucket.
	  */
	 unsigned short subtree_dir_entry[subtreemaxno];
	
	 /**
	  * Number of overflow buckets following next in the chain. Normally this
	  * member is set to 0, denoting no use of overflow buckets. If however we have
	  * stored a single large data chunk, or the root bucket has overflowed then we
	  * use chains. If this is the first bucket of 4-member chain then no_ovrfl_next == 3.
	  */
	  char no_ovrfl_next;	
}; //end of DiskBucketHeader

/**
 * DiskBucket is the structure implementing the Bucket concept. It has a fixed
 * size that depends from the constant PAGESIZE. At the beginning of a bucket we
 * store the DiskBucketHeader, then follow the variable-size chunks in the following manner:
 * {subtree1 dirchunks}{subtree1 datachunks}{subtree2 dirchunks}{subtree2 datachunks}...
 * Finally, beginning from the end of the bucket space and expanding BACKWARDS (!) lies
 * the internal chunk directory, which provides the byte offset for each chunk stored
 * in this bucket. Therefore, the abstraction provided here is that a bucket consists of
 * several "chunk slots".The location of each slot can be found through the directory.
 *
 * @author Nikos Karayannidis
 */

struct DiskBucket {
	/**
	 * Define the type of a directory entry
	 */
        typedef unsigned int dirent_t;

        /**
         * Size of the body of the bucket.
         */
        enum {bodysize = PAGESIZE-sizeof(DiskBucketHeader)-sizeof(dirent_t*)};

        /**
         * Minimum bucket occupancy threshold (in bytes)
         */
        static const unsigned int BCKT_THRESHOLD = int(bodysize*0.5);

        /**
         * The header of the bucket.
         */
	DiskBucketHeader hdr; // header
	
	/**
	 * The body of the bucket. I.e. where the chunks and the
         * directory entries will be stored.
	 */
	char body[bodysize];
	
	/**
	 * The bucket directory (grows backwards!).
	 * (Initialize this directory pointer to point one beyond last byte of body).
	 * The chunk at slot x (where x = 0 for the 1st chunk) resides at the byte offset:
	 * offsetInBucket[-x-1]
	 */
	dirent_t* offsetInBucket;	
}; //end of DiskBucket

#endif //DISK_STRUCTURES_H
