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
