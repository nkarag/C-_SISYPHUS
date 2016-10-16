///////////////////////////////////////////////////////////
// this program tests the unit:
// AccessManager::storeDataChunkInCUBE_FileBucket
//
// Nikos Karayannidis,	4/10/2001
//////////////////////////////////////////////////////////

#include "Exceptions.h"
#include "DiskStructures.h"
#include "classes.h"
#include "functions.h"

#include <fstream>
#include <string>


void storeDataChunkInCUBE_FileBucket(
                                const CubeInfo& cinfo, //input
                                const CostNode* const costRoot, //input
                                const string& factFile,  //input
                                DirEntry& returnDirEntry  //output
                                );

void createSingleDataChunkDiskBucketInHeap(
                                unsigned int maxDepth,
                                unsigned int numFacts,
                                const BucketID& bcktID,
                                const DataChunk& datachunk,
                                DiskBucket* &dbuckp);

main()
{
        ofstream error_out("error.log", ios::app);

	// maxdepth numfacts init
	unsigned int maxDepth = 3;
	unsigned int numFacts = 2;
        unsigned int numDims = 2;

        //create a CubeInfo
        CubeInfo cinfo("lala_cube");
        cinfo.setmaxDepth(maxDepth);
        cinfo.setnumFacts(numFacts);

        //create a cost Node corresponding to a data chunk
        CostNode* nodep = createCostNode(maxDepth,numFacts,numDims);

        //call routine for testing
        DirEntry e;
        try{
                storeDataChunkInCUBE_FileBucket(cinfo,
                                        nodep,
                                        string("facts.fld"),
                                        e);
        }
        catch(GeneralError& error) {
                GeneralError e("main ==> ");
                error += e;
                cerr<<"Program Termination. See error.log\n";
                error_out << error;
                exit(0);
        }
        //ASSERTION2: the returned DirEntry is valid
        if(e.bcktId.isnull() || e.chnkIndex != 0){
                cerr<<"Program Termination. See error.log\n";
                error_out<< GeneralError("AccessManager::putChunksIntoBuckets ==> ASSERTION2: invalid returned dirchunk entry\n");
                exit(0);
        }//if

        cout<<"BucketId : "<<e.bcktId.rid<<endl;
        cout<<"Chunk Index: "<<e.chnkIndex<<endl;
}//end main

void storeDataChunkInCUBE_FileBucket(
       				const CubeInfo& cinfo, //input
       				const CostNode* const costRoot, //input
       				const string& factFile,  //input
       				DirEntry& returnDirEntry  //output
				)
//precondition:
//	costRoot points at a CostNode corresponding to a DataChunk.The total size of the chunk satisfies
//	the following: BUCKET_THRESHOLD <= size <= DiskBucket::bodysize. The cinfo must at least contain valid
//	info on the following:
//	ssm file id, the maximum chunking depth and the number of facts per cell.
//postcondition:
//	the DataChunk has been stored in a CUBE File and returnDirEntry contains the Bucket id and the chunk slot.
{
        unsigned int maxDepth = cinfo.getmaxDepth();
        unsigned int numFacts = cinfo.getnumFacts();

	//ASSERTION 1: this is a Data Chunk
       	if(costRoot->getchunkHdrp()->depth != maxDepth)
       		throw GeneralError("AccessManager::storeDataChunkInCUBE_FileBucket ==> ASSERTION 1: found tree instead of single data chunk in input\n");

	//ASSERTION 2: the cost is within desired limits
       	unsigned int szBytes = 0;
       	CostNode::calcTreeSize(costRoot, szBytes);
cout<<"szBytes: "<<szBytes<<endl;
cout<<"DiskBucket::BCKT_THRESHOLD: "<<DiskBucket::BCKT_THRESHOLD<<endl;
cout<<"DiskBucket::bodysize: "<<DiskBucket::bodysize<<endl;
       	if(szBytes < DiskBucket::BCKT_THRESHOLD || szBytes > DiskBucket::bodysize)
       		throw GeneralError("AccessManager::storeDataChunkInCUBE_FileBucket ==> ASSERTION 2: wrong data chunk size\n");

        //  Create a BucketID for the  Bucket that will store the data chunk.
	//  NOTE: no bucket allocation performed yet, just id generation!
	//create a new bucket id
	BucketID bcktID;
	try{
		bcktID = BucketID::createNewID();
	}
       	catch(GeneralError& error) {
       		GeneralError e("AccessManager::storeDataChunkInCUBE_FileBucket ==> ");
       		error += e;
       		throw error;
       	}

	//read the input file and create a DataChunk instance
	vector<DataChunk>* const dataVectp = new vector<DataChunk>;
        vector<DirChunk>* const dirVectp = 0;
	dataVectp->reserve(1);
        try {
		descendDepth1stCostTree( maxDepth,
                                         numFacts,
                                         costRoot,
                                         factFile,
                                         bcktID,
                                         dirVectp, // no pointer to DirChunk vector passed in this case
                                         dataVectp);
        }
        catch(GeneralError& error) {
               GeneralError e("AccessManager::storeDataChunkInCUBE_FileBucket ==> ");
               error += e;
               throw error;
        }

      	//ASSERTION 3: valid returned vectors
       	if(dataVectp->size() != 1)
		throw GeneralError("AccessManager::storeDataChunkInCUBE_FileBucket ==> ASSERTION 3: error in creating the DataChunk instance from file\n");


	//create a heap allocated DiskBucket instance containing only this chunk
        DiskBucket* dbuckp = 0;
        try {
		createSingleDataChunkDiskBucketInHeap(maxDepth,
					numFacts,
					bcktID,
					*(dataVectp->begin()),
					dbuckp);
        }
        catch(GeneralError& error) {
               GeneralError e("AccessManager::storeDataChunkInCUBE_FileBucket ==> ");
               error += e;
               throw error;
        }

       #ifdef DEBUGGING
	//print the contents of the created bucket in a separate file
        try{
                printDiskBucketContents_SingleTreeAndCluster(dbuckp, maxDepth);
        }
      	catch(GeneralError& error) {
      		GeneralError e("AccessManager::storeTreesInCUBE_FileClusters ==> \n");
      		error += e;
      		throw error;
      	}
       #endif
/****************************************************************
                ACTIVATE IT AFTER COPYING IT BACK TO SISYPHUS SOURCES!!
                ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//  Store the DiskBucket in a fixed size Bucket in the CUBE File
	try {
		FileManager::storeDiskBucketInCUBE_File(dbuckp, cinfo);
	}
      	catch(GeneralError& error) {
      		GeneralError e("AccessManager::storeSingleTreeInCUBE_FileBucket ==>");
      		error += e;
      		throw error;
      	}
*******************************************************************/
       	// update the DirEntry corresponding to the cell pointing to costRoot and
       	// return it to the caller.
       	returnDirEntry.bcktId = bcktID;
	returnDirEntry.chnkIndex = 0; //root of tree is stored at the first chunk slot

 	// free up memory
 	delete dbuckp;
	delete dataVectp;
}//end AccessManager::storeDataChunkInCUBE_FileBucket


void createSingleDataChunkDiskBucketInHeap(
					unsigned int maxDepth,
					unsigned int numFacts,
					const BucketID& bcktID,
					const DataChunk& datachunk,
					DiskBucket* &dbuckp)
//precondition:
//	dbuckp points to NULL. datachunk contains the DataChunk that we want to
//	store in the DiskBucket.
//postcondition:
//	dbuckp points at a heap allocated DiskBucket where the data chunk has been placed in
//	its body.
{
	//ASSERTION1: dbuckp points to NULL
	if(dbuckp)
		throw GeneralError("AccessManager::createSingleDataChunkDiskBucketInHeap ==> ASSERTION1: input pointer to DiskBucket should be null\n");

	// allocate DiskBucket in heap
	dbuckp = new DiskBucket;

	// initialize directory pointer to point one beyond last byte of body
	dbuckp->offsetInBucket = reinterpret_cast<DiskBucket::dirent_t*>(&(dbuckp->body[DiskBucket::bodysize]));
	// initialize the DiskBucketHeader
	dbuckp->hdr.id.rid = bcktID.rid; // store the bucket id

//*****************************************************************************
// REMOVE serial_t COMMENTS ON COPYING IT BACK TO SISYPHUS!!!!!!!!!!!!!!!!!!!!
        dbuckp->hdr.next.rid = 0; // Init the links to other buckets with null ids
        dbuckp->hdr.previous.rid = 0;
	//dbuckp->hdr.next.rid = serial_t::null; // Init the links to other buckets with null ids
	//dbuckp->hdr.previous.rid = serial_t::null;
//*******************************************************************************888
	dbuckp->hdr.no_chunks = 0;  // init chunk counter
	dbuckp->hdr.next_offset = 0; //next free byte offset in the body
	dbuckp->hdr.freespace = DiskBucket::bodysize;
	dbuckp->hdr.no_subtrees = 1; // single "tree" stored
	dbuckp->hdr.subtree_dir_entry[0] = 0; // the root of this "tree" will be stored at chunk slot 0
	dbuckp->hdr.no_ovrfl_next = 0; // no overflow-bucket  chain used

	char* nextFreeBytep = dbuckp->body; //init current byte pointer
	// place the data chunk in the bucket's body
        try{
	        placeSingleDataChunkInDiskBucketBody(maxDepth, numFacts,
		                        datachunk, dbuckp, nextFreeBytep);
        }
       	catch(GeneralError& error) {
       		GeneralError e("AccessManager::createSingleDataChunkDiskBucketInHeap ==> ");
       		error += e;
       		throw error;
       	}
}//AccessManager::createSingleDataChunkDiskBucketInHeap