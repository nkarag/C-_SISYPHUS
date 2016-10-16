/////////////////////////////////////////////////////////
// test unit: AccessManager::dataChunk2DiskDataChunk
////////////////////////////////////////////////////////

#include "Chunk.h"
#include "DiskDataChunk.h"

#include <cstdlib>

DiskDataChunk* dataChunk2DiskDataChunk(const DataChunk& datachnk, unsigned int numFacts, unsigned int maxDepth);
void printDiskDataChunk(const DiskDataChunk& chunk);
DataChunk* createDataChunkInstance(unsigned int nofacts);

main()
{
        cout<<"\nWhat is the number of facts in a cell?\n";
        unsigned int nofacts;
        cin>>nofacts;

        //create a DataChunk instance
        DataChunk* datachnkp = createDataChunkInstance(nofacts);

        cout<<"\nWhat is the Maxumum Chunking Depth?\n";
        unsigned int maxd;
        cin>>maxd;


        //call dataChunk2DiskDataChunk
        DiskDataChunk* diskchnkp = 0;
        try{
                diskchnkp = dataChunk2DiskDataChunk(*datachnkp, nofacts, maxd);
        }
        catch(const char* msg){
                cerr<<msg;
                exit(1);
        }
        delete datachnkp;

        //print contents of DiskDirChunk for testing
        printDiskDataChunk(*diskchnkp);

        delete diskchnkp;
}//end main

void printDiskDataChunk(const DiskDataChunk& chunk)
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

        //print the number of ace
        cout<<"No_ace: "<<chunk.no_ace<<endl;

        //print the bitmap
        cout<<"\nBITMAP:\n\t";
        for(int b=0; b<chunk.hdr.no_entries; b++){
                  (!chunk.testbit(b)) ? cout<<"0" : cout<<"1";
        }//end for

        //print the data entries
        cout<<"\nDiskDataChunk entries:\n";
        cout<<"---------------------\n";
        for(int i=0; i<chunk.no_ace; i++){
                cout<<"Data entry "<<i<<": ";
                for(int j=0; j<chunk.hdr.no_measures; j++){
                        cout<<chunk.entry[i].measures[j]<<", ";
                }//end for
                cout<<endl;
        }//end for
}//end printDiskDataChunk

DataChunk* createDataChunkInstance(unsigned int nofacts)
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

        //get the bitmap
        bit_vector cBmp;
        cout << "give me bitmap (exactly "<<hdr.totNumCells<<" bits - no whitespace in between bits): \n";
        char c;
        cin.ignore(); //get rid of the last '\n'
        for(int i=0; i<hdr.totNumCells; i++){
                cin.get(c); //read one character
                (c=='1') ? cBmp.push_back(true) : cBmp.push_back(false) ;
        }// end for

        //finally get the entries
        vector<DataEntry> dataentv;
        // read the measures from the user
        for(int i=0; i<hdr.rlNumCells; i++){
                DataEntry   new_ent;
                cout<<"Give measures for entry "<<i<<": ";
                for(int j=0; j<nofacts; j++){
                        float tmp;
                        cin>>tmp;
                        new_ent.fact.push_back(tmp);
                        cout<<new_ent.fact[j]<<", ";
                }//end for
                dataentv.push_back(new_ent);
                cout<<endl;
        }//end for

        return (new DataChunk(hdr,cBmp, dataentv));

}// end of createDirChunkInstance

DiskDataChunk* dataChunk2DiskDataChunk(const DataChunk& datachnk, unsigned int numFacts, unsigned int maxDepth)
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
		chnkp->allocBmp(chnkp->hdr.no_entries);
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
