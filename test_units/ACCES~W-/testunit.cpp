///////////////////////////////////////////////////////
// Testing of unit: AccessManager::createDiskBucket
//
// (C) Nikos Karayannidis       August 2001
///////////////////////////////////////////////////////
#include <strstream>

#include "descend.h"
#include "DiskStructures.h"

//enum TreeTraversal_t {depthFirst,breadthFirst};
/**
 * This procedure receives 2 vectors containing DirChunks and DataChunks respectively and
 * a NULL pointer to a DiskBucket. It allocates in heap a new DiskBucket and fills it appropriatelly
 * with the chunks from the two input vectors. The input flag howToTraverse denotes if the chunks
 * should be stored in a depth first or a breadth first manner.
 *
 * @param maxDepth      the max chunking depth of the cube in question
 * @param numFacts      the number of facts in each data entry (i.e., cell)
 * @param bcktID        the bucket id of the bucket corresponding to this DiskBucket
 * @param howToTraverse a flag indicating storage method. Possible values: depthFirst, breadthFirst
 */
void createDiskBucketInHeap(unsigned int maxDepth, unsigned int numFacts, const BucketID& bcktID,
		 const vector<DirChunk>*dirVectp,
		 const vector<DataChunk>*dataVectp,
		 DiskBucket* &dbuckp, const TreeTraversal_t howToTraverse = breadthFirst);

void _storeBreadth1stInDiskBucket(unsigned int maxDepth, unsigned int numFacts,
		 const vector<DirChunk>*dirVectp, const vector<DataChunk>*dataVectp,
		 DiskBucket* const dbuckp, char* &nextFreeBytep);		 		

void _storeDepth1stInDiskBucket(unsigned int maxDepth, unsigned int numFacts,
		 const vector<DirChunk>*dirVectp, const vector<DataChunk>*dataVectp,
		 DiskBucket* const dbuckp, char* &nextFreeBytep);		 		
		
void printDiskBucket(DiskBucket* const dbuckp, unsigned int maxDepth);		
void printDiskDirChunk(ofstream& out, char* const startp);
void printDiskDataChunk(ofstream& out, char* const startp);
void updateDiskDirChunkPointerMembers(DiskDirChunk& chnk);
void updateDiskDataChunkPointerMembers(DiskDataChunk& chnk);

DiskDirChunk* dirChunk2DiskDirChunk(const DirChunk& dirchnk, unsigned int maxDepth);

DiskDataChunk* dataChunk2DiskDataChunk(const DataChunk& datachnk, unsigned int numFacts,
					unsigned int maxDepth);
void placeDiskDirChunkInBcktBody(const DiskDirChunk* const chnkp, unsigned int maxDepth,
       	char* &currentp, size_t& hdr_size, size_t& chnk_size);				

void placeDiskDataChunkInBcktBody(const DiskDataChunk* const chnkp, unsigned int maxDepth,
       	char* &currentp,size_t& hdr_size, size_t& chnk_size);		
		
main()
{

        //create the two vectors containing Dir and Data chunks respectively
        unsigned int maxDepth, numFacts;
        vector<DirChunk>*dirVectp = 0;
        vector<DataChunk>*dataVectp = 0;
        TreeTraversal_t howToTraverse;
        try{
                createChunkVectors(maxDepth, numFacts, dirVectp, dataVectp, howToTraverse);
        }
        catch(const char* m){
                cerr<<m;
        }

        // call createDiskBucket
        DiskBucket* dbuckp = 0;
        BucketID bid; // a dummy bucket id
        try{
                createDiskBucketInHeap(maxDepth, numFacts, bid, dirVectp, dataVectp, dbuckp, howToTraverse);

        }
        catch(const char* m){
                cerr<<m;
        }

        //print the contents of the DiskBucket for testing
        try{
                printDiskBucket(dbuckp, maxDepth);
        }
        catch(const char* m){
                cerr<<m;
        }
}//end main

void printDiskBucket(DiskBucket* const dbuckp, unsigned int maxDepth)
// precondition:
//      dbuckp points at a DiskBucket struct allocated in heap, which has been
//      created by a call to AccessManager::createDiskBucketInHeap.
// postcondition:
//      All the contents of the bucket are printed in a file.
{
        //test the created tree
	ofstream out("outputDiskBucket.txt");

        if (!out)
        {
            cerr << "creating file \"outputDiskBucket.txt\" failed\n";
        }

        out<<"****************************************************************"<<endl;
        out<<"* Contents of a DiskBucket created by                          *"<<endl;
        out<<"*         AccessManager::creatediskBucketInHeap                *"<<endl;
        out<<"****************************************************************"<<endl;

        out<<"\nBUCKET HEADER"<<endl;
        out<<"-----------------"<<endl;
        out<<"id: "<<dbuckp->hdr.id.rid<<endl;
        out<<"previous: "<<dbuckp->hdr.previous.rid<<endl;
        out<<"next: "<<dbuckp->hdr.next.rid<<endl;
        out<<"no_chunks: "<<dbuckp->hdr.no_chunks<<endl;
        out<<"next_offset: "<<dbuckp->hdr.next_offset<<endl;
        out<<"freespace: "<<dbuckp->hdr.freespace<<endl;
        out<<"no_subtrees: "<<int(dbuckp->hdr.no_subtrees)<<endl;
        out<<"subtree directory entries:\n";
        for(int i=0; i<dbuckp->hdr.no_subtrees; i++)
                out<<"subtree_dir_entry["<<i<<"]: "<<dbuckp->hdr.subtree_dir_entry[i]<<endl;
        out<<"no_ovrfl_next: "<<int(dbuckp->hdr.no_ovrfl_next)<<endl;

        //read sequencially the chunk directory stored at the end of the DiskBucket
        for(int chnkslot=0; chnkslot<dbuckp->hdr.no_chunks; chnkslot++) {
                //access each chunk:
                //get a byte pointer at the beginning of the chunk
                char* beginChunkp = dbuckp->body + dbuckp->offsetInBucket[-chnkslot-1];

                //first read the chunk header in order to find whether it is a DiskDirChunk, or
                //a DiskDataChunk
                DiskChunkHeader* chnk_hdrp = reinterpret_cast<DiskChunkHeader*>(beginChunkp);
                if(chnk_hdrp->depth == maxDepth)//then this is a DiskDataChunk
                        printDiskDataChunk(out, beginChunkp);
                else if (chnk_hdrp->depth < maxDepth && chnk_hdrp->depth > Chunk::MIN_DEPTH)//it is a DiskDirchunk
                        printDiskDirChunk(out, beginChunkp);
                else {// Invalid depth!
                        if(chnk_hdrp->depth == Chunk::MIN_DEPTH)
                                throw "printDiskBucekt ==> depth corresponding to root chunk!\n";
                        ostrstream msg_stream;
                        msg_stream<<"printDiskBucekt ==> chunk depth at slot "<<chnkslot        <<
                                        " inside DiskBucket "<<dbuckp->hdr.id.rid<<" is invalid\n"<<endl;
                        throw msg_stream.str();
                }//end else
        }//end for
}//end printDiskBucket

void printDiskDirChunk(ofstream& out, char* const startp)
// precondition:
//   startp is a byte pointer that points at the beginning of the byte stream where a DiskDirChunk
//   has been stored.
// postcondition:
//   The pointer members of the DiskDirChunk are updated to point at the corresponding arrays.
//   The contents of the DiskDirChunk are printed.
{
        //get a pointer to the dir chunk
        DiskDirChunk* const chnkp = reinterpret_cast<DiskDirChunk*>(startp);

        //update pointer members
        updateDiskDirChunkPointerMembers(*chnkp);

        //print header
        //use chnkp->hdrp to print the content of the header
        out<<"**************************************"<<endl;
        out<<"Depth: "<<int(chnkp->hdr.depth)<<endl;
        out<<"No_dims: "<<int(chnkp->hdr.no_dims)<<endl;
        out<<"No_measures: "<<int(chnkp->hdr.no_measures)<<endl;
        out<<"No_entries: "<<chnkp->hdr.no_entries<<endl;
        //print chunk id
        for(int i=0; i<chnkp->hdr.depth; i++){
                for(int j=0; j<chnkp->hdr.no_dims; j++){
                        out<<(chnkp->hdr.chunk_id)[i].ordercodes[j];
                        (j==int(chnkp->hdr.no_dims)-1) ? out<<"." : out<<"|";
                }//end for
        }//end for
        out<<endl;
        //print the order code ranges per dimension level
        for(int i=0; i<chnkp->hdr.no_dims; i++)
                out<<"Dim "<<i<<" range: left = "<<(chnkp->hdr.oc_range)[i].left<<", right = "<<(chnkp->hdr.oc_range)[i].right<<endl;

        //print the dir entries
        out<<"\nDiskDirChunk entries:\n";
        out<<"---------------------\n";
        for(int i=0; i<chnkp->hdr.no_entries; i++){
                out<<"Dir entry "<<i<<": ";
                out<<chnkp->entry[i].bucketid.rid<<", "<<chnkp->entry[i].chunk_slot<<endl;;
        }//end for
}//printDiskDirChunk

void printDiskDataChunk(ofstream& out, char* const startp)
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
        out<<"**************************************"<<endl;
        out<<"Depth: "<<int(chnkp->hdr.depth)<<endl;
        out<<"No_dims: "<<int(chnkp->hdr.no_dims)<<endl;
        out<<"No_measures: "<<int(chnkp->hdr.no_measures)<<endl;
        out<<"No_entries: "<<chnkp->hdr.no_entries<<endl;
        //print chunk id
        for(int i=0; i<chnkp->hdr.depth; i++){
                for(int j=0; j<chnkp->hdr.no_dims; j++){
                        out<<(chnkp->hdr.chunk_id)[i].ordercodes[j];
                        (j==int(chnkp->hdr.no_dims)-1) ? out<<"." : out<<"|";
                }//end for
        }//end for
        out<<endl;
        //print the order code ranges per dimension level
        for(int i=0; i<chnkp->hdr.no_dims; i++)
                out<<"Dim "<<i<<" range: left = "<<(chnkp->hdr.oc_range)[i].left<<", right = "<<(chnkp->hdr.oc_range)[i].right<<endl;

        //print no of ace
        out<<"No of ace: "<<chnkp->no_ace<<endl;

        //print the bitmap
        out<<"\nBITMAP:\n\t";
        for(int b=0; b<chnkp->hdr.no_entries; b++){
                  (!chnkp->testbit(b)) ? out<<"0" : out<<"1";
        }//end for

        //print the data entries
        out<<"\nDiskDataChunk entries:\n";
        out<<"---------------------\n";
        for(int i=0; i<chnkp->no_ace; i++){
                out<<"Data entry "<<i<<": ";
                for(int j=0; j<chnkp->hdr.no_measures; j++){
                        out<<chnkp->entry[i].measures[j]<<", ";
                }//end for
                out<<endl;
        }//end for
}//end printDiskDataChunk

void updateDiskDirChunkPointerMembers(DiskDirChunk& chnk)
//precondition:
//      chnk is a DiskDirChunk structure but it contains uninitialised pointer members
//postcondition:
// the following pointer members have been initialized to point at the corresponding arrays:
// chnk.hdr.chunk_id, chnk.hdr.chunk_id[i].ordercodes (0<=i<chnk.hdr.depth), chnk.hdr.oc_range,
// chnk.entry.
{

        //get  a byte pointer
        char* bytep = reinterpret_cast<char*>(&chnk);

        //update chunk_id pointer
        bytep += sizeof(DiskDirChunk); // move at the end of the static part
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

        //update entry pointer
        bytep += sizeof(DiskChunkHeader::OrderCodeRng_t)*chnk.hdr.no_dims;
        chnk.entry = reinterpret_cast<DiskDirChunk::DirEntry_t*>(bytep);
}//updateDiskDirChunkPointerMembers


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


void createDiskBucketInHeap(unsigned int maxDepth, unsigned int numFacts, const BucketID& bcktID,
		 const vector<DirChunk>*dirVectp, const vector<DataChunk>*dataVectp,
		 DiskBucket* &dbuckp, const TreeTraversal_t howToTraverse)
// precondition:
//	dbuckp points to NULL && dirVectp and dataVectp contain the chunks of a single tree
//	that can fit in a single DiskBucket. howToTraverse shows how we want to store the chunks
//      in the body of the bucket, i.e., in a depth first or in breadth first manner. The former
//      assumes that the two input vectors have been created with AccessManager::descendDepth1stCostTree,
//      while the latter assumes creation  with AccessManager::descendBreadthFirstCostTree.
// postcondition:
//	the chunks of the two vectors have been stored in the chunk slots of a DiskBucket that is allocated
//	in heap and pointed to by dbuckp.
{
	//ASSERTION1: dbuckp points to NULL
	if(dbuckp)
		throw "AccessManager::createDiskBucket ==> ASSERTION1: input pointer to DiskBucket should be null\n";

	// allocate DiskBucket in heap			
	dbuckp = new DiskBucket;
	
	// initialize directory pointer to point one beyond last byte of body
	dbuckp->offsetInBucket = reinterpret_cast<DiskBucket::dirent_t*>(&(dbuckp->body[DiskBucket::bodysize]));
	// initialize the DiskBucketHeader
	dbuckp->hdr.id.rid = bcktID.rid; // store the bucket id
	
	// Init the links to other buckets with null ids
//	dbuckp->hdr.next.rid = serial_t::null;
//	dbuckp->hdr.previous.rid = serial_t::null;
//******************REPLACE THE 2 FOLLOWING LINES WITH THE 2 PREVIOUS LINES IN SISYPHUS SOURCES**************
	dbuckp->hdr.next.rid = 0;
	dbuckp->hdr.previous.rid = 0;	
//*******************************************************************************************
	
	dbuckp->hdr.no_chunks = 0;
	dbuckp->hdr.next_offset = 0; //next free byte offset in the body
	dbuckp->hdr.freespace = DiskBucket::bodysize;
	dbuckp->hdr.no_subtrees = 1; // single tree stored
	dbuckp->hdr.subtree_dir_entry[0] = 0; // the root of this tree will be stored at chunk slot 0
	dbuckp->hdr.no_ovrfl_next = 0; // no overflow-bucket  chain used
	
	char* nextFreeBytep = dbuckp->body; //init current byte pointer
	//size_t curr_offs = dbuckp->hdr.next_offset;  // init current byte offset

        //According to the desired traversal method store the chunks in the body of the bucket	
        switch(howToTraverse){
                case breadthFirst:
                        try{
                                _storeBreadth1stInDiskBucket(maxDepth, numFacts,
		                        dirVectp, dataVectp, dbuckp, nextFreeBytep);
                        }
                	catch(const char* message) {
                		string msg("AccessManager::createDiskBucket ==> ");
                		msg += message;
                		throw msg.c_str();
                	}		
                        break;

                case depthFirst:
                        try{
                                _storeDepth1stInDiskBucket(maxDepth, numFacts,
		                        dirVectp, dataVectp, dbuckp, nextFreeBytep);
                        }
                	catch(const char* message) {
                		string msg("AccessManager::createDiskBucket ==> ");
                		msg += message;
                		throw msg.c_str();
                	}		
                        break;
                default:
                        throw "AccessManager::createDiskBucket ==> unknown traversal method\n";
                        break;
        }//end switch
}//end of AccessManager::createDiskBucketInHeap

void _storeBreadth1stInDiskBucket(unsigned int maxDepth, unsigned int numFacts,
		 const vector<DirChunk>*dirVectp, const vector<DataChunk>*dataVectp,
		 DiskBucket* const dbuckp, char* &nextFreeBytep)
// precondition:
//      dirVectp and dataVectp (input parameters) contain the chunks of a single tree
//	that can fit in a single DiskBucket. These chunks have been placed in the 2 vectors with a
//      call to AccessManager::descendBreadth1stCostTree. dbuckp (input parameter) is a const pointer to an
//      allocated DiskBucket, where its header members have been initialized. Finally, nextFreeBytep is
//      a byte pointer (input+output parameter) that points at the beginning of the DiskBucket's body.
// postcondition:
//      the chunks of the two vectors have been placed in the body of the bucket with respect to a
//      breadth first traversal of the tree. The nextFreeBytep pointer points at the first free byte in the
//      body of the bucket.
{
        // for each dir chunk of this subtree
        for(vector<DirChunk>::const_iterator dir_i = dirVectp->begin();
            dir_i != dirVectp->end(); dir_i++){
		// loop invariant: a DirChunk is stored in each iteration in the body
		//		   of the DiskBucket

		//ASSERTION 1: there is free space to store this dir chunk
		if(dir_i->gethdr().size > dbuckp->hdr.freespace)
			throw "AccessManager::_storeBreadth1stInDiskBucket ==> ASSERTION 1: DirChunk does not fit in DiskBucket!\n";		

      		// update bucket directory (chunk slots begin from slot 0)
      		dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = dbuckp->hdr.next_offset;
      		dbuckp->hdr.freespace -= sizeof(DiskBucket::dirent_t);
        		
      		// convert DirChunk to DiskDirChunk
      		DiskDirChunk* chnkp = 0;
      		try{
                        chnkp = dirChunk2DiskDirChunk(*dir_i, maxDepth);
	      	}
         	catch(const char* message) {
         		string msg("AccessManager::_storeBreadth1stInDiskBucket==>");
         		msg += message;
         		throw msg.c_str();
         	}		
	      	
         	// Now, place the parts of the DiskDirChunk into the body of the DiskBucket
      		size_t chnk_size = 0;
      		size_t hdr_size = 0;
      		try{      		
                        placeDiskDirChunkInBcktBody(chnkp, maxDepth, nextFreeBytep, hdr_size, chnk_size);							
		}
         	catch(const char* message) {
         		string msg("AccessManager::_storeBreadth1stInDiskBucket==>");
         		msg += message;
         		throw msg.c_str();
         	}		
         	#ifdef DEBUGGING
                        //ASSERTION 1.1 : no chunk size mismatch         	
                        if(dir_i->gethdr().size != chnk_size)
                                throw "AccessManager::_storeBreadth1stInDiskBucket ==> ASSERTION 1.1: DirChunk size mismatch!\n";		
         	#endif
         	
                //update next free byte offset indicator
     		dbuckp->hdr.next_offset += chnk_size;
     		//update free space indicator        		
     		dbuckp->hdr.freespace -= chnk_size;
     		
     		// update bucket header: chunk counter
               	dbuckp->hdr.no_chunks++;

               	#ifdef DEBUGGING                 	
               	cout<<"dirchunk : "<<dir_i->gethdr().id.getcid()<<" just placed in a DiskBucket.\n";
               	cout<<"freespace = "<<dbuckp->hdr.freespace<<endl;
               	cout<<"no chunks = "<<dbuckp->hdr.no_chunks<<endl;
               	cout<<"next offset = "<<dbuckp->hdr.next_offset<<endl;
               	//cout<<"current offset = "<<curr_offs<<endl;
                cout<<"---------------------\n";
                #endif								

                delete chnkp; //free up memory
        }//end for

        // for each data chunk of this subtree
        for(vector<DataChunk>::const_iterator data_i = dataVectp->begin();
            data_i != dataVectp->end(); data_i++){
		// loop invariant: a DataChunk is stored in each iteration in the body
		//		   of the DiskBucket

		//ASSERTION 2: there is free space to store this data chunk
		if(data_i->gethdr().size > dbuckp->hdr.freespace)
			throw "AccessManager::_storeBreadth1stInDiskBucket ==> ASSERTION 2: DataChunk does not fit in DiskBucket!\n";		

      		// update bucket directory (chunk slots begin from slot 0)
      		dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = dbuckp->hdr.next_offset;
      		dbuckp->hdr.freespace -= sizeof(DiskBucket::dirent_t);
        		
      		// convert DataChunk to DiskDataChunk
      		DiskDataChunk* chnkp = 0;
      		try{
                        chnkp = dataChunk2DiskDataChunk(*data_i, numFacts, maxDepth);	      		
	      	}
         	catch(const char* message) {
         		string msg("AccessManager::_storeBreadth1stInDiskBucket==>");
         		msg += message;
         		throw msg.c_str();
         	}		
	      	
         	// Now, place the parts of the DiskDataChunk into the body of the DiskBucket
      		size_t chnk_size = 0;
      		size_t hdr_size = 0;
      		try{      		
                        placeDiskDataChunkInBcktBody(chnkp, maxDepth, nextFreeBytep, hdr_size, chnk_size);					
		}
         	catch(const char* message) {
         		string msg("");
         		msg += message;
         		throw msg.c_str();
         	}		
         	#ifdef DEBUGGING
                        //ASSERTION 2.1 : no chunk size mismatch         	
                        if(data_i->gethdr().size != chnk_size)
                                throw "AccessManager::_storeBreadth1stInDiskBucket ==> ASSERTION 1.2: DataChunk size mismatch!\n";		
         	#endif

                //update next free byte offset indicator
     		dbuckp->hdr.next_offset += chnk_size;        		
     		//update free space indicator
     		dbuckp->hdr.freespace -= chnk_size;
     		
     		// update bucket header: chunk counter        		
               	dbuckp->hdr.no_chunks++;

               	#ifdef DEBUGGING                 	
               	cout<<"datachunk : "<<data_i->gethdr().id.getcid()<<" just placed in a DiskBucket.\n";
               	cout<<"freespace = "<<dbuckp->hdr.freespace<<endl;
               	cout<<"no chunks = "<<dbuckp->hdr.no_chunks<<endl;
               	cout<<"next offset = "<<dbuckp->hdr.next_offset<<endl;
               	//cout<<"current offset = "<<curr_offs<<endl;
                cout<<"---------------------\n";
                #endif								

                delete chnkp; //free up memory
        }//end for
}//end of AccessManager::_storeBreadth1stInDiskBucket

void _storeDepth1stInDiskBucket(unsigned int maxDepth, unsigned int numFacts,
		 const vector<DirChunk>*dirVectp, const vector<DataChunk>*dataVectp,
		 DiskBucket* const dbuckp, char* &nextFreeBytep)
// precondition:
//      dirVectp and dataVectp (input parameters) contain the chunks of a single tree
//	that can fit in a single DiskBucket. These chunks have been placed in the 2 vectors with a
//      call to AccessManager::descendDepth1stCostTree. dbuckp (input parameter) is a const pointer to an
//      allocated DiskBucket, where its header members have been initialized. Finally, nextFreeBytep is
//      a byte pointer (input+output parameter) that points at the beginning of the DiskBucket's body.
// postcondition:
//      the chunks of the two vectors have been placed in the body of the bucket with respect to a
//      depth first traversal of the tree. The nextFreeBytep pointer points at the first free byte in the
//      body of the bucket.
{

        //for each dirchunk in the vector, store chunks in the row until
        // you store a (max depth-1) dir chunk. Then you have to continue selecting chunks from the
        // data chunk vector, in order to get the corresponding data chunks (under the same father).
        // After that we continue from the dir chunk vector at the point we were left.
        for(vector<DirChunk>::const_iterator dir_i = dirVectp->begin();
                                                dir_i != dirVectp->end(); dir_i++){
		// loop invariant: a DirChunk is stored in each iteration in the body
		//		   of the DiskBucket. If this DirChunk has a maxDepth-1 depth
		//                 then we also store all the DataChunks that are its children.

		//ASSERTION 1: there is free space to store this dir chunk
		if(dir_i->gethdr().size > dbuckp->hdr.freespace)
			throw "AccessManager::_storeDepth1stInDiskBucket ==> ASSERTION 1: DirChunk does not fit in DiskBucket!\n";		

      		// update bucket directory (chunk slots begin from slot 0)
      		dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = dbuckp->hdr.next_offset;
      		dbuckp->hdr.freespace -= sizeof(DiskBucket::dirent_t);
        		
      		// convert DirChunk to DiskDirChunk
      		DiskDirChunk* chnkp = 0;
      		try{
                        chnkp = dirChunk2DiskDirChunk(*dir_i, maxDepth);
	      	}
         	catch(const char* message) {
         		string msg("AccessManager::_storeDepth1stInDiskBucket==>");
         		msg += message;
         		throw msg.c_str();
         	}		
	      	
         	// Now, place the parts of the DiskDirChunk into the body of the DiskBucket
      		size_t chnk_size = 0;
      		size_t hdr_size = 0;
      		try{      		
                        placeDiskDirChunkInBcktBody(chnkp, maxDepth, nextFreeBytep, hdr_size, chnk_size);							
		}
         	catch(const char* message) {
         		string msg("AccessManager::_storeDepth1stInDiskBucket==>");
         		msg += message;
         		throw msg.c_str();
         	}	
         	#ifdef DEBUGGING
                        //ASSERTION 1.1 : no chunk size mismatch         	
                        if(dir_i->gethdr().size != chnk_size)
                                throw "AccessManager::_storeDepth1stInDiskBucket ==> ASSERTION 1.1: DirChunk size mismatch!\n";		
         	#endif
         		
                //update next free byte offset indicator
     		dbuckp->hdr.next_offset += chnk_size;
     		//update free space indicator        		
     		dbuckp->hdr.freespace -= chnk_size;
     		
     		// update bucket header: chunk counter
               	dbuckp->hdr.no_chunks++;

               	#ifdef DEBUGGING                 	
               	cout<<"dirchunk : "<<dir_i->gethdr().id.getcid()<<" just placed in a DiskBucket.\n";
               	cout<<"freespace = "<<dbuckp->hdr.freespace<<endl;
               	cout<<"no chunks = "<<dbuckp->hdr.no_chunks<<endl;
               	cout<<"next offset = "<<dbuckp->hdr.next_offset<<endl;
               	//cout<<"current offset = "<<curr_offs<<endl;
                cout<<"---------------------\n";
                #endif								

                delete chnkp; //free up memory

                //if this is a dir chunk at max depth
                if((*dir_i).gethdr().depth ==  maxDepth-1){
                        //then we have to continue selecting chunks from the DataChunk vector

                        //find all data chunks in data chunk vect with a prefix in their chunk id
                        //equal with *dir_i.gethdr().id
                        //for each datachunk in the vector
                        for(vector<DataChunk>::const_iterator data_i = dataVectp->begin();
                            data_i != dataVectp->end(); data_i++){
                                //if we have a prefix match
	                        if( (*data_i).gethdr().id.getcid().find((*dir_i).gethdr().id.getcid())
	                                 != string::npos ) {

                         		// loop invariant: a DataChunk is stored in each iteration in the body
                         		//		   of the DiskBucket. All these DataChunks have the same
                         		//                 parent DirChunk

                         		//ASSERTION 2: there is free space to store this data chunk
                         		if(data_i->gethdr().size > dbuckp->hdr.freespace)
                         			throw "AccessManager::_storeDepth1stInDiskBucket ==> ASSERTION 2: DataChunk does not fit in DiskBucket!\n";		

                               		// update bucket directory (chunk slots begin from slot 0)
                               		dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = dbuckp->hdr.next_offset;
                               		dbuckp->hdr.freespace -= sizeof(DiskBucket::dirent_t);
                                 		
                               		// convert DataChunk to DiskDataChunk
                               		DiskDataChunk* chnkp = 0;
                               		try{
                                                 chnkp = dataChunk2DiskDataChunk(*data_i, numFacts, maxDepth);	      		
                         	      	}
                                  	catch(const char* message) {
                                  		string msg("AccessManager::_storeDepth1stInDiskBucket==>");
                                  		msg += message;
                                  		throw msg.c_str();
                                  	}		
                         	      	
                                  	// Now, place the parts of the DiskDataChunk into the body of the DiskBucket
                               		size_t chnk_size = 0;
                               		size_t hdr_size = 0;
                               		try{      		
                                                 placeDiskDataChunkInBcktBody(chnkp, maxDepth,
                                                        nextFreeBytep, hdr_size, chnk_size);					
                         		}
                                  	catch(const char* message) {
                                  		string msg("");
                                  		msg += message;
                                  		throw msg.c_str();
                                  	}		
                                	#ifdef DEBUGGING
                                               //ASSERTION 2.1 : no chunk size mismatch         	
                                               if(data_i->gethdr().size != chnk_size)
                                                       throw "AccessManager::_storeDepth1stInDiskBucket ==> ASSERTION 2.1: DataChunk size mismatch!\n";		
                                	#endif

                                         //update next free byte offset indicator
                              		dbuckp->hdr.next_offset += chnk_size;        		
                              		//update free space indicator
                              		dbuckp->hdr.freespace -= chnk_size;
                              		
                              		// update bucket header: chunk counter        		
                                       	dbuckp->hdr.no_chunks++;

                                       	#ifdef DEBUGGING                 	
                                       	cout<<"datachunk : "<<data_i->gethdr().id.getcid()<<" just placed in a DiskBucket.\n";
                                       	cout<<"freespace = "<<dbuckp->hdr.freespace<<endl;
                                       	cout<<"no chunks = "<<dbuckp->hdr.no_chunks<<endl;
                                       	cout<<"next offset = "<<dbuckp->hdr.next_offset<<endl;
                                       	//cout<<"current offset = "<<curr_offs<<endl;
                                        cout<<"---------------------\n";
                                        #endif								

                                        delete chnkp; //free up memory
                                }//end if
                        }//end for
                }//end if
        }//end for
}//end of AccessManager::_storeDepth1stInDiskBucket

//****************************************************		


DiskDirChunk* dirChunk2DiskDirChunk(const DirChunk& dirchnk, unsigned int maxDepth)
// precondition:
//	dirchnk is a DirChunk instance filled with valid entries. maxDepth is the maximum depth of
//      the cube in question.
// postcondition:
//	A DiskDirChunk has been allocated in heap space that contains the values of
//	dirchnk and a pointer to it is returned.	
{
	//ASSERTION 1.0 : depth and chunk-id compatibility
	if(dirchnk.gethdr().depth != dirchnk.gethdr().id.getChunkDepth())
		throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION 1.0: depth and chunk-id mismatch\n";	
        //ASSERTION 1.1: this is not the root chunk
        if(dirchnk.gethdr().depth == Chunk::MIN_DEPTH)
		throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION 1.1: this method cannot handle the root chunk\n";	
        //ASSERTION 1.2: this is not a data chunk
        if(dirchnk.gethdr().depth == maxDepth)
		throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION 1.2: this method cannot handle a data chunk\n";			
        //ASSERTION 1.3: valid depth value
        if(dirchnk.gethdr().depth < Chunk::MIN_DEPTH || dirchnk.gethdr().depth > maxDepth)
		throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION 1.3: invalid depth value\n";	
		
	//allocate new DiskDirChunk
        DiskDirChunk* chnkp=0;
	try{
		chnkp = new DiskDirChunk;
	}
	catch(bad_alloc){
		throw "AccessManager::dirChunk2DiskDirChunk ==> cant allocate space for new DiskDirChunk!\n";		
	}	
		
	// 1. first copy the headers
	
	//insert values for non-pointer members
	chnkp->hdr.depth = dirchnk.gethdr().depth;
	chnkp->hdr.no_dims = dirchnk.gethdr().numDim;		
	chnkp->hdr.no_measures = 0; // this is a directory chunk
	chnkp->hdr.no_entries = dirchnk.gethdr().totNumCells;
	
	// store the chunk id
	//allocate space for the domains	
	try{	
		chnkp->hdr.chunk_id = new DiskChunkHeader::Domain_t[chnkp->hdr.depth];
	}
	catch(bad_alloc){
		throw "AccessManager::dirChunk2DiskDirChunk ==> cant allocate space for the domains!\n";		
	}	
	
	//ASSERTION2 : valid no_dims value
	if(chnkp->hdr.no_dims <= 0)
		throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION2: invalid num of dims\n";	
        //ASSERTION 2.1: num of dims and chunk id compatibility		
        bool isroot = false;		
        if(chnkp->hdr.no_dims != dirchnk.gethdr().id.getChunkNumOfDim(isroot))
                throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION 2.1: num of dims and chunk id mismatch\n";	
        if(isroot)
                throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION 2.1: root chunk encountered\n";	
		
	//allocate space for each domain's order-codes		
	for(int i = 0; i<chnkp->hdr.depth; i++) { //for each domain of the chunk id
		try{
			chnkp->hdr.chunk_id[i].ordercodes = new DiskChunkHeader::ordercode_t[chnkp->hdr.no_dims];
		}
         	catch(bad_alloc){
         		throw "AccessManager::dirChunk2DiskDirChunk ==> cant allocate space for the ordercodes!\n";		
         	}			
	}//end for
	
	string cid = dirchnk.gethdr().id.getcid();	
	string::size_type begin = 0;
	for(int i=0; i<chnkp->hdr.depth; i++) { //for each domain of the chunk id
		//get the appropriate substring
		string::size_type end = cid.find(".", begin); // get next "."
		// if end==npos then no "." found, i.e. this is the last domain
		// end-begin == the length of the domain substring => substring cid[begin]...cid[begin+(end-begin)-1]		
		string domain = (end == string::npos) ?
		                        string(cid, begin, cid.length()-begin) : string(cid, begin, end-begin);				
		
                string::size_type b = 0;		
		for(int j =0; j<chnkp->hdr.no_dims; j++){ //for each order-code of the domain			
			string::size_type e = domain.find("|", b); // get next "|"
			string ocstr = (e == string::npos) ?
			                        string(domain, b, domain.length()-b) : string(domain, b, e-b);
			chnkp->hdr.chunk_id[i].ordercodes[j] = atoi(ocstr.c_str());			
			b = e+1;
		}//end for
		begin = end+1;			
	}//end for
	
	//store the order-code ranges
	//allocate space
	try{
		chnkp->hdr.oc_range = new DiskChunkHeader::OrderCodeRng_t[chnkp->hdr.no_dims];
	}
	catch(bad_alloc){
		throw "AccessManager::dirChunk2DiskDirChunk ==> cant allocate space for the oc ranges!\n";		
	}	
		
	int i=0;
	vector<LevelRange>::const_iterator iter = dirchnk.gethdr().vectRange.begin();
	//ASSERTION3: combatible vector length and no of dimensions
	if(chnkp->hdr.no_dims != dirchnk.gethdr().vectRange.size())
		throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION3: wrong length in vector\n";	
	while(i<chnkp->hdr.no_dims && iter != dirchnk.gethdr().vectRange.end()){
		if(iter->leftEnd != LevelRange::NULL_RANGE || iter->rightEnd != LevelRange::NULL_RANGE) {			
			chnkp->hdr.oc_range[i].left = iter->leftEnd;		
			chnkp->hdr.oc_range[i].right = iter->rightEnd;		
		}//end if
		//else leave the default null ranges (assigned by the constructor)
		i++;
		iter++;
	}//end while	
	
	// 2. Next copy the entries
	//ASSERTION 3.1: valid total number of cells
	if(dirchnk.gethdr().totNumCells <= 0)
		throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION 3.1: total No of cells is <= 0\n";			
	//allocate space for the entries
	try{
		chnkp->entry = new DiskDirChunk::DirEntry_t[dirchnk.gethdr().totNumCells];
	}
	catch(bad_alloc){
		throw "AccessManager::dirChunk2DiskDirChunk ==> cant allocate space for dir entries!\n";		
	}	
	
	i = 0;
	vector<DirEntry>::const_iterator ent_iter = dirchnk.getentry().begin();
	//ASSERTION4: combatible vector length and no of entries
	if(chnkp->hdr.no_entries != dirchnk.getentry().size())
		throw "AccessManager::dirChunk2DiskDirChunk ==> ASSERTION4: wrong length in vector\n";	
	while(i<chnkp->hdr.no_entries && ent_iter != dirchnk.getentry().end()){
	        chnkp->entry[i].bucketid = ent_iter->bcktId;
       	        chnkp->entry[i].chunk_slot = ent_iter->chnkIndex;
		i++;
		ent_iter++;	
	}//end while
	
	return chnkp;
}// end of AccessManager::dirChunk2DiskDirChunk

DiskDataChunk* dataChunk2DiskDataChunk(const DataChunk& datachnk, unsigned int numFacts,
						      unsigned int maxDepth)
// precondition:
//	datachnk is a DataChunk instance filled with valid entries. numFacts is the number of facts per cell
//      and maxDepth is the maximum chunking depth of the cube in question.
// postcondition:
//	A DiskDataChunk has been allocated in heap space that contains the values of
//	datachnk and a pointer to it is returned.	
{
	//ASSERTION1: input chunk is a data chunk
	if(datachnk.gethdr().depth != maxDepth)
		throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION1: wrong depth for data chunk\n";
		
	//allocate new DiskDataChunk
	DiskDataChunk* chnkp = 0;
	try{
		chnkp = new DiskDataChunk;
	}
	catch(bad_alloc){
		throw "AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for new DiskDataChunk!\n";		
	}	
		
	// 1. first copy the header
	
	//insert values for non-pointer members
	chnkp->hdr.depth = datachnk.gethdr().depth;
	chnkp->hdr.no_dims = datachnk.gethdr().numDim;		
	chnkp->hdr.no_measures = numFacts;
	chnkp->hdr.no_entries = datachnk.gethdr().totNumCells;
	
	// store the chunk id
	//ASSERTION 1.1 : depth and chunk-id compatibility
	if(datachnk.gethdr().depth != datachnk.gethdr().id.getChunkDepth())
		throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION 1.1: depth and chunk-id mismatch\n";		
	//allocate space for the domains	
	try{	
		chnkp->hdr.chunk_id = new DiskChunkHeader::Domain_t[chnkp->hdr.depth];
	}
	catch(bad_alloc){
		throw "AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for the domains!\n";		
	}	
	
	//ASSERTION2 : valid no_dims value
	if(chnkp->hdr.no_dims <= 0)
		throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION2: invalid num of dims\n";	
        //ASSERTION 2.1: num of dims and chunk id compatibility		
        bool isroot = false;		
        if(chnkp->hdr.no_dims != datachnk.gethdr().id.getChunkNumOfDim(isroot))
                throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION 2.1: num of dims and chunk id mismatch\n";	
        if(isroot)
                throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION 2.1: root chunk encountered\n";	
	//allocate space for each domain's order-codes		
	for(int i = 0; i<chnkp->hdr.depth; i++) { //for each domain of the chunk id
		try{
			chnkp->hdr.chunk_id[i].ordercodes = new DiskChunkHeader::ordercode_t[chnkp->hdr.no_dims];
		}
         	catch(bad_alloc){
         		throw "AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for the ordercodes!\n";		
         	}			
	}//end for
	
	string cid = datachnk.gethdr().id.getcid();	
	string::size_type begin = 0;
	for(int i=0; i<chnkp->hdr.depth; i++) { //for each domain of the chunk id
		//get the appropriate substring
		string::size_type end = cid.find(".", begin); // get next "."
		// if end==npos then no "." found, i.e. this is the last domain
		// end-begin == the length of the domain substring => substring cid[begin]...cid[begin+(end-begin)-1]		
		string domain = (end == string::npos) ?
		                        string(cid, begin, cid.length()-begin) : string(cid, begin, end-begin);				
		
                string::size_type b = 0;		
		for(int j =0; j<chnkp->hdr.no_dims; j++){ //for each order-code of the domain			
			string::size_type e = domain.find("|", b); // get next "|"
			string ocstr = (e == string::npos) ?
			                        string(domain, b, domain.length()-b) : string(domain, b, e-b);
			chnkp->hdr.chunk_id[i].ordercodes[j] = atoi(ocstr.c_str());			
			b = e+1;
		}//end for
		begin = end+1;			
	}//end for

	//store the order-code ranges
	//allocate space
	try{
		chnkp->hdr.oc_range = new DiskChunkHeader::OrderCodeRng_t[chnkp->hdr.no_dims];
	}
	catch(bad_alloc){
		throw "AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for the oc ranges!\n";		
	}	
		
	int i=0;
	vector<LevelRange>::const_iterator iter = datachnk.gethdr().vectRange.begin();
	//ASSERTION3: combatible vector length and no of dimensions
	if(chnkp->hdr.no_dims != datachnk.gethdr().vectRange.size())
		throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION3: wrong length in vector\n";	
	while(i<chnkp->hdr.no_dims && iter != datachnk.gethdr().vectRange.end()){
		if(iter->leftEnd != LevelRange::NULL_RANGE || iter->rightEnd != LevelRange::NULL_RANGE) {			
			chnkp->hdr.oc_range[i].left = iter->leftEnd;		
			chnkp->hdr.oc_range[i].right = iter->rightEnd;		
		}//end if
		//else leave the default null ranges (assigned by the constructor)
		i++;
		iter++;
	}//end while			
	
	// 2. Copy number of ace
	//ASSERTION 3.1: valid real and total number of cells
	if(datachnk.gethdr().totNumCells < datachnk.gethdr().rlNumCells)
		throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION 3.1: total No of cells is less than real No of cells!\n";		
	chnkp->no_ace = datachnk.gethdr().rlNumCells;
	
	// 3. Next copy the bitmap
	//ASSERTION 4: combatible bitmap length and no of entries
	if(chnkp->hdr.no_entries != datachnk.getcomprBmp().size())
		throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION 4: bitmap size and No of entries mismatch\n";	
	
	//allocate space for the bitmap
	try{
		//chnkp->bitmap = new WORD[::numOfwords(datachnk.gethdr().totNumCells)];
		chnkp->allocBmp(::numOfWords(chnkp->hdr.no_entries));
	}
	catch(bad_alloc){
		throw "AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for the bitmap!\n";		
	}	
	//bitmap initialization
        int bit = 0;
        for(bit_vector::const_iterator iter = datachnk.getcomprBmp().begin();
            iter != datachnk.getcomprBmp().end(); iter++){
                (*iter == true) ? chnkp->setbit(bit):chnkp->clearbit(bit);
                bit++;
        }
							
	// 4. Next copy the entries
	//allocate space for the entries
	try{
		chnkp->entry = new DiskDataChunk::DataEntry_t[chnkp->no_ace];
	}
	catch(bad_alloc){
		throw "AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for data entries!\n";		
	}	
	
	i = 0;
	vector<DataEntry>::const_iterator ent_iter = datachnk.getentry().begin();
	//ASSERTION5: combatible vector length and no of entries
	if(chnkp->no_ace != datachnk.getentry().size())
		throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION4: wrong length in vector\n";	
	while(i<chnkp->no_ace && ent_iter != datachnk.getentry().end()){
		//allocate space for the measures
         	try{
         		chnkp->entry[i].measures = new measure_t[int(chnkp->hdr.no_measures)];
         	}
         	catch(bad_alloc){
         		throw "AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for data entries!\n";
         	}	
		
         	//store the measures of this entry				
       		//ASSERTION6: combatible vector length and no of measures
		if(chnkp->hdr.no_measures != ent_iter->fact.size())
			throw "AccessManager::dataChunk2DiskDataChunk ==> ASSERTION6: wrong length in vector\n";	         	
		int j=0;
		vector<measure_t>::const_iterator m_iter = ent_iter->fact.begin();
		while(j<chnkp->hdr.no_measures && m_iter != ent_iter->fact.end()){
			chnkp->entry[i].measures[j] = *m_iter;
			j++;
			m_iter++;		
		}//end while
		i++;
		ent_iter++;
	}//end while
	
	return chnkp;
}// end of AccessManager::dataChunk2DiskDataChunk

void placeDiskDirChunkInBcktBody(const DiskDirChunk* const chnkp, unsigned int maxDepth,
					char* &currentp, size_t& hdr_size, size_t& chnk_size)
// precondition:
//		chnkp points at a DiskDirChunk structure && currentp is a byte pointer pointing in the
//		body of a DiskBucket at the point, where the DiskDirChunk must be placed.maxDepth
//		gives the maximum depth of the cube in question and it is used for confirming that this
//		is a data chunk.
// postcondition:
//		the DiskDirChunk has been placed in the body && currentp points at the next free byte in
//		the body && chnk_size contains the bytes consumed by the placement of the DiskDirChunk &&
//              hdr_size contains the bytes consumed by the DiskChunkHeader.		
{
        // init size counters
        chnk_size = 0;
        hdr_size = 0;

	//ASSERTION1: input pointers are not null
	if(!chnkp || !currentp)
		throw "AccessManager::placeDiskDirChunkInBcktBody ==> ASSERTION1: null pointer\n";

	//get a const pointer to the DiskChunkHeader
	const DiskChunkHeader* const hdrp = &chnkp->hdr;
	
	//ASSERTION 1.1: not out of range for depth
	if(hdrp->depth < Chunk::MIN_DEPTH || hdrp->depth > maxDepth)
		throw "AccessManager::placeDiskDirChunkInBcktBody ==> ASSERTION 1.1: depth out of range for a dir chunk\n";	
		
	//ASSERTION 1.2: not the root chunk
	if(hdrp->depth == Chunk::MIN_DEPTH)
		throw "AccessManager::placeDiskDirChunkInBcktBody ==> ASSERTION 1.2: depth denotes the root chunk!\n";	
		
	//ASSERTION 1.3: not a data chunk
	if(hdrp->depth == maxDepth)
		throw "AccessManager::placeDiskDirChunkInBcktBody ==> ASSERTION 1.3 depth denotes a data chunk!\n";	
				
	//begin by placing the static part of a DiskDirchunk structure
	memcpy(currentp, reinterpret_cast<char*>(chnkp), sizeof(DiskDirChunk));
	currentp += sizeof(DiskDirChunk); // move on to the next empty position
	chnk_size += sizeof(DiskDirChunk); // this is the size of the static part of a DiskDirChunk
	hdr_size += sizeof(DiskChunkHeader); // this is the size of the static part of a DiskChunkHeader
			
	//continue with placing the chunk id
	//ASSERTION2: chunkid is not null
	if(!hdrp->chunk_id)
		throw "AccessManager::placeDiskDirChunkInBcktBody ==> ASSERTION2: null pointer\n";	
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
			throw "AccessManager::placeDiskDirChunkInBcktBody ==> ASSERTION3: null pointer\n";			
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
		throw "AccessManager::placeDiskDirChunkInBcktBody ==> ASSERTION4: null pointer\n";		
	for(int i = 0; i < hdrp->no_dims; i++) {
	//loop invariant: store an order code range structure
		DiskChunkHeader::OrderCodeRng_t* rngp = &(hdrp->oc_range)[i];
       		memcpy(currentp, reinterpret_cast<char*>(rngp), sizeof(DiskChunkHeader::OrderCodeRng_t));
       		currentp += sizeof(DiskChunkHeader::OrderCodeRng_t); // move on to the next empty position
       		hdr_size += sizeof(DiskChunkHeader::OrderCodeRng_t);		
       		chnk_size += sizeof(DiskChunkHeader::OrderCodeRng_t);		       		
	}//end for
		
	//finally place the dir entries
       	for(int i =0; i<chnkp->hdr.no_entries; i++) { //for each entry
	       	DiskDirChunk::DirEntry_t*ep = &chnkp->entry[i];       		
        	memcpy(currentp, reinterpret_cast<char*>(ep), sizeof(DiskDirChunk::DirEntry_t));
        	currentp += sizeof(DiskDirChunk::DirEntry_t); // move on to the next empty position
        	chnk_size += sizeof(DiskDirChunk::DirEntry_t);		       	
       	}//end for  						
}// end of AccessManager::placeDiskDirChunkInBcktBody      		

void placeDiskDataChunkInBcktBody(const DiskDataChunk* const chnkp, unsigned int maxDepth,
			char* &currentp,size_t& hdr_size, size_t& chnk_size)
// precondition:
//		chnkp points at a DiskDataChunk structure && currentp is a byte pointer pointing in the
//		body of a DiskBucket at the point, where the DiskDataChunk must be placed. maxDepth
//		gives the maximum depth of the cube in question and it is used for confirming that this
//		is a data chunk.
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
		throw "AccessManager::placeDiskDataChunkInBcktBody ==> ASSERTION1: null pointer\n";

	//get a const pointer to the DiskChunkHeader
	const DiskChunkHeader* const hdrp = &chnkp->hdr;
	
	//ASSERTION 1.1: this is a data chunk
	if(hdrp->depth != maxDepth)
		throw "AccessManager::placeDiskDataChunkInBcktBody ==> ASSERTION 1.1: chunk's depth != maxDepth in Data Chunk\n";	
		
	//begin by placing the static part of a DiskDatachunk structure
	memcpy(currentp, reinterpret_cast<char*>(chnkp), sizeof(DiskDataChunk));
	currentp += sizeof(DiskDataChunk); // move on to the next empty position
	chnk_size += sizeof(DiskDataChunk); // this is the size of the static part of a DiskDataChunk
	hdr_size += sizeof(DiskChunkHeader); // this is the size of the static part of a DiskChunkHeader
			
	//continue with placing the chunk id
	//ASSERTION2: chunkid is not null
	if(!hdrp->chunk_id)
		throw "AccessManager::placeDiskDataChunkInBcktBody ==> ASSERTION2: null pointer\n";	
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
			throw "AccessManager::placeDiskDataChunkInBcktBody ==> ASSERTION3: null pointer\n";			
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
		throw "AccessManager::placeDiskDataChunkInBcktBody ==> ASSERTION4: null pointer\n";		
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
}// end of AccessManager::placeDiskDataChunkInBcktBody      		