#include <iostream>
#include <string>
#include <cstring>
#include <cmath>
#include <new>

//the type of a measure value
typedef float measure_t;

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
	  DiskDataChunk(const DiskChunkHeader& h, unsigned int i, WORD* bmp, DataEntry_t * e):
	  	 hdr(h), no_ace(i), bitmap(bmp), entry(e){}
	  	
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
	inline int testbit(int i){
	        WORD MASK = ::create_mask();
        	return bitmap[i>>SHIFT] & (1<<(i & MASK));	
	}
}; //end struct DiskDataChunk



void placeDiskDataChunkInBody(const DiskDataChunk* const chnkp, char* &currentp,
                                size_t& hdr_size, size_t& chnk_size);
DiskChunkHeader* createNewDiskChunkHeader();
DiskDataChunk* createNewDiskDataChunk(const DiskChunkHeader* const hdrp);
void printDataChunk(char* const startp);
void updateDiskDataChunkPointerMembers(DiskDataChunk& chnk);

main()
{
        //allocate a byte array for storage of the header
        char body[1000];
        //init a byte pointer
        char* bytep = body;

        //create a new DiskChunkHeader
        DiskChunkHeader* hdrp = createNewDiskChunkHeader();

        //create a new DiskDataChunk
        DiskDataChunk* chnkp = createNewDiskDataChunk(hdrp);

        size_t chnk_size;
        size_t hdr_size;
       	try{      		
 		placeDiskDataChunkInBody(chnkp,bytep,hdr_size,chnk_size);      		
 	}
  	catch(const char* message) {
  		string msg("");
  		msg += message;
  		throw msg.c_str();
  	}

  	//print the contents of the chunk
  	printDataChunk(body);
  	  	
        cout<<"Bytes consumed by chunk: "<<chnk_size<<endl;
        cout<<"Bytes consumed by chunk header: "<<hdr_size<<endl;
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

DiskDataChunk* createNewDiskDataChunk(const DiskChunkHeader* const hdrp)
//precondition:
//      hdrp points at a DiskChunkHEader stucture
//postcondition:
//      returns a pointer to a DiskDataChunk allocated in heap space that contains
//      *hdrp as its hdr member.
{
        DiskDataChunk* chnkp = new DiskDataChunk(*hdrp);

        //get number of ace
        cout<<"Give number of ace (non-empty positions)"<<endl;
        cin>>chnkp->no_ace;

        //get the bitmap
        //allocate new bitmap
        chnkp->bitmap = new WORD[numOfWords(hdrp->no_entries)];
        cout << "give me bitmap (exactly "<<hdrp->no_entries<<" bits - no whitespace in between bits): \n";
        char c;
        cin.ignore(); //get rid of the last '\n'
        for(int i=0; i<hdrp->no_entries; i++){
                cin.get(c); //read one character
                (c=='1') ? chnkp->setbit(i) : chnkp->clearbit(i) ;
        }// end for

        //finally get the entries
        //allocate space for the entries
        chnkp->entry = new DiskDataChunk::DataEntry_t[chnkp->no_ace];
        //for each data entry allocate space for the measures
        for(int i=0; i<chnkp->no_ace; i++){
                chnkp->entry[i].measures = new measure_t[hdrp->no_measures];
        }//end for
        // read the measures from the user
        for(int i=0; i<chnkp->no_ace; i++){
                cout<<"Give measures for entry "<<i<<": ";
                for(int j=0; j<hdrp->no_measures; j++){
                        cin>>chnkp->entry[i].measures[j];
                        cout<<chnkp->entry[i].measures[j]<<", ";
                }//end for
                cout<<endl;
        }//end for

        return chnkp;
}//createNewDiskDataChunk

void printDataChunk(char* const startp)
// precondition:
//   startp is a byte pointer that points at the beginning of the byte stream where a DiskDataChunk
//   has been stored.
// postcondition:
//   The pointer members of the DiskDataChunk are updated to point at the corresponding arrays.
//   The contents of the DiskDataChunk are printed.
{
        //get a pointer to the data chunk
        DiskDataChunk* const chnkp = reinterpret_cast<DiskDataChunk*>(startp);

        //update pointer members
        updateDiskDataChunkPointerMembers(*chnkp);

        //print header
        //use chnkp->hdrp to print the content of the header
        cout<<"**************************************"<<endl;
        cout<<"Depth: "<<int(chnkp->hdr.depth)<<endl;
        cout<<"No_dims: "<<int(chnkp->hdr.no_dims)<<endl;
        cout<<"No_measures: "<<int(chnkp->hdr.no_measures)<<endl;
        cout<<"No_entries: "<<chnkp->hdr.no_entries<<endl;
        //print chunk id
        for(int i=0; i<chnkp->hdr.depth; i++){
                for(int j=0; j<chnkp->hdr.no_dims; j++){
                        cout<<(chnkp->hdr.chunk_id)[i].ordercodes[j];
                        (j==int(chnkp->hdr.no_dims)-1) ? cout<<"." : cout<<"|";
                }//end for
        }//end for
        cout<<endl;
        //print the order code ranges per dimension level
        for(int i=0; i<chnkp->hdr.no_dims; i++)
                cout<<"Dim "<<i<<" range: left = "<<(chnkp->hdr.oc_range)[i].left<<", right = "<<(chnkp->hdr.oc_range)[i].right<<endl;

        //print no of ace
        cout<<"No of ace: "<<chnkp->no_ace<<endl;

        //print the bitmap
        cout<<"\nBITMAP:\n\t";
        for(int b=0; b<chnkp->hdr.no_entries; b++){
                  (!chnkp->testbit(b)) ? cout<<"0" : cout<<"1";
        }//end for

        //print the data entries
        cout<<"\nDiskDataChunk entries:\n";
        cout<<"---------------------\n";
        for(int i=0; i<chnkp->no_ace; i++){
                cout<<"Data entry "<<i<<": ";
                for(int j=0; j<chnkp->hdr.no_measures; j++){
                        cout<<chnkp->entry[i].measures[j]<<", ";
                }//end for
                cout<<endl;
        }//end for
}//end printDataChunk

void updateDiskDataChunkPointerMembers(DiskDataChunk& chnk)
//precondition:
//      chnk is a DiskDataChunk structure but it contains uninitialised pointer members
//postcondition:
// the following pointer members have been initialized to point at the corresponding arrays:
// chnk.hdr.chunk_id, chnk.hdr.chunk_id[i].ordercodes (0<=i<chnk.hdr.depth), chnk.hdr.oc_range,
// chnk.bitmap, chnk.entry, chnk.entry[i].measures (0<= i <chnk.no_ace).
{

        //get  a byte pointer
        char* bytep = reinterpret_cast<char*>(&chnk);

        //update chunk_id pointer
        bytep += sizeof(DiskDataChunk); // move at the end of the static part
        chnk.hdr.chunk_id = reinterpret_cast<DiskChunkHeader::Domain_t*>(bytep);

        //update each domain pointer
        //We need to initialize the "ordercodes" pointer inside each Domain_t:
        //place byte pointer at the 1st ordercode of the first Domain_t
        bytep += sizeof(DiskChunkHeader::Domain_t) * chnk.hdr.depth;
        //for each Domain_t
        for(int i=0; i<chnk.hdr.depth; i++){
                //place byte pointer at the 1st ordercode of the current Domain_t
                (chnk.hdr.chunk_id)[i].ordercodes = reinterpret_cast<DiskChunkHeader::ordercode_t*>(bytep);
                bytep += sizeof(DiskChunkHeader::ordercode_t) * chnk.hdr.no_dims;
        }//end for

        //update oc_range pointer
        // in the header to point at the first OrderCodeRng_t
        // the currp must already point  at the first OrderCodeRng_t
        chnk.hdr.oc_range = reinterpret_cast<DiskChunkHeader::OrderCodeRng_t*>(bytep);

        //update bitmap pointer
        bytep += sizeof(DiskChunkHeader::OrderCodeRng_t)*chnk.hdr.no_dims;
        chnk.bitmap = reinterpret_cast<WORD*>(bytep);

        //update entry pointer
        bytep += sizeof(WORD)* ::numOfWords(chnk.hdr.no_entries); // move to the 1st data entry
        chnk.entry = reinterpret_cast<DiskDataChunk::DataEntry_t*>(bytep);

        //move byte pointer at the first measure value
        bytep += sizeof(DiskDataChunk::DataEntry_t)*chnk.no_ace;
        //for each data entry update measures pointer
        for(int e=0; e<chnk.no_ace; e++){
                //place measures pointer at the first measure
                chnk.entry[e].measures = reinterpret_cast<measure_t*>(bytep);
                bytep += sizeof(measure_t)*chnk.hdr.no_measures; //move on to next set of measures
        }//end for
}//updateDiskDataChunkPointerMembers

void placeDiskDataChunkInBody(const DiskDataChunk* const chnkp, char* &currentp,
                                size_t& hdr_size, size_t& chnk_size)
// precondition:
//		chnkp points at a DiskDataChunk structure && currentp is a byte pointer pointing in the
//		body of a DiskBucket at the point, where the DiskDataChunk must be placed.
// postcondition:
//		the DiskDataChunk has been placed in the body && currentp points at the next free byte in
//		the body && chnk_size contains the bytes consumed by the placement of the DiskDataChunk &&
//              hdr_size contains the bytes consumed by the placement of the DiskChunkHeader.
{
        // init size counters
        chnk_size = 0;
        hdr_size = 0;

	//ASSERTION1: input pointers are not null
	if(!chnkp || !currentp)
		throw "AccessManager::placeDiskDirChunkInBody ==> ASSERTION1: null pointer\n";

	//get a const pointer to the DiskChunkHeader
	const DiskChunkHeader* const hdrp = &chnkp->hdr;
	
	//begin by placing the static part of a DiskDatachunk structure
	memcpy(currentp, reinterpret_cast<char*>(chnkp), sizeof(DiskDataChunk));
	currentp += sizeof(DiskDataChunk); // move on to the next empty position
	chnk_size += sizeof(DiskDataChunk); // this is the size of the static part of a DiskDataChunk
	hdr_size += sizeof(DiskChunkHeader); // this is the size of the static part of a DiskChunkHeader
			
	//continue with placing the chunk id
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
		chnk_size += sizeof(DiskChunkHeader::Domain_t);
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
        		chnk_size += sizeof(DiskChunkHeader::ordercode_t);		
		}//end for	
        }//end for
		
	//next place the orcercode ranges
	//ASSERTION4: oc_range is not null
	if(!hdrp->oc_range)
		throw "AccessManager::placeDiskChunkHdrInBody ==> ASSERTION4: null pointer\n";		
	for(int i = 0; i < hdrp->no_dims; i++) {
	//loop invariant: store an order code range structure
		DiskChunkHeader::OrderCodeRng_t* rngp = &(hdrp->oc_range)[i];
       		memcpy(currentp, reinterpret_cast<char*>(rngp), sizeof(DiskChunkHeader::OrderCodeRng_t));
       		currentp += sizeof(DiskChunkHeader::OrderCodeRng_t); // move on to the next empty position
       		hdr_size += sizeof(DiskChunkHeader::OrderCodeRng_t);		
       		chnk_size += sizeof(DiskChunkHeader::OrderCodeRng_t);		       		
	}//end for
	
	//next place the bitmap (i.e., array of WORDS)
  	for(int b=0; b<numOfWords(hdrp->no_entries); b++){
		WORD* wp = &chnkp->bitmap[b];
		memcpy(currentp, reinterpret_cast<char*>(wp), sizeof(WORD));
	       	currentp += sizeof(WORD); // move on to the next empty position
       		chnk_size += sizeof(WORD);       		
  	}//end for
  	
       	//Now, place the DataEntry_t structures
       	for(int i=0; i<chnkp->no_ace; i++){
        	DiskDataChunk::DataEntry_t* ep = &chnkp->entry[i];
             	memcpy(currentp, reinterpret_cast<char*>(ep), sizeof(DiskDataChunk::DataEntry_t));
             	currentp += sizeof(DiskDataChunk::DataEntry_t); // move on to the next empty position
             	chnk_size += sizeof(DiskDataChunk::DataEntry_t);		       	
       	}//end for

       	//Finally place the measure values
       	// for each data entry
       	for(int i=0; i<chnkp->no_ace; i++){
       		// for each measure of this entry
               	for(int m=0; m<hdrp->no_measures; m++){
                	measure_t* mp = &chnkp->entry[i].measures[m];
                     	memcpy(currentp, reinterpret_cast<char*>(mp), sizeof(measure_t));
                     	currentp += sizeof(measure_t); // move on to the next empty position
                     	chnk_size += sizeof(measure_t);		       	
                }//end for
       	}//end for       	       	
}// end of AccessManager::placeDiskDataChunkInBody      		


