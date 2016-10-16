/***************************************************************************
                          function.cpp  -  description
                             -------------------
    begin                : Wed Sep 26 2001
    copyright            : (C) 2001 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/
#include <strstream>
#include <vector>
#include <cstdlib>
#include <string>
//#include <exception>
//#include <new>

#include "Exceptions.h"
#include "functions.h"
#include "DiskStructures.h"

CostNode* createCostNode(unsigned int maxDepth, unsigned int numFacts, unsigned int numDims)
{
       //create chunk header
        ChunkHeader* hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|0.0|-1"));
        //      depth
        hdrp->depth = maxDepth;
        //      number of dimensions
        hdrp->numDim = numDims;
        //      total number of cells
        hdrp->totNumCells = 1000;
        //      real num of cells
        hdrp->rlNumCells = 1;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DataChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells,
                                                          hdrp->rlNumCells,
                                                          numFacts);
       	}
       	catch(const char* msg){
       		string m("createCostNode ==> ");
       		m += msg;
                	throw m;
       	}
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("country"),0,1));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("type"),0,1));


        //create cell map
        CellMap* clmpp = new CellMap;
        //clmpp->insert(string("0|0.0|0.0|-1.0|0"));
        clmpp->insert(string("0|0.0|0.0|-1.0|1"));

        return new CostNode(hdrp, clmpp);
}//createCostNode

void printDiskBucketContents_SingleTreeAndCluster(
							DiskBucket* const dbuckp,
							unsigned int maxDepth)
// precondition:
//	the dbuckp points at a DiskBucket allocated in heap, which contains a single tree or
//	a cluster of trees. The pointer members of DiskBucket DON NOT contain valid values, therefore
//	they ought to be updated prior to reading the contents of the body.
//postcondition:
//	a file with unique name (for each bucket) has been created with the contents od *dbuckp and
//	the *dbuckp pointer members have been updated to point at the right places in the bucket's body.
{
	// the following piece of codes creates a unique file name for
	// each invocation of this function, based on a static variable called "prefix"
        /*
        static int prefix = 0; // prefix of the output file name
        ostrstream file_name;
        file_name<<"OutputDiskBucket_"<<prefix<<".dbug"<<ends;
        prefix++; //increase for next call
        */

        // Alternatively, we can create a unique file based on the bucket id
        ostrstream file_name;
        file_name<<"OutputDiskBucket_"<<dbuckp->hdr.id.rid<<".dbug"<<ends;

	ofstream out(file_name.str()); //create a file stream for output

        if (!out)
        {
		string s("AccessManager::printDiskBucketContents_SingleTreeAndCluster ==> ");
		string msg = s + string("creating file") + string(file_name.str()) +  string(" failed\n");
		GeneralError error(msg);
		throw error;
        }

        out<<"****************************************************************"<<endl;
        out<<"* Contents of a DiskBucket created by                          *"<<endl;
        out<<"*         AccessManager::createXXXDiskBucketInHeap             *"<<endl;
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

        // for each subtree in the bucket
        for(int i=0; i<dbuckp->hdr.no_subtrees; i++){

                out<<"\n---- SUBTREE "<<i<<" ----\n";
                //get the corresponding chunk slot range
                unsigned int treeBegin = dbuckp->hdr.subtree_dir_entry[i];
                unsigned int treeEnd;
                if(i+1==dbuckp->hdr.no_subtrees) //if this is the last subtree
                        treeEnd  = dbuckp->hdr.no_chunks; //get slot one-passed the last chunk slot of the tree
                else
                        treeEnd  = dbuckp->hdr.subtree_dir_entry[i+1]; //get the next tree's 1st slot

                //for each chunk in the range of slots that correspond to this subtree
                for(int chnk_slot = treeBegin; chnk_slot < treeEnd; chnk_slot++) {
                        //access each chunk:
                        //get a byte pointer at the beginning of the chunk
                        char* beginChunkp = dbuckp->body + dbuckp->offsetInBucket[-chnk_slot-1];

                        //first read the chunk header in order to find whether it is a DiskDirChunk, or
                        //a DiskDataChunk
                        DiskChunkHeader* chnk_hdrp = reinterpret_cast<DiskChunkHeader*>(beginChunkp);
                        if(chnk_hdrp->depth == maxDepth)//then this is a DiskDataChunk
                                printDiskDataChunk(out, beginChunkp);
                        else if (chnk_hdrp->depth < maxDepth && chnk_hdrp->depth > Chunk::MIN_DEPTH)//it is a DiskDirchunk
                                printDiskDirChunk(out, beginChunkp);
                        else {// Invalid depth!
                                if(chnk_hdrp->depth == Chunk::MIN_DEPTH)
                                        throw GeneralError("printDiskBucketContents_SingleTreeAndCluster ==> depth corresponding to root chunk!\n");
                                ostrstream msg_stream;
                                msg_stream<<"printDiskBucketContents_SingleTreeAndCluster ==> chunk depth at slot "<<chnk_slot<<
                                                " inside DiskBucket "<<dbuckp->hdr.id.rid<<" is invalid\n"<<endl;
                                throw GeneralError(msg_stream.str());
                        }//end else
                }//end for
        }//end for
}//AccessManager::printDiskBucketContents_SingleTreeAndCluster

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
cout<<"datachunk.gethdr().size: "<<datachunk.gethdr().size<<endl;
cout<<"chnk_size: "<<chnk_size<<endl;
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
		throw GeneralError("AccessManager::placeDiskDataChunkInBcktBody ==> ASSERTION1: null pointer\n");

	//get a const pointer to the DiskChunkHeader
	const DiskChunkHeader* const hdrp = &chnkp->hdr;
	
	//ASSERTION 1.1: this is a data chunk
	if(hdrp->depth != maxDepth)
		throw GeneralError("AccessManager::placeDiskDataChunkInBcktBody ==> ASSERTION 1.1: chunk's depth != maxDepth in Data Chunk\n");

	//begin by placing the static part of a DiskDatachunk structure
	memcpy(currentp, reinterpret_cast<char*>(chnkp), sizeof(DiskDataChunk));
	currentp += sizeof(DiskDataChunk); // move on to the next empty position
	chnk_size += sizeof(DiskDataChunk); // this is the size of the static part of a DiskDataChunk
	hdr_size += sizeof(DiskChunkHeader); // this is the size of the static part of a DiskChunkHeader

	//continue with placing the chunk id
	//ASSERTION2: chunkid is not null
	if(!hdrp->chunk_id)
		throw GeneralError("AccessManager::placeDiskDataChunkInBcktBody ==> ASSERTION2: null pointer\n");	
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
			throw GeneralError("AccessManager::placeDiskDataChunkInBcktBody ==> ASSERTION3: null pointer\n");			
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
		throw GeneralError("AccessManager::placeDiskDataChunkInBcktBody ==> ASSERTION4: null pointer\n");		
	for(int i = 0; i < hdrp->no_dims; i++) {
	//loop invariant: store an order code range structure
		DiskChunkHeader::OrderCodeRng_t* rngp = &(hdrp->oc_range)[i];
       		memcpy(currentp, reinterpret_cast<char*>(rngp), sizeof(DiskChunkHeader::OrderCodeRng_t));
       		currentp += sizeof(DiskChunkHeader::OrderCodeRng_t); // move on to the next empty position
       		hdr_size += sizeof(DiskChunkHeader::OrderCodeRng_t);		
       		chnk_size += sizeof(DiskChunkHeader::OrderCodeRng_t);		       		
	}//end for
	
	//next place the bitmap (i.e., array of WORDS)
  	for(int b=0; b<bmp::numOfWords(hdrp->no_entries); b++){
		bmp::WORD* wp = &chnkp->bitmap[b];
		memcpy(currentp, reinterpret_cast<char*>(wp), sizeof(bmp::WORD));
	       	currentp += sizeof(bmp::WORD); // move on to the next empty position
       		chnk_size += sizeof(bmp::WORD);       		
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
		throw GeneralError("AccessManager::dataChunk2DiskDataChunk ==> ASSERTION1: wrong depth for data chunk\n");
		
	//allocate new DiskDataChunk
	DiskDataChunk* chnkp = 0;
	try{
		chnkp = new DiskDataChunk;
	}
	catch(bad_alloc){
		throw GeneralError("AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for new DiskDataChunk!\n");		
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
		throw GeneralError("AccessManager::dataChunk2DiskDataChunk ==> ASSERTION 1.1: depth and chunk-id mismatch\n");		
	//allocate space for the domains
	try{	
		chnkp->hdr.chunk_id = new DiskChunkHeader::Domain_t[chnkp->hdr.depth];
	}
	catch(bad_alloc){
		throw GeneralError("AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for the domains!\n");
	}	
	
	//ASSERTION2 : valid no_dims value
	if(chnkp->hdr.no_dims <= 0)
		throw GeneralError("AccessManager::dataChunk2DiskDataChunk ==> ASSERTION2: invalid num of dims\n");	
        //ASSERTION 2.1: num of dims and chunk id compatibility		
        bool isroot = false;		
        if(chnkp->hdr.no_dims != datachnk.gethdr().id.getChunkNumOfDim(isroot))
                throw GeneralError("AccessManager::dataChunk2DiskDataChunk ==> ASSERTION 2.1: num of dims and chunk id mismatch\n");	
        if(isroot)
                throw GeneralError("AccessManager::dataChunk2DiskDataChunk ==> ASSERTION 2.1: root chunk encountered\n");	
	//allocate space for each domain's order-codes
	for(int i = 0; i<chnkp->hdr.depth; i++) { //for each domain of the chunk id
		try{
			chnkp->hdr.chunk_id[i].ordercodes = new DiskChunkHeader::ordercode_t[chnkp->hdr.no_dims];
		}
         	catch(bad_alloc){
         		throw GeneralError("AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for the ordercodes!\n");		
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
		throw GeneralError("AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for the oc ranges!\n");		
	}	
		
	int i=0;
	vector<LevelRange>::const_iterator iter = datachnk.gethdr().vectRange.begin();
	//ASSERTION3: combatible vector length and no of dimensions
	if(chnkp->hdr.no_dims != datachnk.gethdr().vectRange.size())
		throw GeneralError("AccessManager::dataChunk2DiskDataChunk ==> ASSERTION3: wrong length in vector\n");	
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
		throw GeneralError("AccessManager::dataChunk2DiskDataChunk ==> ASSERTION 3.1: total No of cells is less than real No of cells!\n");		
	chnkp->no_ace = datachnk.gethdr().rlNumCells;
	
	// 3. Next copy the bitmap
	//ASSERTION 4: combatible bitmap length and no of entries
	if(chnkp->hdr.no_entries != datachnk.getcomprBmp().size())
		throw GeneralError("AccessManager::dataChunk2DiskDataChunk ==> ASSERTION 4: bitmap size and No of entries mismatch\n");	

	//allocate space for the bitmap
	try{
		chnkp->allocBmp(chnkp->hdr.no_entries);
	}
	catch(bad_alloc){
		throw GeneralError("AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for the bitmap!\n");		
	}	
	//bitmap initialization
        int bit = 0;
        for(bit_vector::const_iterator bititer = datachnk.getcomprBmp().begin();
            bititer != datachnk.getcomprBmp().end(); bititer++){
                if(*bititer == true) {
                	chnkp->set_bit(bit);
                }
                else{
                	chnkp->clear_bit(bit);
                }
                bit++;
        }
							
	// 4. Next copy the entries
	//allocate space for the entries
	try{
		chnkp->entry = new DiskDataChunk::DataEntry_t[chnkp->no_ace];
	}
	catch(bad_alloc){
		throw GeneralError("AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for data entries!\n");		
	}
	
	i = 0;
	vector<DataEntry>::const_iterator ent_iter = datachnk.getentry().begin();
	//ASSERTION5: combatible vector length and no of entries
	if(chnkp->no_ace != datachnk.getentry().size())
		throw GeneralError("AccessManager::dataChunk2DiskDataChunk ==> ASSERTION4: wrong length in vector\n");	
	while(i<chnkp->no_ace && ent_iter != datachnk.getentry().end()){
		//allocate space for the measures
         	try{
         		chnkp->entry[i].measures = new measure_t[int(chnkp->hdr.no_measures)];
         	}
         	catch(bad_alloc){
         		throw GeneralError("AccessManager::dataChunk2DiskDataChunk ==> cant allocate space for data entries!\n");
         	}	
		
         	//store the measures of this entry
       		//ASSERTION6: combatible vector length and no of measures
		if(chnkp->hdr.no_measures != ent_iter->fact.size())
			throw GeneralError("AccessManager::dataChunk2DiskDataChunk ==> ASSERTION6: wrong length in vector\n");	         	
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
}//AccessManager::printDiskDirChunk

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
                  (!chnkp->test_bit(b)) ? out<<"0" : out<<"1";
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
}//end AccessManager::printDiskDataChunk

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
}//AccessManager::updateDiskDirChunkPointerMembers


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
        chnk.bitmap = reinterpret_cast<bmp::WORD*>(bytep);

        //update entry pointer
        bytep += sizeof(bmp::WORD)* bmp::numOfWords(chnk.hdr.no_entries); // move to the 1st data entry
        chnk.entry = reinterpret_cast<DiskDataChunk::DataEntry_t*>(bytep);

        //move byte pointer at the first measure value
        bytep += sizeof(DiskDataChunk::DataEntry_t)*chnk.no_ace;
        //for each data entry update measures pointer
        for(int e=0; e<chnk.no_ace; e++){
                //place measures pointer at the first measure
                chnk.entry[e].measures = reinterpret_cast<measure_t*>(bytep);
                bytep += sizeof(measure_t)*chnk.hdr.no_measures; //move on to next set of measures
        }//end for
}//AccessManager::updateDiskDataChunkPointerMembers

void descendDepth1stCostTree(
				 unsigned int maxDepth,
				 unsigned int numFacts,
				 const CostNode* const costRoot,
				 const string& factFile,
				 const BucketID& bcktID,
				 vector<DirChunk>* const dirVectp,
				 vector<DataChunk>* const dataVectp)
// precondition:
//	costRoot points at a tree, where BUCKET_THRESHOLD<= tree-size <= DiskBucket::bodysize
// postcondition:
//	The dir chunks and the data chunks of this tree have been filled with values
//	and are stored in heap space in two vectors: dirVectp and dataVectp, in the following manner:
//	We descend in a depth first manner and we store each node (Chunk) as we first visit it.
{
	if(!costRoot) {
	 	//empty sub-tree
	 	throw GeneralError("AccessManager::descendDepth1stCostTree  ==> Emtpy sub-tree!!\n");
	}
	// I. If this is not a leaf chunk
	if(costRoot->getchunkHdrp()->depth < maxDepth){
	     	// 1. Create the vector holding the chunk's entries.
	     	//      NOTE: the default DirEntry constructor should initialize the member BucketID
	     	//	with a BucketID::null constant.
	     	vector<DirEntry> entryVect(costRoot->getchunkHdrp()->totNumCells);

	     	// 2. Fill in those entries
	     	//    the current chunk (costRoot) will be stored in
	     	//    chunk slot == dirVectp->size()+dataVectp->size()
	     	//   the 1st child will be stored just after the current chunk
	     	unsigned int index = dirVectp->size() + dataVectp->size() + 1;
		vector<ChunkID>::const_iterator icid = costRoot->getcMapp()->getchunkidVectp()->begin();
		vector<CostNode*>::const_iterator ichild = costRoot->getchild().begin();
		// ASSERTION1: size of CostNode vectors is equal
		if(costRoot->getchild().size() != costRoot->getcMapp()->getchunkidVectp()->size())
			throw GeneralError("AccessManager::descendDepth1stCostTree ==> ASSERTION1: != vector sizes\n");
		while(ichild != costRoot->getchild().end() && icid != costRoot->getcMapp()->getchunkidVectp()->end()){
			// loop invariant: for current entry e holds:
			//		  e.bcktId == the same for all entries
			//		  e.chnkIndex == the next available chunk slot in the bucket
			//			after all the chunks of the sub-trees hanging from
			//			the previously inserted entries.
			DirEntry e;
			e.bcktId = bcktID;
                        e.chnkIndex = index;
			// insert entry at entryVect in the right offset (calculated from the Chunk Id)
			Coordinates c;
			(*icid).extractCoords(c);
			//ASSERTION2
			unsigned int offs = DirChunk::calcCellOffset(c, *costRoot->getchunkHdrp());
       			if(offs >= entryVect.size()){
       				throw GeneralError("AccessManager::descendDepth1stCostTree ==>ASSERTION2: entryVect out of range!\n");
       			}
			entryVect[offs] = e;
                        unsigned int numChunks = 0;
                	CostNode::countChunksOfTree(*ichild, maxDepth, numChunks);
			index = index + numChunks;
			ichild++;
			icid++;
		} //end while

	     	// 3. Now that entryVect is filled, create dirchunk object
		DirChunk newChunk(*costRoot->getchunkHdrp(), entryVect);

	     	// 4. Store new chunk in the corresponding vector
	     	dirVectp->push_back(newChunk);

	     	// 5. Descend to children
	     	for(vector<CostNode*>::const_iterator ichd = costRoot->getchild().begin();
	     	    ichd != costRoot->getchild().end(); ++ichd) {
                	try {
                		descendDepth1stCostTree(
                				maxDepth,
                				numFacts,
                				(*ichd),
                				factFile,
                				bcktID,
                				dirVectp,
                				dataVectp);
                	}
                      	catch(GeneralError& error) {
                      		GeneralError e("AccessManager::descendDepth1stCostTree ==> ");
                      		error += e;
                      		throw error;
                      	}
                      	//ASSERTION3
                       	if(dirVectp->empty() || dataVectp->empty()){
                       			throw GeneralError("AccessManager::descendDepth1stCostTree  ==>ASSERTION3: empty chunk vectors (possibly both)!!\n");
                       	}
	     	} // end for
	} //end if
	else { // II. this is a leaf chunk
		//ASSERTION4: depth check
		if(costRoot->getchunkHdrp()->depth != maxDepth)
			throw GeneralError("AccessManager::descendDepth1stCostTree ==>ASSERTION4: Wrong depth in leaf chunk!\n");

		// 1. Create the vector holding the chunk's entries and the compression bitmap
		//    (initialized to 0's).
	     	vector<DataEntry> entryVect(costRoot->getchunkHdrp()->rlNumCells);
	     	bit_vector cmprBmp(costRoot->getchunkHdrp()->totNumCells, false);

	     	// 2. Fill in those entries
        	// open input file for reading
        	ifstream input(factFile.c_str());
        	if(!input)
        		throw GeneralError("AccessManager::descendDepth1stCostTree ==> Error in creating ifstream obj\n");

        	string buffer;
        	// skip all schema staff and get to the fact values section
        	do{
        		input >> buffer;
        	}while(buffer != "VALUES_START");

        	// read on until you find prefix corresponding to current data chunk
        	string prefix = costRoot->getchunkHdrp()->id.getcid();
        	do {
	        	input >> buffer;
	        	if(input.eof())
	        		throw GeneralError("AccessManager::descendDepth1stCostTree ==> Can't find prefix in input file\n");
	        }while(buffer.find(prefix) != 0);

	        // now, we 've got a prefix match.
	        // Read the fact values for the non-empty cells of this chunk.
	        unsigned int factsPerCell = numFacts;
	        vector<measure_t> factv(factsPerCell);
		map<ChunkID, DataEntry> helpmap; // for temporary storage of entries
		int numCellsRead = 0;
	        do {
	        // loop invariant: read a line from fact file, containing the values of
		//     		   a single cell. All cells belong to chunk with id prefix
			for(int i = 0; i<factsPerCell; ++i){
				input >> factv[i];
			}

			DataEntry e(factsPerCell, factv);
			//Offset in chunk can't be computed until bitmap is created. Store
			// the entry temporarily in this map container, by chunkid
			ChunkID cellid(buffer);

			//ASSERTION5: no such id already exists in the map
			if(helpmap.find(cellid) != helpmap.end())
                                throw GeneralError("AccessManager::descendDepth1stCostTree ==>ASSERTION5: double entry for cell in fact load file\n");
			helpmap[cellid] = e;

			// insert entry at cmprBmp in the right offset (calculated from the Chunk Id)
			Coordinates c;
			cellid.extractCoords(c);
			//ASSERTION6
			unsigned int offs = DirChunk::calcCellOffset(c, *costRoot->getchunkHdrp());
       			if(offs >= cmprBmp.size()){
       				throw GeneralError("AccessManager::descendDepth1stCostTree ==>ASSERTION6: cmprBmp out of range!\n");
       			}
			cmprBmp[offs] = true; //this cell is non-empty

			numCellsRead++;

	        	//read next cell id (i.e. chunk  id)
	        	input >> buffer;
	        } while(buffer.find(prefix) == 0 && !input.eof()); // we are still under the same prefix
	                                                           // (i.e. data chunk)
		input.close();

		//ASSERTION7: number of non-empty cells read
		if(numCellsRead != costRoot->getchunkHdrp()->rlNumCells)
			throw GeneralError("AccessManager::descendDepth1stCostTree ==>ASSERTION7: Wrong number of non-empty cells\n");

		// now store into the entry vector of the data chunk
		for(map<ChunkID, DataEntry>::const_iterator map_i = helpmap.begin();
		    map_i != helpmap.end(); ++map_i) {
        		// insert entry at entryVect in the right offset (calculated from the Chunk Id)
        		Coordinates c;
        		map_i->first.extractCoords(c);

        		bool emptyCell = false;
        		unsigned int offs = DataChunk::calcCellOffset(c, cmprBmp,
        		                                             *costRoot->getchunkHdrp(), emptyCell);
        		//ASSERTION8: offset within range
       			if(offs >= entryVect.size())
       				throw GeneralError("AccessManager::descendDepth1stCostTree ==>ASSERTION8: (DataChunk) entryVect out of range!\n");

       			//ASSERTION9: non-empty cell
       			if(emptyCell)
               			throw GeneralError("AccessManager::descendDepth1stCostTree ==>ASSERTION9: access to empty cell!\n");
        		//store entry in vector
        		entryVect[offs] = map_i->second;
		}//end for

	     	// 3. Now that entryVect and cmprBmp are filled, create dataChunk object
		DataChunk newChunk(*costRoot->getchunkHdrp(), cmprBmp, entryVect);

	     	// 4. Store new chunk in the corresponding vector
	     	dataVectp->push_back(newChunk);
	} // end else
}// end of AccessManager::descendDepth1stCostTree
