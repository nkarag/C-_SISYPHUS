#include <iostream>
#include <string>
#include <cstring>


/**
 * This struct plays the role of the physical id of a bucket. In the current
 * design this is equivalent with the logical id of a shore record.
 * @author: Nikos Karayannidis
 */
struct BucketID{
        int id;

        BucketID():id(0){}
};


/**
 * This structure implements the header of a chunk (dir or data chunk).
 *
 * @author Nikos Karayannidis
 */
struct DiskChunkHeader {
	/**
	 * Define the type of an order-code
	 */
        typedef short int ordercode_t;

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
	 * We implement the chunk id with a vector of vectors (or array of arrays) of order-codes.
	 * The outermost vector corresponds to the different domains (i.e. depths)
	 * of the chunk id, while the innermost corresponds to the different dimensions.
	 * This heap structure is laid out on a byte array order-code by order-code prior to
	 * disk storage. Therefore,in order to read it back, special offset computing
	 * routines are needed.
	 */
        Domain_t* chunk_id;
	//vector<vector<ordercode_t> > chunk_id;
	//ordercode_t** chunk_id;
	
	
	/**
	 * This vector contains a range of order-codes for each dimension level of the
	 * depth of this chunk.Therefore the length is equal with the number of dimensions.
	 * This range denotes the order-codes covered on each dimension
	 * from this chunk.
	 * This heap structure is laid out on a byte array range by range prior to
	 * disk storage. Therefore,in order to read it back, special offset computing
	 * routines are needed.	
	 */
        OrderCodeRng_t* oc_range;
	 //vector<OrderCodeRng_t> oc_range;
		
        DiskChunkHeader(): chunk_id(0),oc_range(0) {}

	 /**
	  * copy constructor
	  */
	  DiskChunkHeader(const DiskChunkHeader& h):
	  	depth(h.depth),no_dims(h.no_dims),no_measures(h.no_measures),no_entries(h.no_entries),
	  	chunk_id(h.chunk_id),oc_range(h.oc_range) {}

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
	
	DiskDirChunk():entry(0){}
	
	/**
	 * constructor
	 */
	 DiskDirChunk(const DiskChunkHeader& h): hdr(h), entry(0){}
	
	 /**
	  * constructor
	  */
	 DiskDirChunk(const DiskChunkHeader& h, DirEntry_t* const ep): hdr(h), entry(ep){}
	
	~DiskDirChunk() { delete [] entry;}
}; //end DiskDirChunk


size_t placeDiskChunkHdrInBody(const DiskChunkHeader* const hdrp, char* &currentp);
size_t placeDiskDirChunkInBody(const DiskDirChunk* const chnkp, char* &currentp);
DiskChunkHeader* createNewDiskChunkHeader();
DiskDirChunk* createNewDiskDirChunk(const DiskChunkHeader* const hdrp);
void printHeader(char* const startp);
void printChunk(char* const startp);

main()
{
        //allocate a byte array for storage of the header
        char body[1000];
        //init a byte pointer
        char* bytep = body;

        //create a new DiskChunkHeader
        DiskChunkHeader* hdrp = createNewDiskChunkHeader();

        //create a new DiskDirChunk
        DiskDirChunk* chnkp = createNewDiskDirChunk(hdrp);

        size_t chnk_size;
       	try{      		
 		chnk_size = placeDiskDirChunkInBody(chnkp,bytep);      		
 	}
  	catch(const char* message) {
  		string msg("");
  		msg += message;
  		throw msg.c_str();
  	}

  	//print the contents of the chunk
  	printChunk(body);
  	  	
        cout<<"Bytes consumed by chunk: "<<chnk_size<<endl;
        cout<<"bytep - body: "<<reinterpret_cast<int>(bytep-body)<<endl;
}//end main

DiskChunkHeader* createNewDiskChunkHeader()
{
        //create a new DiskChunkHeader
        DiskChunkHeader* hdrp = new DiskChunkHeader;

        //prompt for member values
        short int input;
        cout<<"Give depth: "; cin>>input;
        hdrp->depth = char(input);
        cout<<"\nGive no_dims: "; cin>>input;
        hdrp->no_dims = char(input);
        cout<<"\nGive no_measures: "; cin>>input;
        hdrp->no_measures = char(input);
        cout<<"\nGive no_entries: "; cin>>hdrp->no_entries;
        cout<<endl;

        //allocate space for chunk id
        hdrp->chunk_id = new DiskChunkHeader::Domain_t[hdrp->depth];
        for(int i =0; i<hdrp->depth; i++)
                (hdrp->chunk_id)[i].ordercodes = new DiskChunkHeader::ordercode_t[hdrp->no_dims];

        //prompt for the chunk id
        for(int i =0; i<hdrp->depth; i++){
                cout<<"Domain "<<i<<": "<<endl;
                for(int j=0; j<hdrp->no_dims; j++){
                        cout<<"Give ordercode "<<j<<": ";
                        cin>>(hdrp->chunk_id)[i].ordercodes[j];
                        cout<<endl;
                }//end for
        }//end for

        //allocate space for order-code ranges
        hdrp->oc_range = new DiskChunkHeader::OrderCodeRng_t[hdrp->no_dims];

        //prompt for the ranges on each dimension
        for(int j=0; j<hdrp->no_dims; j++){
                cout<<"Give oc range for dimension "<<j<<endl;
                cout<<"left: "; cin>>(hdrp->oc_range)[j].left;
                cout<<"right: "; cin>>(hdrp->oc_range)[j].right;
        }//end for
        return hdrp;
}// createNewDiskChunkHeader

DiskDirChunk* createNewDiskDirChunk(const DiskChunkHeader* const hdrp)
{
        DiskDirChunk* chnkp = new DiskDirChunk(*hdrp);

        //allocate space for the entries
        chnkp->entry = new DiskDirChunk::DirEntry_t[chnkp->hdr.no_entries];

        //fill in these entries
        int i;
        for( i = 0; i<chnkp->hdr.no_entries; i++){
                chnkp->entry[i].chunk_slot = i;
        }//end for

        return chnkp;
}//createNewDiskDirChunk

void printChunk(char* const startp)
// precondition:
//   startp is a byte pointer that points at the beginning of the byte stream where a DiskDirChunk
//   has been stored.
// postcondition:
//   The "entry" member of the DiskDirChunk is updated to point at the corresponding entries.
//   The contents of the DiskDirChunk are printed.
{
        //print the contents of the stored header in order to check correctness
        printHeader(startp);

        const char* currp = startp; // init current byte pointer

        // get a pointer to the stored header
        const DiskChunkHeader* const hdrp = reinterpret_cast<DiskChunkHeader*>(currp);

        //move current pointer at the chunk "entry" member
        char* helpp = reinterpret_cast<char*>(hdrp->oc_range); //get address of first ordercode range
        //move at the end of the ordercode ranges where the member "entry" lies.
        currp = reinterpret_cast<const char*>(helpp+sizeof(DiskChunkHeader::OrderCodeRng_t) * hdrp->no_dims);
        //set a const pointer to point at the "entry" member
        DiskDirChunk::DirEntry_t** const entryp = reinterpret_cast<DiskDirChunk::DirEntry_t**>(currp);

        //now update the"entry" member to point at the sequence of stored entries, beginning just after it
        *entryp = reinterpret_cast<DiskDirChunk::DirEntry_t*>(currp + sizeof(DiskDirChunk::DirEntry_t*));

        //OK. We can use now *entryp to read the array of stored entries
        DiskDirChunk::DirEntry_t* entry = *entryp;

        //print entries (at long last!!)
        cout<<"\nDiskDirChunk entries:\n";
        cout<<"---------------------\n";

        for(int i=0; i<hdrp->no_entries; i++){
                cout<<entry[i].chunk_slot<<endl;
        }//end for
}

void printHeader(char* const startp)
// precondition:
//   startp is a byte pointer that points at the beginning of the byte stream where a DiskChunkHeader
//   has been stored.
// postcondition:
//   the pointer members of the DiskChunkHeader (stored in the byte stream) have been updated to point at
//   the corresponding entries. The contents of the DiskChunkHeader are printed
{
        //init current byte pointer
        const char* currp = startp;

        //init a chunk header pointer
        DiskChunkHeader* const hdrp = reinterpret_cast<DiskChunkHeader*>(currp);

        //initiallize the "chunk_id" pointer in the header to point at the first Domain_t:

        //place byte pointer to point at the first Domain_t
        currp = reinterpret_cast<const char*>(&(hdrp->oc_range)); //this is the last member before the 1st Domain_t entry
        currp+=sizeof(hdrp->oc_range); //pass the oc_range member to point at the 1st Domain_t entry
        hdrp->chunk_id = reinterpret_cast<DiskChunkHeader::Domain_t*>(currp);
        //Now we need to initialize the "ordercodes" pointer inside each Domain_t:
        //place byte pointer at the 1st ordercode of the first Domain_t
        currp += sizeof(DiskChunkHeader::Domain_t) * hdrp->depth;
        //for each Domain_t
        for(int i=0; i<hdrp->depth; i++){
                //place byte pointer at the 1st ordercode of the current Domain_t
                (hdrp->chunk_id)[i].ordercodes = reinterpret_cast<DiskChunkHeader::ordercode_t*>(const_cast<char*>(currp));
                currp += sizeof(DiskChunkHeader::ordercode_t) * hdrp->no_dims;
        }//end for

        //initialize the "oc_range" pointer in the header to point at the first OrderCodeRng_t
        // the currp must already point  at the first OrderCodeRng_t
        hdrp->oc_range = reinterpret_cast<DiskChunkHeader::OrderCodeRng_t*>(currp);

        //use hdrp to print the content of the header
        cout<<"**************************************"<<endl;
        cout<<"Depth: "<<int(hdrp->depth)<<endl;
        cout<<"No_dims: "<<int(hdrp->no_dims)<<endl;
        cout<<"No_measures: "<<int(hdrp->no_measures)<<endl;
        cout<<"No_entries: "<<hdrp->no_entries<<endl;
        //print chunk id
        for(int i=0; i<hdrp->depth; i++){
                for(int j=0; j<hdrp->no_dims; j++){
                        cout<<(hdrp->chunk_id)[i].ordercodes[j];
                        (j==int(hdrp->no_dims)-1) ? cout<<"." : cout<<"|";
                }//end for
        }//end for
        cout<<endl;
        //print the order code ranges per dimension level
        for(int i=0; i<hdrp->no_dims; i++)
                cout<<"Dim "<<i<<" range: left = "<<(hdrp->oc_range)[i].left<<", right = "<<(hdrp->oc_range)[i].right<<endl;
}//end printHeader

size_t placeDiskDirChunkInBody(const DiskDirChunk* const chnkp, char* &currentp)
// precondition:
//		chnkp points at a DiskDirChunk structure && currentp is a byte pointer pointing in the
//		body of a DiskBucket at the point, where the DiskDirChunk must be placed.
// postcondition:
//		the DiskDirChunk has been placed in the body && currentp points at the next free byte in
//		the body && chnk_size contains the bytes consumed by the placement of the DiskDirChunk.		
{
        size_t chnk_size = 0;

	//ASSERTION1: input pointers are not null
	if(!chnkp || !currentp)
		throw "AccessManager::placeDiskDirChunkInBody ==> ASSERTION1: null pointer\n";

	//first the place the header
	const DiskChunkHeader* const hdrp = &chnkp->hdr;
	size_t hdr_size;
       	try{      		
 		hdr_size = placeDiskChunkHdrInBody(hdrp,currentp);      		
 	}
  	catch(const char* message) {
  		string msg("");
  		msg += message;
  		throw msg.c_str();
  	}
  	chnk_size += hdr_size;
  	
  	//Next place the entry pointer
  	DiskDirChunk::DirEntry_t* const * p = &chnkp->entry;
       	memcpy(currentp, reinterpret_cast<char*>(p), sizeof(DiskDirChunk::DirEntry_t*));
       	currentp += sizeof(DiskDirChunk::DirEntry_t*); // move on to the next empty position
       	chnk_size += sizeof(DiskDirChunk::DirEntry_t*);
       	
       	//Now, place the entries
       	for(int i =0; i<chnkp->hdr.no_entries; i++) { //for each entry
	       	DiskDirChunk::DirEntry_t*ep = &chnkp->entry[i];       		
        	memcpy(currentp, reinterpret_cast<char*>(ep), sizeof(DiskDirChunk::DirEntry_t));
        	currentp += sizeof(DiskDirChunk::DirEntry_t); // move on to the next empty position
        	chnk_size += sizeof(DiskDirChunk::DirEntry_t);		       	
       	}//end for  					
       	
       	return chnk_size;
}// end of AccessManager::placeDiskDirChunkInBody      		

size_t placeDiskChunkHdrInBody(const DiskChunkHeader* const hdrp, char* &currentp)
// precondition:
//		hdrp points at a DiskChunkHeader structure && currentp is a byte pointer pointing in the
//		body of a DiskBucket at the point, where the DiskChunkHeader must be placed.
// postcondition:
//		the DiskChunkHeader has been placed in the body && currentp points at the next free byte in
//		the body && hdr_size contains the bytes consumed by the placement of the header
{
        size_t hdr_size =0; //will store the consumed bytes

	//ASSERTION1: hdrp and currentp non null pointers
	if(!hdrp || !currentp)
		throw "AccessManager::placeDiskChunkHdrInBody ==> ASSERTION1: null pointer\n";

	// first store the static-size part		
	memcpy(currentp, reinterpret_cast<char*>(hdrp), sizeof(DiskChunkHeader));
	currentp += sizeof(DiskChunkHeader); // move on to the next empty position
	hdr_size += sizeof(DiskChunkHeader);
	
	// Next, store the chunk id
	//ASSERTION2: chunkid is not null
	if(!hdrp->chunk_id)
		throw "AccessManager::placeDiskChunkHdrInBody ==> ASSERTION2: null pointer\n";	
	//first store the domains of the chunk id
	for (int i = 0; i < hdrp->depth; i++){ //for each domain of the chunk id
	//loop invariant: a domain of the chunk id will be stored. A domain
	// is only a pointer to an array of order codes.
		DiskChunkHeader::Domain_t* dmnp = &(hdrp->chunk_id)[i];
		memcpy(currentp, reinterpret_cast<char*>(dmnp), sizeof(DiskChunkHeader::Domain_t));
		currentp += sizeof(DiskChunkHeader::Domain_t); // move on to the next empty position
		hdr_size += sizeof(DiskChunkHeader::Domain_t);
	}//end for		
	//Next store the order-codes of the domains
	for (int i = 0; i < hdrp->depth; i++){ //for each domain of the chunk id	
		//ASSERTION3: ordercodes pointer is not null
		if(!(hdrp->chunk_id)[i].ordercodes)
			throw "AccessManager::placeDiskChunkHdrInBody ==> ASSERTION3: null pointer\n";			
		for(int j = 0; j < hdrp->no_dims; j++) { //fore each order code of this domain
		//loop invariant: each ordercode of the current domain will stored
			DiskChunkHeader::ordercode_t* op = &((hdrp->chunk_id)[i].ordercodes[j]);
        		memcpy(currentp, reinterpret_cast<char*>(op), sizeof(DiskChunkHeader::ordercode_t));
        		currentp += sizeof(DiskChunkHeader::ordercode_t); // move on to the next empty position
        		hdr_size += sizeof(DiskChunkHeader::ordercode_t);		
		}//end for	
        }//end for
	
	//next store the order code ranges
	//ASSERTION4: oc_range is not null
	if(!hdrp->oc_range)
		throw "AccessManager::placeDiskChunkHdrInBody ==> ASSERTION4: null pointer\n";		
	for(int i = 0; i < hdrp->no_dims; i++) {
	//loop invariant: store an order code range structure
		DiskChunkHeader::OrderCodeRng_t* rngp = &(hdrp->oc_range)[i];
       		memcpy(currentp, reinterpret_cast<char*>(rngp), sizeof(DiskChunkHeader::OrderCodeRng_t));
       		currentp += sizeof(DiskChunkHeader::OrderCodeRng_t); // move on to the next empty position
       		hdr_size += sizeof(DiskChunkHeader::OrderCodeRng_t);		
	}//end for
	
	return hdr_size;
}// end AccessManager::placeDiskChunkHdrInBody