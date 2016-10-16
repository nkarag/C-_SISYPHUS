#include "Chunk.h"
#include "DiskDirChunk.h"


#include <cstdlib>

/**
  * This function receives a Dirchunk instance. It allocates space in heap for a DiskDirChunk
  * and copies appropriately the contents of the input DirChunk to the new instance
  * of DiskDirChunk. It returns a pointer to the new DiskDirChunk
  *
  * @param dirchnk	the input DirChunk instance
  */
DiskDirChunk* dirChunk2DiskDirChunk(const DirChunk& dirchnk, unsigned int maxDepth);

DirChunk* createDirChunkInstance();

void printDiskDirChunk(const DiskDirChunk& chunk);

main()
{
        //create a DirChunk instance
        DirChunk* dirchnkp = createDirChunkInstance();

        cout<<"\nWhat is the Maxumum Chunking Depth?\n";
        unsigned int maxd;
        cin>>maxd;

        //call dirChunk2DiskDirChunk
        DiskDirChunk* diskchnkp = 0;
        try{
                diskchnkp = dirChunk2DiskDirChunk(*dirchnkp, maxd);
        }
        catch(const char* msg){
                cerr<<msg;
                exit(1);
        }

        //print contents of DiskDirChunk for testing
        printDiskDirChunk(*diskchnkp);
}//end main

DirChunk* createDirChunkInstance()
{
        //create a ChunkHeader
        ChunkHeader hdr;

        //get the chunk id
        cout<<"Give the chunk id: ";
        string id;
        cin>>id;
        hdr.id.setcid(id);
        cout<<endl;

        //get the chunking depth
        cout<<"Give me the chunking depth: ";
        cin>>hdr.depth;
        cout<<endl;

        //get the number of dimensions
        cout<<"Give me the num of dims: ";
        cin>>hdr.numDim;

        //get order-code ranges
        cout<<"Give order code ranges for each dim:\n";
        for(int i=0; i<hdr.numDim; i++){
                LevelRange rng;
                cout<<"\t Left: ";
                cin>>rng.leftEnd;
                cout<<"\tRight: ";
                cin>>rng.rightEnd;
                hdr.vectRange.push_back(rng);
        }//end for
        cout<<endl;

        cout<<"Give total number of cells: ";
        cin>>hdr.totNumCells;
        cout<<endl;

        cout<<"Give real number of cells: ";
        cin>>hdr.rlNumCells;
        cout<<endl;

        cout<<"Give size: ";
        cin>>hdr.size;
        cout<<endl;

        //get DirEntries
        vector<DirEntry> direntv;
        for(int i=0; i<hdr.totNumCells; i++){
                DirEntry e;
                e.bcktId.rid = i;
                e.chnkIndex =  hdr.totNumCells-i;
                direntv.push_back(e);
        }//end for

        return (new DirChunk(hdr,direntv));

}// end of createDirChunkInstance

void printDiskDirChunk(const DiskDirChunk& chunk)
{
        //print header
        cout<<"**************************************"<<endl;
        cout<<"Depth: "<<int(chunk.hdr.depth)<<endl;
        cout<<"No_dims: "<<int(chunk.hdr.no_dims)<<endl;
        cout<<"No_measures: "<<int(chunk.hdr.no_measures)<<endl;
        cout<<"No_entries: "<<chunk.hdr.no_entries<<endl;
        //print chunk id
        for(int i=0; i<chunk.hdr.depth; i++){
                for(int j=0; j<chunk.hdr.no_dims; j++){
                        cout<<(chunk.hdr.chunk_id)[i].ordercodes[j];
                        (j==int(chunk.hdr.no_dims)-1) ? cout<<"." : cout<<"|";
                }//end for
        }//end for
        cout<<endl;
        //print the order code ranges per dimension level
        for(int i=0; i<chunk.hdr.no_dims; i++)
                cout<<"Dim "<<i<<" range: left = "<<(chunk.hdr.oc_range)[i].left<<", right = "<<(chunk.hdr.oc_range)[i].right<<endl;

        //print dir entries (at long last!!)
        cout<<"\nDiskDirChunk entries:\n";
        cout<<"---------------------\n";

        for(int i=0; i<chunk.hdr.no_entries; i++){
                cout<<chunk.entry[i].bucketid.rid<<", "<<chunk.entry[i].chunk_slot<<endl;
        }//end for

}//end printDiskDirChunk

DiskDirChunk* dirChunk2DiskDirChunk(const DirChunk& dirchnk, unsigned int maxDepth)
// precondition:
//	dirchnk is a DirChunk instance filled with valid entries. maxDepth is the maximum depth of
//      the cube in question.
// postcondition:
//	A DiskDirChunk has been allocated in heap space that contains the values of
//	dirchnk and a pointer to it is returned.	
{
	//ASSERTION 1.0 : depth and chunk-id compatibility
	if(dirchnk.gethdr().depth != dirchnk.gethdr().id.get_chunk_depth())
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
