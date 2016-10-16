///////////////////////////////////////////////////////////
// this program test the unit:
// AccessManager::placeSingleDataChunkInDiskBucketBody
//
// Nikos Karayannidis,	26/9/2001
//////////////////////////////////////////////////////////

#include "Exceptions.h"
#include "DiskStructures.h"
#include "classes.h"
#include "functions.h"

#include <fstream>

void placeSingleDataChunkInDiskBucketBody(
			unsigned int maxDepth,
			unsigned int numFacts,
			const DataChunk& datachunk,
			DiskBucket* const dbuckp,
			char* &nextFreeBytep);

main()
{
        ofstream error_out("error.log", ios::app);

	// maxdepth numfacts init
	unsigned int maxDepth = 3;
	unsigned int numFacts = 2;

        //create a DataChunk
        const DataChunk* chunkp = createDataChunk(maxDepth, numFacts);

        #ifdef DEBUGGING
        //print the created data chunk
        cerr<<"The created DataChunk is:\n";
        cerr<<"HEADER\n";
        cerr<<"~~~~~~~~~~~\n";
        cerr<<"header.id.getcid: "<<chunkp->gethdr().id.getcid()<<endl;
        cerr<<"header.depth: "<<chunkp->gethdr().depth<<endl;
        cerr<<"header.numDim: "<<chunkp->gethdr().numDim<<endl;
        cerr<<"header.totNumCells: "<<chunkp->gethdr().totNumCells<<endl;
        cerr<<"header.rlNumCells: "<<chunkp->gethdr().rlNumCells<<endl;
        cerr<<"header.size: "<<chunkp->gethdr().size<<endl;
        for(vector<LevelRange>::const_iterator rngiter = chunkp->gethdr().vectRange.begin();
                                rngiter != chunkp->gethdr().vectRange.end(); rngiter++) {
                cerr<<"Level "<<rngiter->lvlName<<": ["<<rngiter->leftEnd<<", "<<rngiter->rightEnd<<"]\n";
        }//for

        //print the bitmap
        cerr<<"\nBITMAP\n";
        cerr<<"~~~~~~~~~~~\n";
        cerr<<"The bitmap is (size "<<chunkp->getcomprBmp().size()<<"):\n";
        for(bit_vector::const_iterator biter = chunkp->getcomprBmp().begin();
                                                biter!=chunkp->getcomprBmp().end(); biter++)
                cerr<<*biter<<" ";

        cerr<<endl;

        //print Data entries
        cerr<<"\nDATA ENTRIES\n";
        cerr<<"~~~~~~~~~~~\n";
        cerr<<"Number of entries: "<<chunkp->getentry().size()<<endl;
        for(vector<DataEntry>::const_iterator entiter = chunkp->getentry().begin();
                                entiter!=chunkp->getentry().end(); entiter++) {

                cerr<<"numFacts: "<<entiter->numFacts<<endl;

                for(vector<measure_t>::const_iterator miter = entiter->fact.begin();
                                miter != entiter->fact.end(); miter++){
                        cerr<<*miter<<", ";
                }//for
                cerr<<endl;
        }//for
        #endif


	// create a DiskBucket and init its header members
	// allocate DiskBucket in heap
	DiskBucket* dbuckp = new DiskBucket;

	// initialize directory pointer to point one beyond last byte of body
	dbuckp->offsetInBucket = reinterpret_cast<DiskBucket::dirent_t*>(&(dbuckp->body[DiskBucket::bodysize]));
	// initialize the DiskBucketHeader
	BucketID bcktID;
	try{
		bcktID = BucketID::createNewID();
	}
       	catch(GeneralError& error) {
       		GeneralError e("main ==> ");
       		error += e;
       		error_out << error << endl;
       		cerr<<"Program termination! See error.log\n";
       		exit(0);
       	}
	dbuckp->hdr.id.rid = bcktID.rid; // store the bucket id

	//dbuckp->hdr.next.rid = serial_t::null; // Init the links to other buckets with null ids
	//dbuckp->hdr.previous.rid = serial_t::null;
	dbuckp->hdr.next.rid = 0; // Init the links to other buckets with null ids
	dbuckp->hdr.previous.rid = 0;

	dbuckp->hdr.no_chunks = 0;  // init chunk counter
	dbuckp->hdr.next_offset = 0; //next free byte offset in the body
	dbuckp->hdr.freespace = DiskBucket::bodysize;
	dbuckp->hdr.no_subtrees = 1; // single "tree" stored
	dbuckp->hdr.subtree_dir_entry[0] = 0; // the root of this "tree" will be stored at chunk slot 0
	dbuckp->hdr.no_ovrfl_next = 0; // no overflow-bucket  chain used

	// init byte pointer at 1st free byte of body
	char* nextFreeBytep = dbuckp->body; //init current byte pointer

	// call routine for testing
        try{
	        placeSingleDataChunkInDiskBucketBody(maxDepth, numFacts,
		                        *chunkp, dbuckp, nextFreeBytep);
        }
       	catch(GeneralError& error) {
       		GeneralError e("main ==> ");
       		error += e;
       		error_out << error << endl;
       		cerr<<"Program termination! See error.log\n";
       		exit(0);       		
       	}
        delete chunkp; //emtpy free space

	//printResult
	//print the contents of the created bucket in a separate file
        try{
                printDiskBucketContents_SingleTreeAndCluster(dbuckp, maxDepth);
        }
      	catch(GeneralError& error) {
      		GeneralError e("AccessManager::storeTreesInCUBE_FileClusters ==> \n");
      		error += e;
       		error_out << error << endl;
       		cerr<<"Program termination! See error.log\n";
       		exit(0);       		
      	}
}//end main

void placeSingleDataChunkInDiskBucketBody(
			unsigned int maxDepth,
			unsigned int numFacts,
			const DataChunk& datachunk,
			DiskBucket* const dbuckp,
			char* &nextFreeBytep)
//precondition:
//	datachunk (input parameter) is a reference to the DataChunk instance that we want to place in the bucket's body.
//	dbuckp (input parameter) is
//	a const pointer to an allocated DiskBucket, where its header members have been initialized.
//	nextFreeBytep is a byte pointer (input+output parameter) that points at the beginning
//	of free space in the DiskBucket's body.
// postcondition:
//      the  datachunk has been placed in the body of the bucket.The nextFreeBytep pointer points at the first free byte in the
//      body of the bucket.
{
	//ASSERTION 1: there is free space to store this data chunk
	// add the size of the datachunk
	size_t szBytes = datachunk.gethdr().size;
	//also add the cost for the corresponding entry in the internal directory of the DiskBucket
	szBytes += sizeof(DiskBucket::dirent_t);
	if(szBytes > dbuckp->hdr.freespace)
		throw GeneralError("AccessManager::placeSingleDataChunkInDiskBucketBody ==> ASSERTION 1: DataChunk does not fit in DiskBucket!\n");		

      	// update bucket directory (chunk slots begin from slot 0)
      	dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = dbuckp->hdr.next_offset;
      	dbuckp->hdr.freespace -= sizeof(DiskBucket::dirent_t);
        		
      	// convert DataChunk to DiskDataChunk
      	DiskDataChunk* chnkp = 0;
      	try{
                chnkp = dataChunk2DiskDataChunk(datachunk, numFacts, maxDepth);	      		
      	}
 	catch(GeneralError& error) {
 		GeneralError e("AccessManager::placeSingleDataChunkInDiskBucketBody ==> ");
 		error += e;
 		throw error;
 	}		
	      	
 	// Now, place the parts of the DiskDataChunk into the body of the DiskBucket
      	size_t chnk_size = 0;
      	size_t hdr_size = 0;
      	try{      		
                placeDiskDataChunkInBcktBody(chnkp, maxDepth, nextFreeBytep, hdr_size, chnk_size);					
	}
 	catch(GeneralError& error) {
 		GeneralError e("AccessManager::placeSingleDataChunkInDiskBucketBody ==> ");
 		error += e;
 		throw error;
 	}		
 	#ifdef DEBUGGING
                //ASSERTION 1.2 : no chunk size mismatch         	
                if(datachunk.gethdr().size != chnk_size)
                        throw GeneralError("AccessManager::placeSingleDataChunkInDiskBucketBody ==>  ASSERTION 1.2: DataChunk size mismatch!\n");		
 	#endif

        //update next free byte offset indicator
     	dbuckp->hdr.next_offset += chnk_size;        		
     	//update free space indicator
     	dbuckp->hdr.freespace -= chnk_size;
     		
     	// update bucket header: chunk counter        		
       	dbuckp->hdr.no_chunks++;

        delete chnkp; //free up memory
}// end AccessManager::placeSingleDataChunkInDiskBucketBody