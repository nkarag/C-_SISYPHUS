/////////////////////////////////////////////////////////////////
// This program's goal is to test the unit:
// AccessManager::storeTreesInCUBE_FileClusters
//
// Nikos Karayannidis 17/9/2001
////////////////////////////////////////////////////////////////

#include <ctime>
#include "classes.h"
#include "functions.h"

class SimpleClusteringAlg {
      	private:
      		//no extra input parameters needed for this algorithm.
      	public:
      		/**
      		 * This implements the algorithm.The algorithm sequencially scans the
      		 * input vector and tries to put in the same bucket as many trees as possible.
      		 * NOTE: the algorithm does not sort the input vector nor it assumes any specific order.
      		 *	 However, due to the way the caseBvect is constructed, its entries are sorted in
      		 * 	 ascending order of their chunk id.
      		 */
		void operator()( const vector<CaseStruct>& caseBvect,
				 multimap<BucketID,ChunkID>& resultRegions,
				 vector<BucketID>& resultBucketIDs
				);
};

void formulateBucketRegions(
       	const vector<CaseStruct>& caseBvect,
       	multimap<BucketID, ChunkID>& resultRegions,
       	vector<BucketID>& resultBucketIDs,
       	const clustAlgToken_t clustering_algorithm = simple);

void createTreeRegionDiskBucketInHeap(
		unsigned int maxDepth,
		unsigned int numFacts,
		const vector<CostNode*>& treesOfregion,
		const BucketID& bcktID,
		const string& factFile,
		DiskBucket* &dbuckp,
		map<ChunkID, DirEntry>& resultMap,
		const TreeTraversal_t howToTraverse = breadthFirst);

void storeTreesInCUBE_FileClusters(const CubeInfo& cinfo,
                        const vector<CaseStruct>& caseBvect,
                        const CostNode* const costRoot,
                        const string& factFile,
                        vector<DirEntry>& resultVect,
                        const TreeTraversal_t howToTraverse = breadthFirst,
                        const clustAlgToken_t clustering_algorithm = simple);

main()
{

        //create cube info
        CubeInfo cinfo(string("lala"));
        cinfo.setmaxDepth(3);
        cinfo.setnumFacts(2);
        int numDims = 2;

        //create a single cost tree (the sub-tree under cell 0|0 in fig 5 of TR)
        CostNode* root;
        try{
                createSingleTree(root, cinfo.getmaxDepth(), cinfo.getnumFacts(), numDims);
        }
       	catch(const string& message) {
       		string msg("main ==> \n"); //error_out<<msg<<endl;
       		msg += message;
                cerr<<"*** TERMINATION OF PROGRAM - check error.log ***\n";

                ofstream error_out("error.log");//,ios::app); //open error file steam in append mode
                if (!error_out)
                        cerr << "creating file \"error.log\" failed\n";

                time_t* timep = new time_t;
                time(timep);
                error_out<<"********************************************"<<endl;
                error_out<<"Error Log"<<endl;
                error_out<<ctime(timep);
                error_out<<"********************************************"<<endl;

                error_out<<msg<<endl;
                exit(0);
       	}

        //create caseBvect
        vector<CaseStruct> caseBvect;
        createCaseBVect(root,caseBvect);

        //decide on clustering alg

        //decide on traversal method
        TreeTraversal_t howToTraverse;
        cout<<"Choose traversal method (1 breadth1st, 2 depth1st) : ";
        int answr;
        cin>>answr;
        (answr == 1) ? howToTraverse = breadthFirst : howToTraverse = depthFirst;

        // call method for testing
        vector<DirEntry> resultVect;
	try{
		storeTreesInCUBE_FileClusters(
                        cinfo,
                        caseBvect,
                        root,
                        string("facts.fld"),
                        resultVect,
                        howToTraverse
                        );
	}
       	catch(const string& message) {
       		string msg("main ==> \n"); //error_out<<msg<<endl;
       		msg += message;
                cerr<<"*** TERMINATION OF PROGRAM - check error.log ***\n";

                ofstream error_out("error.log");//,ios::app); //open error file steam in append mode
                if (!error_out)
                        cerr << "creating file \"error.log\" failed\n";

                time_t* timep = new time_t;
                time(timep);
                error_out<<"********************************************"<<endl;
                error_out<<"Error Log"<<endl;
                error_out<<ctime(timep);
                error_out<<"********************************************"<<endl;

                error_out<<msg<<endl;
                exit(0);
       	}

        // print results
        printResults(resultVect, caseBvect);

}//end main

void storeTreesInCUBE_FileClusters(
			const CubeInfo& cinfo,
			const vector<CaseStruct>& caseBvect,
			const CostNode* const costRoot,
			const string& factFile,
			vector<DirEntry>& resultVect,
			const TreeTraversal_t howToTraverse,
        		const clustAlgToken_t clustering_algorithm)
// precondition:
//	each entry caseBvect corresponds to a chunk-tree. All trees are hanging from the same parent
//	and have a size-cost less than the bucket threshold. costRoot is a pointer
//	to the common parent CostNode. The resultVect is empty but we have already reserved
//	space for it, because we know that its size is equal with the size of caseBvect.
//
// postcondition:
//	The trees have been stored in CUBE_File buckets, some of them in the same bucket, forming a "cluster"
//	(or "bucket region").
//	The resultVect is filled with DirEntries that correspond 1-1 to the entries in caseBVect. Each DirEntry
//	denotes the bucket id as well as the chunk slot that each tree resides.
{
        if(!costRoot)
                throw string("AccessManager::storeTreesInCUBE_FileClusters ==> null tree pointer\n");

        if(factFile.empty())
                throw string("AccessManager::storeTreesInCUBE_FileClusters ==> no load file name\n");

	//formulate the regions
	multimap<BucketID, ChunkID> bucketRegion;
	vector<BucketID> buckIDs;
	try{
		formulateBucketRegions(caseBvect, bucketRegion, buckIDs, clustering_algorithm);
	}
       	catch(const string& message) {
       		string msg("AccessManager::storeTreesInCUBE_FileClusters ==> \n"); //error_out<<msg<<endl;
       		msg += message;
       		throw msg;
       	}

       	map<ChunkID, DirEntry> resultMap; // map, to associate each root of the trees with its
       					  // corresponding DirEntry in its parent chunk.

	//for each region...
	typedef multimap<BucketID, ChunkID>::const_iterator MMAP_ITER;
        unsigned int maxDepth = cinfo.getmaxDepth();
        unsigned int numFacts = cinfo.getnumFacts();
	// for each bucket id corresponding to a region
	for(vector<BucketID>::const_iterator buck_i = buckIDs.begin();
	    buck_i != buckIDs.end(); ++buck_i) {

		//find the range of "items" in a specific region corresponding to a specific BucketID
        	pair<MMAP_ITER,MMAP_ITER> c = bucketRegion.equal_range(*buck_i);
        	vector<CostNode*> treesOfregion; // vector to hold the corresponding cost nodes pointers (tree pointers)
        	// iterate through all chunk ids of a region
        	for(MMAP_ITER iter = c.first; iter!=c.second; ++iter) {
			CostNode* childNodep = costRoot->getchildById(iter->second);
			//ASSERTION
			if(!childNodep)
				throw string("AccessManager::storeTreesInCUBE_FileClusters ==> ASSERTION: child node not found!\n");
        		 treesOfregion.push_back(childNodep);
      		} //end for

		//create a DiskBucket instance in heap containing this region and update resultMap
	        DiskBucket* dbuckp = 0;
		try{
			createTreeRegionDiskBucketInHeap(maxDepth, numFacts, treesOfregion, *buck_i,
							factFile, dbuckp, resultMap);
		}
         	catch(const string& message) {
         		string msg("AccessManager::storeTreesInCUBE_FileClusters ==> \n");
         		msg += message;
         		throw msg;
         	}

//***************************************
#ifdef DEBUGGING
        try{
                printDiskBucketContents_SingleTreeAndCluster(dbuckp, maxDepth);
        }
      	catch(const string& message) {
      		string msg("AccessManager::storeTreesInCUBE_FileClusters ==> \n");
      		msg += message;
      		throw msg;
      	}
#endif
//***************************************

//*********************************************
// remove comments when copied to Sisyphus sources
       /* 	// Store the DiskBucket in a fixed size Bucket in the CUBE File
        	try {
        		FileManager::storeDiskBucketInCUBE_File(dbuckp, cinfo);
        	}
         	catch(const string& message) {
         		string msg("AccessManager::storeTreesInCUBE_FileClusters ==> \n");
         		msg += message;
         		throw msg;
         	}
         */
//**********************************************
         	delete dbuckp; //free up memory
	}//end for

	//update result vector with the appropriate DirEntries
	// NOTE: resultVect has 1-1 correspondence with caseBvect
	for(vector<CaseStruct>::const_iterator i = caseBvect.begin();
		i!=caseBvect.end(); ++i) {

		resultVect.push_back(resultMap[(*i).id]);
	} //end for
}// end of AccessManager::storeTreesInCUBE_FileClusters




void formulateBucketRegions(
			const vector<CaseStruct>& caseBvect,
			multimap<BucketID, ChunkID>& resultRegions,
			vector<BucketID>& resultBucketIDs,
			const clustAlgToken_t clustering_algorithm)
// precondition:
//	each entry of caseBvect corresponds to a chunk-tree. All trees are hanging from the same parent
//	and have a size-cost less than the bucket threshold. clustering_algorithm denotes the algorithm
//	with which the bucket regions (i.e. clusters) will be created.
//
// postcondition:
//	resultBucketIDs contains the set of bucket ids that correspond to the buckets that will store
//	the formulated regions. Each region is stored in a single bucket. resultRegions contains the
// 	mapping of each bucket id to the chunk ids of the roots of the trees of the corresponding region.
{
	// Call the clustering algorithm to form the Bucket Regions
	//multimap<BucketID, ChunkID, less_than_BucketID> bucketRegion;
	switch(clustering_algorithm) {
		case simple:
			SimpleClusteringAlg algorithm; //init function object
			try {
				algorithm(caseBvect, resultRegions, resultBucketIDs);
			}
                       	catch(const string& message) {
                       		string msg("AccessManager::formulateBucketRegions ==> \n"); //error_out<<msg<<endl;
                       		msg += message;
                       		throw msg;
                       	}
			break;
		default:
			throw string("AccessManager::formulateBucketRegions ==> Unknown clustering algorithm\n");
			break;
	}// end switch

} // end of AccessManager::formulateBucketRegions

void SimpleClusteringAlg::operator() (
							const vector<CaseStruct>& caseBvect,
							multimap<BucketID,ChunkID>& resultRegions,
							vector<BucketID>& resultBucketIDs
				                    )
// precondition:
//	each entry of caseBvect corresponds to a chunk-tree. All trees are hanging from the same parent
//	and have a size-cost less than the bucket threshold.
// postcondition:
//	resultBucketIDs contains the set of bucket ids that correspond to the buckets that will store
//	the formulated regions. Each region is stored in a single bucket. resultRegions contains the
// 	mapping of each bucket id to the chunk ids of the roots of the trees of the corresponding region.
{
        //ASSERTION: no empty input
        if(caseBvect.empty())
                throw string("SimpleClusteringAlg::operator() ==> empty input vector\n");

	size_t CLUSTER_SIZE_LIMIT = DiskBucket::bodysize; //a cluster's size must not exceed this limit

	//cost of current cluster
	size_t curr_clst_cost = 0;

        unsigned int noSubtreesInCurrClst = 0; //number of subtrees in current cluster

	//create a new bucket id
	BucketID curr_id = BucketID::createNewID();

	//insert bucket id into bucket id output vector
	resultBucketIDs.push_back(curr_id);

	//loop from the  1st item to the last of the input vector
	for(vector<CaseStruct>::const_iterator iter = caseBvect.begin(); iter != caseBvect.end(); iter++) {
                //ASSERTION: cost must be below Threshold
                if(iter->cost >= DiskBucket::BCKT_THRESHOLD)
                        throw string("SimpleClusteringAlg::operator() ==> cost of tree exceeds bucket threshold!\n");
		//add the cost of current item to the cost of current cluster
		//if the cost of current cluster is still below the limit
		if(curr_clst_cost + (*iter).cost <= CLUSTER_SIZE_LIMIT
                                        &&
                   noSubtreesInCurrClst+1 <= DiskBucketHeader::subtreemaxno) {
			//then insert current item in the current cluster
                        resultRegions.insert(make_pair(curr_id, iter->id));
			curr_clst_cost += (*iter).cost; // update cluster cost
                        noSubtreesInCurrClst++; //update cluster counter
		}
		else {
			//create a new bucket id
			curr_id = BucketID::createNewID();
                	//insert bucket id into bucket id output vector
                	resultBucketIDs.push_back(curr_id);
                	// re-initialize cluster cost
                	curr_clst_cost = 0;
                        noSubtreesInCurrClst = 0; //reset subtree counter
			//then insert current item in the current cluster
			resultRegions.insert(make_pair(curr_id, iter->id));
			curr_clst_cost += (*iter).cost; // update cluster cost
		}// end else
	}//end for
}// end of AccessManager::SimpleClusteringAlg::operator()

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
		throw string("AccessManager::createTreeRegionDiskBucketInHeap ==> ASSERTION1: input pointer to DiskBucket should be null\n");
        //ASSERTION2: non empty input vector
        if(treesOfregion.empty())
                throw string("AccessManager::createTreeRegionDiskBucketInHeap ==> ASSERTION2: input vector is empty\n");
        //ASSERTION3: NOT too many subtrees in a single cluster
        if(treesOfregion.size() > DiskBucketHeader::subtreemaxno)
                throw string("AccessManager::createTreeRegionDiskBucketInHeap ==> ASSERTION3: too many subtrees to store in a cluster\n");

	// allocate DiskBucket in heap
	dbuckp = new DiskBucket;

	// initialize bucket directory pointer to point one beyond last byte of body
	dbuckp->offsetInBucket = reinterpret_cast<DiskBucket::dirent_t*>(&(dbuckp->body[DiskBucket::bodysize]));

	// initialize the DiskBucketHeader
	dbuckp->hdr.id.rid = bcktID.rid; // store the bucket id
	// Init the links to other buckets with null ids
//******************************
	//dbuckp->hdr.next.rid = serial_t::null;
	//dbuckp->hdr.previous.rid = serial_t::null;
	dbuckp->hdr.next.rid = 0;
	dbuckp->hdr.previous.rid = 0;

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
               	catch(const string& message) {
               		string msg("AccessManager::createTreeRegionDiskBucketInHeap ==> \n");
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
               	catch(const string& message) {
               		string msg("AccessManager::createTreeRegionDiskBucketInHeap ==> \n");
               		msg += message;
               		throw msg;
               	}

                //insert new entry in result map that holds chunk-id to DirEntry associations. The chunk id
                // corresponds to the root of the tree.
		//ASSERTION 4: first assert that this is the first time we insert this chunk id
		ChunkID chnkid = (*tree_iter)->getchunkHdrp()->id;
		if(resultMap.find(chnkid) != resultMap.end())
			throw string("AccessManager::createTreeRegionDiskBucketInHeap ==>ASSERTION 4: Duplicate chunk id for root of tree in cluster\n");
		DirEntry dirent(bcktID, dbuckp->hdr.subtree_dir_entry[dbuckp->hdr.no_subtrees-1]); // create the DirEntry consisting of the bucket
												   // id and the chunk slot corresponding
												   // to this tree
		resultMap[chnkid] = dirent;

               	// free up memory
        	delete dirVectp;
        	delete dataVectp;
	}//end for
}//end of AccessManager::createTreeRegionDiskBucketInHeap
