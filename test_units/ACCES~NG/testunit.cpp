////////////////////////////////////////////////////
// this program's target is to test the unit:
// AccessManager::createTreeRegionDiskBucketInHeap
//
// (C) Nikos Karayannidis               11/9/2001
////////////////////////////////////////////////////
#include <map>
#include <string>

#include "classes.h"
#include "functions.h"
#include "DiskStructures.h"

void traverseSingleCostTreeCreateChunkVectors(
        				unsigned int maxDepth,
        				unsigned int numFacts,
        				const CostNode* const costRoot,
        				const string& factFile,
        				const BucketID& bcktID,
        				vector<DirChunk>* &dirVectp,
        				vector<DataChunk>* &dataVectp,
					const TreeTraversal_t howToTraverse = breadthFirst);

void placeChunksOfSingleTreeInDiskBucketBody(
		unsigned int maxDepth,
		unsigned int numFacts,
		const vector<DirChunk>* const dirVectp,
		const vector<DataChunk>* const dataVectp,
		DiskBucket* const dbuckp,
		char* &nextFreeBytep,
		const TreeTraversal_t howToTraverse = breadthFirst);



void createTreeRegionDiskBucketInHeap(
                        unsigned int maxDepth,
                        unsigned int numFacts,
                        const vector<CostNode*>& treesOfregion,
                        const BucketID& buckid,
                        const string& factFile,
                        DiskBucket* &dbuckp,
                        map<ChunkID, DirEntry>& resultMap,
                        const TreeTraversal_t howToTraverse = breadthFirst
                        );

main()
{
        unsigned int max_depth = 3;
        unsigned int numFacts = 2;
        int numDims = 2;

        //create the cost trees of a region
        vector<CostNode*> treesOfRegion;
        createTreesOfRegion(treesOfRegion, max_depth, numFacts, numDims);

        // create a bucket id
        BucketID bid(15);

        TreeTraversal_t howToTraverse;
        cout<<"Choose traversal method (1 breadth1st, 2 depth1st) : ";
        int answr;
        cin>>answr;
        (answr == 1) ? howToTraverse = breadthFirst : howToTraverse = depthFirst;


        //call the procedure for testing
       	map<ChunkID, DirEntry> resultMap;
        DiskBucket* dbuckp = 0;
        try{
                createTreeRegionDiskBucketInHeap(max_depth, numFacts, treesOfRegion, bid,
                                                string("facts.fld"), dbuckp, resultMap, howToTraverse);
        }
        catch(const char* message) {
                string msg("AccessManager::storeTreesInCUBE_FileClusters ==> ");
                msg += message;
                cerr<< msg.c_str()<<endl;
                exit(0);
        }

        //print result
        printResult(dbuckp, resultMap);
}//end main

void createTreeRegionDiskBucketInHeap(
			unsigned int maxDepth,
			unsigned int numFacts,
			const vector<CostNode*>& treesOfregion,
			const BucketID& bcktID,
			const string& factFile,
			DiskBucket* &dbuckp,
			map<ChunkID, DirEntry>& resultMap,
			const TreeTraversal_t howToTraverse)
//precondition:
//	the cost-trees in treesOfregion belong to the same region (cluster) and each
//	has a size-cost less than the bucket threshold. dbuckp is a NULL pointer
//	and resultMap is empty.
//postcondition:
//	dbuckp points at a DiskBucket that contains the corresponding chunk trees that have been
//	loaded from the input factFile. The resultMap contains the DirEntries corresponding to the trees.
{
	//ASSERTION1: dbuckp points to NULL
	if(dbuckp)
		throw "AccessManager::createTreeRegionDiskBucketInHeap ==> ASSERTION1: input pointer to DiskBucket should be null\n";
        //ASSERTION2: non empty input vector
        if(treesOfregion.empty())
                throw "AccessManager::createTreeRegionDiskBucketInHeap ==> ASSERTION2: input vector is empty\n";
        //ASSERTION3: NOT too many subtrees in a single cluster
        if(treesOfregion.size() > DiskBucketHeader::subtreemaxno)
                throw "AccessManager::createTreeRegionDiskBucketInHeap ==> ASSERTION3: too many subtrees to store in a cluster\n";

	// allocate DiskBucket in heap
	dbuckp = new DiskBucket;

	// initialize bucket directory pointer to point one beyond last byte of body
	dbuckp->offsetInBucket = reinterpret_cast<DiskBucket::dirent_t*>(&(dbuckp->body[DiskBucket::bodysize]));

	// initialize the DiskBucketHeader
	dbuckp->hdr.id.rid = bcktID.rid; // store the bucket id
	// Init the links to other buckets with null ids
//*************************************************************************
// COMMENT THIS OUT WHEN cOPIED TO SISYPHUS SOURCES
	//dbuckp->hdr.next.rid = serial_t::null;
	//dbuckp->hdr.previous.rid = serial_t::null;
	dbuckp->hdr.next.rid = 0;
	dbuckp->hdr.previous.rid = 0;
//**************************************************************************
	dbuckp->hdr.no_chunks = 0;  	//init chunk counter
	dbuckp->hdr.next_offset = 0; //next free byte offset in the body
	dbuckp->hdr.freespace = DiskBucket::bodysize;//init free space counter
	dbuckp->hdr.no_ovrfl_next = 0; // no overflow-bucket  chain used

	dbuckp->hdr.no_subtrees = 0; // init subtree counter
	char* nextFreeBytep = dbuckp->body; //init current byte pointer
	//for each cost-tree of the region...
	for(vector<CostNode*>::const_iterator tree_iter = treesOfregion.begin();
					tree_iter != treesOfregion.end(); tree_iter++) {

		//create the corresponding dir/data chunks from the input file
		//according to the traversal method
        	//  In order to create a DiskBucket we will need a vector of DirChunks, a vector of DataChunks
        	vector<DirChunk>* dirVectp = 0;
        	vector<DataChunk>* dataVectp = 0;
        	// traverse the cost-tree and fill these vectors with chunks, loaded from the fact input file
        	try{
                        traverseSingleCostTreeCreateChunkVectors(
                        				maxDepth,
							numFacts,
                        				*tree_iter,
                        				factFile,
                        				bcktID,
                        				dirVectp,
                        				dataVectp,
                        				howToTraverse);
        	}
               	catch(const char* message) {
               		string msg("AccessManager::createTreeRegionDiskBucketInHeap ==> ");
               		msg += message;
               		throw msg;
               	}

		//update subtree counter in bucket header
		dbuckp->hdr.no_subtrees++; //one more tree will be added
		//update subtree-directory with the chunk-directory entry (i.e. chunk slot) corresponding
		//to this tree.
		dbuckp->hdr.subtree_dir_entry[dbuckp->hdr.no_subtrees-1] = dbuckp->hdr.no_chunks;

                //According to the desired traversal method store the chunks in the body of the bucket
                try{
        	        placeChunksOfSingleTreeInDiskBucketBody(maxDepth, numFacts,
        		                        dirVectp, dataVectp, dbuckp, nextFreeBytep,howToTraverse);
                }
               	catch(const char* message) {
               		string msg("AccessManager::createTreeRegionDiskBucketInHeap ==> ");
               		msg += message;
               		throw msg.c_str();
               	}

                //insert new entry in result map that holds chunk-id to DirEntry associations. The chunk id
                // corresponds to the root of the tree.
		//ASSERTION 4: first assert that this is the first time we insert this chunk id
		ChunkID chnkid = (*tree_iter)->getchunkHdrp()->id;
		if(resultMap.find(chnkid) != resultMap.end())
			throw "AccessManager::createTreeRegionDiskBucketInHeap ==>ASSERTION 4: Duplicate chunk id for root of tree in cluster\n";
		DirEntry dirent(bcktID, dbuckp->hdr.subtree_dir_entry[dbuckp->hdr.no_subtrees-1]); // create the DirEntry consisting of the bucket
												   // id and the chunk slot corresponding
												   // to this tree
		resultMap[chnkid] = dirent;

               	// free up memory
        	delete dirVectp;
        	delete dataVectp;
	}//end for
}//end of AccessManager::createTreeRegionDiskBucketInHeap

void traverseSingleCostTreeCreateChunkVectors(
				unsigned int maxDepth,
				unsigned int numFacts,
				const CostNode* const costRoot,
				const string& factFile,
				const BucketID& bcktID,
				vector<DirChunk>* &dirVectp,
				vector<DataChunk>* &dataVectp,
				const TreeTraversal_t howToTraverse)
//precondition:
//	costRoot points at a cost-tree and NOT a single data chunk,
//	dirVectp and dataVectp are NULL vectors
//	bcktID contains a valid bucket id where these tree will be eventually be stored in
//postcondition:
//	dirVectp and dataVectp point at two vectors containing the instantiated dir and Data chunks
//	respectively.
{
	// ASSERTION 1: costRoot does not point at a data chunk
       	if(costRoot->getchunkHdrp()->depth == maxDepth)
       		throw "AccessManager::traverseSingleCostTreeCreateChunkVectors ==> ASSERTION 1: found single data chunk in input tree\n";

	// ASSERTION 2: null input pointers
	if(dirVectp || dataVectp)
       		throw "AccessManager::traverseSingleCostTreeCreateChunkVectors ==> ASSERTION 2: not null vector pointers\n";

	// ASSERTION 3: not null bucket id
	if(bcktID.isnull())
       		throw "AccessManager::traverseSingleCostTreeCreateChunkVectors ==> ASSERTION 3: found null input bucket id\n";

	// Reserve space for the two chunk vectors
	dirVectp = new vector<DirChunk>;
	unsigned int numDirChunks = 0;
	CostNode::countDirChunksOfTree(costRoot, maxDepth, numDirChunks);
	dirVectp->reserve(numDirChunks);

	dataVectp = new vector<DataChunk>;
	unsigned int numDataChunks = 0;
	CostNode::countDataChunksOfTree(costRoot, maxDepth, numDataChunks);
	dataVectp->reserve(numDataChunks);

	// Fill the chunk vectors by descending the cost tree with the appropriate
	// traversal method.
        switch(howToTraverse){
                case breadthFirst:
                        {
                        	queue<CostNode*> nextToVisit; // breadth-1st queue
                        	try {
                        		descendBreadth1stCostTree(
                        		        maxDepth,
                        		        numFacts,
                        		        costRoot,
                        		        factFile,
                        		        bcktID,
                        		        nextToVisit,
                        		        dirVectp,
                        		        dataVectp);
                        	}
                              	catch(const char* message) {
                              		string msg("AccessManager::traverseSingleCostTreeCreateChunkVectors ==>");
                              		msg += message;
                              		throw msg;
                              	}
                      	}//end block
                      	break;
                case depthFirst:
                        {
                        	try {
                        		descendDepth1stCostTree(
                        		        maxDepth,
                        		        numFacts,
        				        costRoot,
        				        factFile,
        				        bcktID,
        				        dirVectp,
        				        dataVectp);
                        	}
                              	catch(const char* message) {
                              		string msg("AccessManager::traverseSingleCostTreeCreateChunkVectors ==>");
                              		msg += message;
                              		throw msg;
                              	}
                      	}//end block
              		break;
                 default:
                        throw "AccessManager::traverseSingleCostTreeCreateChunkVectors ==> Unkown traversal method\n";
                        break;
         }//end switch

      	//ASSERTION 4: valid returned vectors
       	if(dirVectp->empty() || dataVectp->empty())
       			throw "AccessManager::traverseSingleCostTreeCreateChunkVectors ==> ASSERTION 4: empty chunk vector!!\n";
}//end of AccessManager::traverseSingleCostTreeCreateChunkVectors

void placeChunksOfSingleTreeInDiskBucketBody(unsigned int maxDepth, unsigned int numFacts,
		 const vector<DirChunk>* const dirVectp, const vector<DataChunk>* const dataVectp,
		 DiskBucket* const dbuckp, char* &nextFreeBytep, const TreeTraversal_t howToTraverse)
// precondition:
//      dirVectp and dataVectp (input parameters) contain the chunks of a single tree
//	that can fit in a single DiskBucket. These chunks have been placed in the 2 vectors with a
//      call to AccessManager::traverseSingleCostTreeCreateChunkVectors. dbuckp (input parameter) is
//	a const pointer to an allocated DiskBucket, where its header members have been initialized.
//	Finally, nextFreeBytep is a byte pointer (input+output parameter) that points at the beginning
//	of free space in the DiskBucket's body.
// postcondition:
//      the chunks of the two vectors have been placed in the body of the bucket with respect to a
//      traversal method of the tree. The nextFreeBytep pointer points at the first free byte in the
//      body of the bucket.
{
        //According to the desired traversal method store the chunks in the body of the bucket	
        switch(howToTraverse){
                case breadthFirst:
                        try{
                                _storeBreadth1stInDiskBucket(maxDepth, numFacts,
		                        dirVectp, dataVectp, dbuckp, nextFreeBytep);
                        }
                	catch(const char* message) {
                		string msg("AccessManager::placeChunksOfSingleTreeInDiskBucketBody ==> ");
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
                		string msg("AccessManager::placeChunksOfSingleTreeInDiskBucketBody ==> ");
                		msg += message;
                		throw msg.c_str();
                	}		
                        break;
                default:
                        throw "AccessManager::placeChunksOfSingleTreeInDiskBucketBody ==> unknown traversal method\n";
                        break;
        }//end switch
}// end of AccessManager::placeChunksOfSingleTreeInDiskBucketBody

