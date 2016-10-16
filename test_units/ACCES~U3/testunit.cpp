////////////////////////////////////////////////////
// this program's target is to test the unit:
// AccessManager::createDataChunkRegionDiskBucketInHeap
//
// (C) Nikos Karayannidis               9/10/2001
////////////////////////////////////////////////////
#include <map>
#include <string>
#include <fstream>

#include "classes.h"
#include "functions.h"
#include "DiskStructures.h"
#include "Exceptions.h"

void createDataChunkRegionDiskBucketInHeap(
			unsigned int maxDepth,
			unsigned int numFacts,
			const vector<CostNode*>& dataChunksOfregion,
			const BucketID& bcktID,
			const string& factFile,
			DiskBucket* &dbuckp,
			map<ChunkID, DirEntry>& resultMap);
                                				
main()
{
        ofstream errorlog("error.log", ios::app);

        unsigned int max_depth = 3;
        unsigned int numFacts = 2;
        int numDims = 2;

        //create the cost trees of a region
        vector<CostNode*> dataChunksOfRegion;
        createDataChunksOfRegion(max_depth, numFacts, numDims, dataChunksOfRegion);

        // create a bucket id
        BucketID bid(15);

        //call the procedure for testing
       	map<ChunkID, DirEntry> resultMap;
        DiskBucket* dbuckp = 0;
        try{
                createDataChunkRegionDiskBucketInHeap(max_depth, numFacts, dataChunksOfRegion, bid,
                                                string("facts2.fld"), dbuckp, resultMap);
        }
        catch(GeneralError& error) {
               GeneralError e("main ==> ");
               error += e;
               errorlog << error;
               cerr<<"PROGRAM TERMINATION! See error.log\n";
               exit(0);
        }

        #ifdef DEBUGGING
 	//print the contents of the created bucket in a separate file
         try{
                 printDiskBucketContents_SingleTreeAndCluster(dbuckp, max_depth);
         }
       	catch(GeneralError& error) {
       		GeneralError e("main ==> ");
       		error += e;
               errorlog << error;
               cerr<<"PROGRAM TERMINATION! See error.log\n";
               exit(0);
       	}
        #endif

        //print result
        printResult(resultMap);
}//end main

void createDataChunkRegionDiskBucketInHeap(
			unsigned int maxDepth,
			unsigned int numFacts,
			const vector<CostNode*>& dataChunksOfregion,
			const BucketID& bcktID,
			const string& factFile,
			DiskBucket* &dbuckp,
			map<ChunkID, DirEntry>& resultMap)
//precondition:
//	the cost-nodes in dataChunksOfregion belong to the same region (cluster) and each
//	has a size-cost less than the bucket threshold. dbuckp is a NULL pointer
//	and resultMap is empty.
//postcondition:
//	dbuckp points at a DiskBucket that contains the corresponding data chunks that have been
//	loaded from the input factFile and stored in a bucket as different subtrees.
//	The resultMap contains the DirEntries corresponding to the data chunks.
{
	//ASSERTION1: dbuckp points to NULL
	if(dbuckp)
		throw GeneralError("AccessManager::createDataChunkRegionDiskBucketInHeap ==> ASSERTION1: input pointer to DiskBucket should be null\n");
        //ASSERTION2: non empty input vector
        if(dataChunksOfregion.empty())
                throw GeneralError("AccessManager::createDataChunkRegionDiskBucketInHeap ==> ASSERTION2: input vector is empty\n");
        //ASSERTION3: NOT too many subtrees in a single cluster
        if(dataChunksOfregion.size() > DiskBucketHeader::subtreemaxno)
                throw GeneralError("AccessManager::createDataChunkRegionDiskBucketInHeap ==> ASSERTION3: too many subtrees to store in a cluster\n");

	// allocate DiskBucket in heap
	dbuckp = new DiskBucket;

	// initialize bucket directory pointer to point one beyond last byte of body
	dbuckp->offsetInBucket = reinterpret_cast<DiskBucket::dirent_t*>(&(dbuckp->body[DiskBucket::bodysize]));

	// initialize the DiskBucketHeader
	dbuckp->hdr.id.rid = bcktID.rid; // store the bucket id
	// Init the links to other buckets with null ids
//////////////////////////////////////////////////////////////	
	//dbuckp->hdr.next.rid = serial_t::null;
	//dbuckp->hdr.previous.rid = serial_t::null;
        dbuckp->hdr.next.rid = 0;
	dbuckp->hdr.previous.rid = 0;
//////////////////////////////////////////////////////////////	
	
	dbuckp->hdr.no_chunks = 0;  	//init chunk counter
	dbuckp->hdr.next_offset = 0; //next free byte offset in the body
	dbuckp->hdr.freespace = DiskBucket::bodysize;//init free space counter
	dbuckp->hdr.no_ovrfl_next = 0; // no overflow-bucket  chain used

	dbuckp->hdr.no_subtrees = 0; // init subtree counter
	char* nextFreeBytep = dbuckp->body; //init current byte pointer
	//for each cost-node of the region...
	for(vector<CostNode*>::const_iterator chunk_iter = dataChunksOfregion.begin();
					chunk_iter != dataChunksOfregion.end(); chunk_iter++) {

		//create the corresponding data chunk from the input file
        	//read the input file and create a DataChunk instance
        	vector<DataChunk>* const dataVectp = new vector<DataChunk>;
                vector<DirChunk>* const dirVectp = 0; // no pointer to DirChunk vector passed in this case
        	dataVectp->reserve(1);
                try {
        		descendDepth1stCostTree( maxDepth,
                                                 numFacts,
                                                 *chunk_iter,
                                                 factFile,
                                                 bcktID,
                                                 dirVectp,
                                                 dataVectp);
                }
                catch(GeneralError& error) {
                       GeneralError e("AccessManager::createDataChunkRegionDiskBucketInHeap ==> ");
                       error += e;
                       throw error;
                }

              	//ASSERTION 3: valid returned vectors
               	if(dataVectp->size() != 1)
        		throw GeneralError("AccessManager::createDataChunkRegionDiskBucketInHeap ==> ASSERTION 3: error in creating the DataChunk instance from file\n");

		//update subtree counter in bucket header
		dbuckp->hdr.no_subtrees++; //one more tree will be added
		//update subtree-directory with the chunk-directory entry (i.e. chunk slot) corresponding
		//to this tree.
		dbuckp->hdr.subtree_dir_entry[dbuckp->hdr.no_subtrees-1] = dbuckp->hdr.no_chunks;
		
        	// place the data chunk in the bucket's body		
                try{
        	        placeSingleDataChunkInDiskBucketBody(maxDepth, numFacts,
        		                        *(dataVectp->begin()), dbuckp, nextFreeBytep);
                }
               	catch(GeneralError& error) {
               		GeneralError e("AccessManager::createDataChunkRegionDiskBucketInHeap ==> ");
               		error += e;
               		throw error;
               	}							
		
                //insert new entry in result map that holds chunk-id to DirEntry associations. The chunk id
                // corresponds to the data chunk.
		//ASSERTION 4: first assert that this is the first time we insert this chunk id
		ChunkID chnkid = (*chunk_iter)->getchunkHdrp()->id;
		if(resultMap.find(chnkid) != resultMap.end())
			throw GeneralError("AccessManager::createDataChunkRegionDiskBucketInHeap ==>ASSERTION 4: Duplicate chunk id for data chunk in cluster\n");
		DirEntry dirent(bcktID, dbuckp->hdr.subtree_dir_entry[dbuckp->hdr.no_subtrees-1]); // create the DirEntry consisting of the bucket
												   // id and the chunk slot corresponding
												   // to this data chunk
		resultMap[chnkid] = dirent;

               	// free up memory
        	//delete dirVectp;
        	delete dataVectp;
	}//end for	
}//AccessManager::createDataChunkRegionDiskBucketInHeap	