#include "Chunk.h"

CellMap* Chunk::scanFileForPrefix(const string& factFile,const string& prefix, bool isDataChunk = false)
//precondition:
//      We assume that the fact file contains in each line: <cell chunk id>\t<cell value1>\t<cell value2>...<\t><cell valueN>
//      All the chunk ids in the file correspond only to grain level data points. Also all chunk id contain at least 2 domains, i.e.,
//      all dimensions have at least a 2-level hierarchy. "prefix" input parameter corresponds to the chunk id of a specific chunk (source chunk)
//      whose cells we want to search for in the input file. "isDataChunk" indicates whether the latter is data chunk or a directory chunk.
//processing:
//      For directory chunks when we find a match, then we insert in a CellMap the prefix+the "next domain"
//      from the chunk-id that matched. All following matches with this "next domain" are discarded
//                                                              -----------------------
//      (i.e. no insertion takes place).
//      For data chunks if we do find a second match with the same "next domain" following the prefix, then
//      we throw an exception, since that would mean that we have found values for the same cell, more
//      than once in the input file.
//postcondition:
//      A pointer to a CellMap is returned containing the existing cells of the source chunk that were found in the input file
{
	// open input file for reading
	ifstream input(factFile.c_str());
	if(!input)
		throw GeneralError("Chunk::scanFileForPrefix ==> Error in creating ifstream obj, in Chunk::scanFileForPrefix\n");

        #ifdef DEBUGGING
              cerr<<"Chunk::scanFileForPrefix ==> Just opened file: "<<factFile.c_str()<<endl;
              cerr<<"Chunk::scanFileForPrefix ==> Prefix is : "<<prefix<<endl;
        #endif

	string buffer;
	// skip all schema staff and get to the fact values section
	do{
		input >> buffer;
	}while(buffer != "VALUES_START");

	CellMap* mapp = new CellMap;
	input >> buffer;
	while(buffer != "VALUES_END"){
                /*#ifdef DEBUGGING
                	cerr<<"Chunk::scanFileForPrefix ==> buffer = "<<buffer<<endl;
                #endif*/
		if(prefix == "root"){ //then we are scanning for the root chunk
                	// get the first domain from each entry and insert it into the CellMap
			string::size_type pos = buffer.find("."); // Find character "." in buffer starting from left
			if (pos == string::npos){
				throw GeneralError("Chunk::scanFileForPrefix ==> Assertion 1: ChunkID syntax error: no \".\" in id in fact load file\n");
			}
			string child_chunk_id(buffer,0,pos);
                        //#ifdef DEBUGGING
                        //      cerr<<"Chunk::scanFileForPrefix ==> child_chunk_id = "<<child_chunk_id<<endl;
                        //#endif
			mapp->insert(child_chunk_id);
		}
		else {
			string::size_type pos = buffer.find(prefix);
			if(pos==0) { // then we got a prefix match
        			//get the next domain
        			// find pos of first character not of prefix
        			//string::size_type pos = buffer.find_first_not_of(prefix); // get the position after the prefix (this should be a ".")
        			pos = prefix.length(); // this must point to a "."
        			if (buffer[pos] != '.') {
                                        throw GeneralError("Chunk::scanFileForPrefix ==> Assertion2:  ChunkID syntax error: no \".\" (after input prefix) in id in fact load file\n");
                                }//end if
        			pos = pos + 1; //move on one position to get to the first character
                                /*#ifdef DEBUGGING
                                      cerr<<"pos = "<<pos<<endl;
                                #endif*/
        			string::size_type pos2 = buffer.find(".", pos); // find the end of the next domain
                                /*#ifdef DEBUGGING
                                      cerr<<"pos2 = "<<pos2<<endl;
                                #endif*/
        			string nextDom(buffer,pos,pos2-pos); //get next domain
                                /*#ifdef DEBUGGING
                                      cerr<<"nextDom = "<<nextDom<<endl;
                                #endif*/
        			string child_chunk_id = prefix + string(".") + nextDom; // construct child chunk's id
                                #ifdef DEBUGGING
                                      cerr<<"Chunk::scanFileForPrefix ==> child_chunk_id = "<<child_chunk_id<<endl;
                                #endif
        			// insert into CellMap
       				bool firstTimeInserted = mapp->insert(child_chunk_id);
       				if(!firstTimeInserted && isDataChunk) {
                               		//then we have found a double entry, i.e. the same cell is given a value more than once
       					string msg = string ("Chunk::scanFileForPrefix ==> Error in input file: double chunk id: ") + child_chunk_id;
       					throw GeneralError(msg.c_str());
       				}//end if
        		}// end if
		} // end else
		// read on until the '\n', in order to skip the fact values
		getline(input,buffer);
		// now, read next id
		input >> buffer;
	}//end while
	input.close();
        #ifdef DEBUGGING
		/*cerr<<"Printing contents of CellMap : \n";
                for(vector<ChunkID>::const_iterator i = mapp->getchunkidVectp()->begin();
                	  i != mapp->getchunkidVectp()->end(); ++i){

  			cerr<<(*i).getcid()<<", "<<endl;
                }*/
                if(mapp->getchunkidVectp()->empty()) {
                 	cerr<<"Chunk::scanFileForPrefix ==> CellMapp is empty!!!\n";
                }
        #endif
	return mapp;
} //end Chunk::scanFileforPrefix


size_t DirChunk::calculateStgSizeInBytes(short int depth, short int maxDepth,
					unsigned int numDim, unsigned int totNumCells, short int local_depth)
//precondition:
//	depth is the depth of the DirChunk we wish to calculate its storage size.
//	This DirChunk can be also the root chunk (depth==Chunk::MIN_DEPTH). numDim is
//	the number of dimensions of the cube and totNumCells is the total number of entries including
//	empty entries (i.e.,cells)
//postcondition:
//	the size in bytes consumed by the corresponding DiskDirChunk structure is returned.
{
	//ASSERTION 1: assert that this is a valid depth
	if(depth > maxDepth)
		throw GeneralError("DirChunk::calculateStgSizeInBytes ==> Invalid depth\n");
	// first add the size of the static parts
	size_t size = sizeof(DiskDirChunk);

	//Now, the size of the dynamic parts:
	//1. the chunk id (in DiskChunkHeader)

        //fist find the number of domains in the chunk id
        int noDomains;
        try{
        	noDomains = DiskChunkHeader::getNoOfDomainsFromDepth(depth, local_depth);
        }
	catch(GeneralError& error) {
        	GeneralError e("DirChunk::calculateStgSizeInBytes ==> ");
        	error += e;
        	throw error;
        }

	size += noDomains * ( sizeof(DiskChunkHeader::Domain_t) + numDim*sizeof(DiskChunkHeader::ordercode_t) );

	//2. the order-code ranges (in DiskChunkHeader)
	size += numDim * sizeof(DiskChunkHeader::OrderCodeRng_t);

	//3. The number of dir entries (in DiskDirChunk)
	size += totNumCells * sizeof(DiskDirChunk::DirEntry_t);

	return size;
}//end of DirChunk::calculateStgSizeInBytes

size_t DataChunk::calculateStgSizeInBytes(short int depth, short int maxDepth, unsigned int numDim,
				unsigned int totNumCells, unsigned int rlNumCells, unsigned int numfacts, short int local_depth)
//precondition:
//	depth is the depth of the DataChunk we wish to calculate its storage size.
//	numDim is the number of dimensions of the cube and totNumCells is the total number of entries
//	including empty entries (i.e.,cells). rlNumCells is the real number of data entries, i.e.,
//	only the non-empty cells included. numfacts is the number of facts inside a data entry
//	(i.e., cell).
//postcondition:
//	the size in bytes consumed by the corresponding DiskDataChunk structure is returned.
{
	//ASSERTION 1
	if(depth != maxDepth)
		throw GeneralError("DataChunk::calculateStgSizeInBytes ==> Invalid depth\n");

	// first add the size of the static parts
	size_t size = sizeof(DiskDataChunk);

	//Now, the size of the dynamic parts:
	//1. the chunk id (in DiskChunkHeader)

        //fist find the number of domains in the chunk id
        int noDomains;
        try{
        	noDomains = DiskChunkHeader::getNoOfDomainsFromDepth(depth, local_depth);
        }
	catch(GeneralError& error) {
        	GeneralError e("DataChunk::calculateStgSizeInBytes ==> ");
        	error += e;
        	throw error;
        }

	size += noDomains * ( sizeof(DiskChunkHeader::Domain_t) + numDim*sizeof(DiskChunkHeader::ordercode_t) );

	//2. the order-code ranges (in DiskChunkHeader)
	size += numDim * sizeof(DiskChunkHeader::OrderCodeRng_t);

	//3. The number of data entries (in DiskDataChunk)
	size_t entry_size = sizeof(DiskDataChunk::DataEntry_t) + (numfacts * sizeof(measure_t));
	int no_words = bmp::numOfWords(totNumCells); //number of words for bitmap
	size += (rlNumCells * entry_size) + no_words*sizeof(bmp::WORD);


	/* **** In this version of Sisyphus ALL data chunks will maintain a bitmap ****

	if(float(rlNumCells)/float(totNumCells) > SPARSENESS_THRSHLD){
		// no need for compression : i.e. size = totNumCells * entry_size
		size += totNumCells * entry_size;
	}
	else {// need for compression bitmap
		// number of unsigned integers used to represent this bitmap of size hdrp->totNumCells
		//int no_words = hdrp->totNumCells/bmp::BITSPERWORD + 1;
		int no_words = bmp::numOfWords(totNumCells);
		size += (rlNumCells * entry_size) + no_words*sizeof(bmp::WORD);
	} */

	return size;
}// end of DataChunk::calculateStgSizeInBytes
