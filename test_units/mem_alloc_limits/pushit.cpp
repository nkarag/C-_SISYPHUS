///////////////////////////////////////
// We will try to allocate space
// for different DiskBucket sizes
// and push the memory to the limit!
//
// 15/11/2001 -NIKOS KARAYANNIDIS
//////////////////////////////////////
#include <iostream.h>
#include <new>

const unsigned int PAGESIZE = 30000000;//16384;//8192;//32768;//8116; // 8K page //65536;

struct BucketID {
  	/**
	 * SSM Logical Volume id where this record's file resides
	 */
	//lvid_t vid; //you can retrieve this from the SystemManager
  	/**
	 * SSM logical record id
	 */
	unsigned int rid;
};

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
	 //enum {subtreemaxno = 100};
	 static const unsigned int subtreemaxno = 100;
	
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


struct DiskBucket {
	/**
	 * Define the type of a directory entry
	 */
        typedef unsigned int dirent_t;

        /**
         * Size of the body of the bucket.
         */
        //enum {bodysize = PAGESIZE-sizeof(DiskBucketHeader)-sizeof(dirent_t*)};
        static const unsigned int bodysize = PAGESIZE-sizeof(DiskBucketHeader)-sizeof(dirent_t*);

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

main()
{
        cout << "allocate a disk bucket of size: "<< sizeof(DiskBucket)<<endl;
        DiskBucket* dbp = 0;
        char* p = 0;
        try{
                //dbp = new DiskBucket;
                p = new char[PAGESIZE];
        }
	catch(bad_alloc){
		cerr<<"could not allocate space!!!\n";
		exit(0);
	}
	for(int i = 0; i < PAGESIZE; i++) {
	        p[i] = char(0);
	}//end for
	
	//char xxx;
	//cin >> xxx;	
	cout<<"Bucket allocated OK!"<<endl;
	delete dbp;

}//main