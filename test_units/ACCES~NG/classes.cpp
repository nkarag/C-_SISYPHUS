#include "classes.h"
#include "DiskStructures.h"

const int LevelRange::NULL_RANGE=-1;


//************************ CostNode ******************************************//
CostNode::~CostNode(){
	delete chunkHdrp;
	delete cMapp;
	//delete child;
	for (	vector<CostNode*>::iterator iter = child.begin();
		iter != child.end();
		++iter )
	{
       		delete (*iter);
	}
}

const CostNode & CostNode::operator=(const CostNode & other)
{
	if(this != &other) {
		// deallocate current data
		delete chunkHdrp;
		delete cMapp;
		//delete child;
    		for (
			vector<CostNode*>::iterator iter = child.begin();
			iter != child.end();
			++iter
		)
		{
         		delete (*iter);
		}

		// duplicate other's data
		chunkHdrp = new ChunkHeader(*(other.getchunkHdrp()));
		cMapp = new CellMap(*(other.getcMapp()));
		//child = new vector<CostNode>(*(other.getchild()));
		child = other.getchild();
	}
	return (*this);
}
					
void CostNode::countDirChunksOfTree(const CostNode* const costRoot,
					 unsigned int maxdepth,
					 unsigned int& total)
// precondition:
//	costRoot points at a CostNode corresponding to a directory or a data chunk. total is either 0 (when first
//	called - 1st recursion), or it contains
//	the number of parent directory chunks all the way up to the root (with which the first call
//	was made).maxdepth contains the maximum depth of the cube in question.
// postcondition:
//	the returned result equals the number of directory chunks under costRoot
{
	//ASSERTION1: not a null pointer
	if(!costRoot)
		throw "ASSERTION1 in CostNode::countDirChunksOfTree ==> null pointer\n";

	//ASSERTION2: it has a valid depth
	if(costRoot->getchunkHdrp()->depth > maxdepth + 1 ||
						costRoot->getchunkHdrp()->depth < Chunk::MIN_DEPTH)
		throw "ASSERTION2 in CostNode::countDirChunksOfTree ==> invalid depth\n";

	// if this is a data chunk
	if(costRoot->getchunkHdrp()->depth == maxdepth + 1)
		return; //dont add anything

	//This is a dir chunk so add 1 to the total
	total += 1;

	//if this is the maximum depth then
	if(costRoot->getchunkHdrp()->depth == maxdepth){
		//return the total
		return;
	}//end if
	else{   //this is not a max depth chunk
		//for each non-empty cell
		for(vector<CostNode*>::const_iterator cnode_i = costRoot->getchild().begin();
			cnode_i != costRoot->getchild().end(); cnode_i++){
			//add the number of dir chunks of each sub-tree hanging from there
			countDirChunksOfTree(*cnode_i, maxdepth, total);
		}//end for
	}//end else
}// CostNode::countDirChunksOfTree

void CostNode::countDataChunksOfTree(const CostNode* const costRoot,
						unsigned int maxdepth,
						unsigned int& total)
// precondition:
//	costRoot points at a CostNode corresponding to a directory or a data chunk. total is either 0 (when first
//	called - 1st recursion), or it contains
//	the number of data chunks under costRoot already visited. maxdepth contains the maximum depth of the cube in question.
// postcondition:
//	the returned result equals the number of data chunks under costRoot
{
	//ASSERTION1: not a null pointer
	if(!costRoot)
		throw "ASSERTION1 in CostNode::countDataChunksOfTree ==> null pointer\n";

	//ASSERTION2: it has a valid depth
	if(costRoot->getchunkHdrp()->depth > maxdepth + 1 ||
						costRoot->getchunkHdrp()->depth < Chunk::MIN_DEPTH)
		throw "ASSERTION2 in CostNode::countDataChunksOfTree ==> invalid depth\n";

	// if this is a data chunk
	if(costRoot->getchunkHdrp()->depth == maxdepth + 1){
		//add one to the total
		total += 1;
		return; //return to the parent node
	}//end if

	//This is a dir chunk so descend to its children nodes

       	//for each non-empty cell
       	for(vector<CostNode*>::const_iterator cnode_i = costRoot->getchild().begin();
       		cnode_i != costRoot->getchild().end(); cnode_i++){
       		//add the number of dir chunks of each sub-tree hanging from there
       		countDataChunksOfTree(*cnode_i, maxdepth, total);
       	}//end for
}//CostNode::countDataChunksOfTree


						
void CostNode::countChunksOfTree(const CostNode* const costRoot,
					unsigned int maxdepth,
					unsigned int& total)
// precondition:
//	costRoot points at a CostNode corresponding to a directory or a data chunk. total is either 0 (when first
//	called - 1st recursion), or it contains
//	the number of chunks (dir or data) under costRoot already visited.
//      maxdepth contains the maximum depth of the cube in question.
// postcondition:
//	the returned result equals the number of chunks (dir or data) under costRoot
{
	//ASSERTION1: not a null pointer
	if(!costRoot)
		throw "ASSERTION1 in CostNode::countChunksOfTree ==> null pointer\n";

	//ASSERTION2: it has a valid depth
	if(costRoot->getchunkHdrp()->depth > maxdepth + 1 ||
						costRoot->getchunkHdrp()->depth < Chunk::MIN_DEPTH)
		throw "ASSERTION2 in CostNode::countChunksOfTree ==> invalid depth\n";

	// if this is a data chunk
	if(costRoot->getchunkHdrp()->depth == maxdepth + 1){
		//add one to the total
		total += 1;
		return; //return to the parent node
	}//end if

	//This is a dir chunk so add one to the total and descend to its children nodes
	total += 1;
       	//for each non-empty cell
       	for(vector<CostNode*>::const_iterator cnode_i = costRoot->getchild().begin();
       		cnode_i != costRoot->getchild().end(); cnode_i++){
       		//add the number of chunks of each sub-tree hanging from there
       		countChunksOfTree(*cnode_i, maxdepth, total);
       	}//end for
}//CostNode::countChunksOfTree

void CostNode::printTree(CostNode* root, ofstream& out)
{
        /*#ifdef DEBUGGING
              cerr<<"CostNode::printTree ==> Inside CostNode::printTree" << endl;
        #endif*/
	if(!root){
		return;
	}

       	out << "CHUNK : "<<root->getchunkHdrp()->id.getcid()<<endl;
       	out << "----------------------------------------"<<endl;
        out <<"\tDepth = "<<root->getchunkHdrp()->depth<<", totNumCells = "
       		<<root->getchunkHdrp()->totNumCells<<", rlNumCells = "
       		<<root->getchunkHdrp()->rlNumCells<<endl;

       	//out <<"\tDepth = "<<root->getchunkHdrp()->depth<<", NumOfDims = "
       	//	<<root->getchunkHdrp()->numDim<<", totNumCells = "
       	//	<<root->getchunkHdrp()->totNumCells<<", rlNumCells = "
       	//	<<root->getchunkHdrp()->rlNumCells<<", size(bytes) = "
       	//	<<root->getchunkHdrp()->size<<endl;

       	// print levels
       	for (	vector<LevelRange>::const_iterator i = root->getchunkHdrp()->vectRange.begin();
  		i != root->getchunkHdrp()->vectRange.end();	++i){
  			out<<"\t"<<(*i).dimName<<"("<<(*i).lvlName<<") : ["<<(*i).leftEnd<<", "<<(*i).rightEnd<<"]"<<endl;
 	}
        // print cell chunk ids
	out<<"\tExisting Cells (Chunk-IDs) : "<<endl;
        #ifdef DEBUGGING
        	if(root->getcMapp()->getchunkidVectp()->empty()) {
        	 	cerr<<"CostNode::printTree ==> CellMap Chunkid vector is empty!!!"<<endl;
        	}
        #endif
      	for (	vector<ChunkID>::const_iterator iter = root->getcMapp()->getchunkidVectp()->begin();
   		iter != root->getcMapp()->getchunkidVectp()->end();
   		++iter	){

                /*#ifdef DEBUGGING
                      cerr<<"CostNode::printTree ==> printing cell chunk-ids..." << endl;
                #endif*/

   		out<<"\t"<<(*iter).getcid()<<", "<<endl;
   	}
        #ifdef DEBUGGING
        	cerr<<"CostNode::printTree ==> Testing CostNode children:\n";
	      	for (	vector<CostNode*>::const_iterator j = root->getchild().begin();
   		j != root->getchild().end();
   		++j	){
   			cerr<<"\t"<<(*j)->getchunkHdrp()->id.getcid()<<endl;
   		}
        #endif
        /*#ifdef DEBUGGING
        	cerr<<"CostNode::printTree ==> Testing CostNode children:\n";
	      	for (	vector<CostNode>::const_iterator j = root->getchild()->begin();
   		j != root->getchild()->end();
   		++j	){
   			cerr<<"\t"<<(*j).getchunkHdrp()->id.getcid()<<endl;
   		}
        #endif	*/


   	out <<"\n\n";
   	// descend to children
      	for (	vector<CostNode*>::const_iterator j = root->getchild().begin();
   		j != root->getchild().end();
   		++j	){
		CostNode::printTree(*j, out);
   	}

      	/*for (	vector<CostNode>::const_iterator j = root->getchild()->begin();
   		j != root->getchild()->end();
   		++j	){
		CostNode::printTree(const_cast<vector<CostNode>::iterator>(j), out);
   	}*/
} // end of CostNode::printTree



//********************* ChunkID ***********************************//

void ChunkID::extractCoords(Coordinates& c)const
{
	if(cid == "root")
		throw "ChunkID::extractCoords ==> Can't extract coords from \"root\"\n";

	// 1. get last domain
	string::size_type pos = cid.rfind("."); //find last "."
	string lastdom;
	if (pos == string::npos){ //then no "." found. This must be a child of the root chunk.
		lastdom = cid;
	}
	else {
		pos += 1; // move on to the first char of the last domain
		string::size_type ldom_length = cid.length()-pos;
		lastdom = cid.substr(pos, ldom_length); // read substring cid[pos]..cid[pos+ldom_length -1]
	}

	// 2. update c
	string::size_type begin = 0;
	string::size_type end = lastdom.find("|", begin); // get 1st "|", i.e. coordinate for 1st dimension
	while(end != string::npos){
        	string coordstr(lastdom, begin, end-begin); // substring lastdom[begin]...lastdom[begin+(end-begin)-1]
        	int coord = (unsigned int)atoi(coordstr.c_str());
        	c.cVect.push_back(coord);
        	c.numCoords += 1;
        	// read on
        	begin = end + 1;
        	end = lastdom.find("|", begin);
        } //end while
        // repeat once more for the last dimension
       	string coordstr(lastdom, begin, lastdom.length()-begin); // substring lastdom[begin]...lastdom[begin+l-1], l=lastdom.length()-begin
       	int coord = (unsigned int)atoi(coordstr.c_str());
       	c.cVect.push_back(coord);
       	c.numCoords += 1;

} // end of ChunkID::extractCoords


//*************************** CellMap ****************************************//
CellMap::CellMap() : chunkidVectp(new vector<ChunkID>) {}
CellMap::~CellMap() { delete chunkidVectp; }

/*CellMap::CellMap(CellMap const & map) : chunkidVect(map)
{
}*/

CellMap::CellMap(CellMap const & map) {
        // copy the data
	chunkidVectp = new vector<ChunkID>(*(map.getchunkidVectp()));
}

//reference version
/*void CellMap::setchunkidVect(const vector<ChunkID>& chv)
{
	chunkidVect = chv;
}*/
// pointer version
void CellMap::setchunkidVectp(vector<ChunkID>* const chv)
{
	chunkidVectp = chv;
}

CellMap const& CellMap::operator=(CellMap const& other)
{
	if(this != &other) {
		// deallocate current data
		delete chunkidVectp;
		// duplicate other's data
		chunkidVectp = new vector<ChunkID>(*(other.getchunkidVectp()));
	}
	return (*this);
}

// pointer version
bool CellMap::insert(const string& id)
{
	if(id.empty())
		throw "Error inside CellMap::insert : empty chunk id!\n";
	// check if the chunk id already exists
	ChunkID newId(id);
	vector<ChunkID>::iterator result = find(chunkidVectp->begin(), chunkidVectp->end(), newId);
	/*#ifdef DEBUGGING
		cerr<<"CellMap::insert ==> result =  "<<result<<", chunk id to insert = "<<newId.getcid()<<endl;
	#endif*/
	if(result == chunkidVectp->end()){
         	// OK its a new one
         	chunkidVectp->push_back(newId);
         	#ifdef DEBUGGING
         		cerr<<"CellMap::insert ==> Just inserted into Cellmap id : "<<chunkidVectp->back().getcid()<<endl;
         	#endif

		return true;
	}
	return false;
}

bool operator==(const ChunkID& c1, const ChunkID& c2)
{
	return (c1.cid == c2.cid);
}

bool operator<(const ChunkID& c1, const ChunkID& c2)
{
	return (c1.cid < c2.cid);
}
//************************* DirChunk **************************//
unsigned int DirChunk::calcCellOffset(const Coordinates& coords, const ChunkHeader& hdr)
// precondition:
//	coords contain a valid set of coordinates in a dir chunk. The order of the coords in the
//	vector member from left to right corresponds to major-to-minor positioning of the cells
//	hdr corresponds to the chunk header of the dir chunk in question.
// postcondition:
//	the corresponding cell offset is returned.  This offset is a value in the range [0..totNumCells-1] and
//	can be used as an index value for access of the cell in an array (or vector) of the form: array[totNumCells]
{
	//ASSERTION1: not an empty vector
	if(coords.cVect.empty())
		throw "DirChunk::calcCellOffset ==> ASSERTION1: empty coord vector\n";

        //ASSERTION2: if there are pseudo levels, then the NULL ranges must correspond to the pseudo coordinates
        for(int c = 0; c<coords.numCoords; c++){
                  // if this is a pseudo coordinate
                  if(coords.cVect[c] == Level_Member::PSEUDO_CODE){
                        //then the corresponding range should be NULL
                        if(hdr.vectRange[c].leftEnd != LevelRange::NULL_RANGE ||
                                                hdr.vectRange[c].rightEnd != LevelRange::NULL_RANGE)
                                throw "DirChunk::calcCellOffset ==> ASSERTION2: mismatch in position of pseudo levels\n";
                  }//end if
        }//end for

        //first remove Pseudo-code coordinate values
        Coordinates newcoords;
        coords.excludePseudoCoords(newcoords);

        //Also remove null level-ranges
        vector<LevelRange> newvectRange;
        hdr.excludeNULLRanges(newvectRange);

        //ASSERTION3: same number of pseudo levels found
        if(newvectRange.size() != newcoords.numCoords || newvectRange.size() != newcoords.cVect.size())
                throw "DirChunk::calcCellOffset ==> ASSERTION3: mismatch in no of pseudo levels\n";

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// offset(Cn,Cn-1,...,C1) = Cn*card(Dn-1)*...*card(D1) + Cn-1*card(Dn-2)*...*card(D1) + ... + C2*card(D1) + C1
	// where
	// card(Di) = the number of values along dimension Di
        // NOTE: Cn,Cn-1, etc., are assumed to be normalized to reflect as origin the coordinate 0
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//init offset
	unsigned int offset = 0;
	//for each coordinate
	for(int d=0; d<newcoords.numCoords; d++){
		//compute product of cardinalities
		// init product total
		unsigned int prod = newcoords.cVect[d] - newvectRange[d].leftEnd; // normalize coordinate to 0 origin
		//for each dimension (except for d) get cardinality and multiply total product
		for(int dd = d+1; dd<newcoords.numCoords; dd++){
			//ASSERTION4: proper level ranges
			if(newvectRange[dd].rightEnd < newvectRange[dd].leftEnd)
				throw "DirChunk::calcCellOffset ==> ASSERTION4: invalid level range\n";
			//ASSERTION5: not null range
			if(newvectRange[dd].leftEnd == LevelRange::NULL_RANGE ||
                                                newvectRange[dd].rightEnd == LevelRange::NULL_RANGE)
				throw "DirChunk::calcCellOffset ==> ASSERTION5: NULL level range\n";
			//get cardinality for this dimension
			int card = newvectRange[dd].rightEnd - newvectRange[dd].leftEnd + 1;
			// multiply total with current cardinality
			prod *= card;
		}//end for

		//add product to the offset
		offset += prod;
	}//end for
	return offset;
}// end DirChunk::calcCellOffset

size_t DirChunk::calculateStgSizeInBytes(unsigned int depth, unsigned int maxDepth,
					unsigned int numDim, unsigned int totNumCells)
//precondition:
//	depth is the depth of the DirChunk we wish to calculate its storage size.
//	This DirChunk can be also the root chunk (depth==Chunk::MIN_DEPTH). numDim is
//	the number of dimensions of the cube and totNumCells is the total number of entries including
//	empty entries (i.e.,cells)
//postcondition:
//	the size in bytes consumed by the corresponding DiskDirChunk structure is returned.
{
	//ASSERTION 1
	if(depth >= maxDepth)
		throw "DirChunk::calculateStgSizeInBytes ==> Invalid depth\n";
	// first add the size of the static parts
	size_t size = sizeof(DiskDirChunk);
		
	//Now, the size of the dynamic parts:
	//1. the chunk id (in DiskChunkHeader)
	if(depth>Chunk::MIN_DEPTH)
		//then this is NOT a root chunk.
		//for the root chunk, no chunk id will be stored
		size += depth*(sizeof(DiskChunkHeader::Domain_t) + numDim*sizeof(DiskChunkHeader::ordercode_t));

	//2. the order-code ranges (in DiskChunkHeader)
	size += numDim * sizeof(DiskChunkHeader::OrderCodeRng_t);
	
	//3. The number of dir entries (in DiskDirChunk)
	size += totNumCells * sizeof(DiskDirChunk::DirEntry_t);		
	return size;
}//end of DirChunk::calculateStgSizeInBytes

//*********************** DataChunk **************************//
unsigned int DataChunk::calcCellOffset(const Coordinates& coords)
{
        return 0;
}// end of DataChunk::calcCellOffset(const Coordinates& coords)

unsigned int DataChunk::calcCellOffset(const Coordinates& coords, const bit_vector& bmp,
                                        const ChunkHeader& hdr, bool& isEmpty)
// precondition:
//	coords contain a valid set of coordinates in a data chunk. The order of the coords in the
//	vector member from left to right corresponds to major-to-minor positioning of the cells. Only the non-empty
//	cells have been allocated and we use the bitmap bmp to locate empty cells.
//	hdr corresponds to the chunk header of the data chunk in question.
// postcondition:
//	the corresponding cell offset is returned. This offset is a value in the range [0..realNumCells-1] and
//	can be used as an index value for access of the cell in an array (or vector) of the form: array[realNumCells]
//      The flag isEmpty is set to true if the requested cell corresponds to a 0 bit in the bitmap.
{
	//ASSERTION1: not an empty vector
	if(coords.cVect.empty())
		throw "DataChunk::(static)calcCellOffset ==> ASSERTION1: empty coord vector\n";

	//ASSERTION2: not an empty vector
	if(bmp.empty())
		throw "DataChunk::(static)calcCellOffset ==> ASSERTION2: empty bitmap\n";

        //ASSERTION3: this is a data chunk therefore no pseudo coords must exist
        for(int c = 0; c<coords.numCoords; c++){
                  // if this is a pseudo coordinate
                  if(coords.cVect[c] == Level_Member::PSEUDO_CODE){
                        //then the corresponding range should be NULL
                                throw "DataChunk::calcCellOffset ==> ASSERTION3: pseudo coord in data chunk coordinates!\n";
                  }//end if
        }//end for


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// comprOffset(Cn,Cn-1,...,C1) = offset(Cn,Cn-1,...,C1) - number_of_zeros(offset(Cn,Cn-1,...,C1))
	//
	// where
	// offset(Cn,Cn-1,...,C1) = Cn*card(Dn-1)*...*card(D1) + Cn-1*card(Dn-2)*...*card(D1) + ... + C2*card(D1) + C1
	// card(Di) = the number of values along dimension Di
	// number_of_zeros(i) = the number of zeros in the bitmap from the beginning up to the ith bit (not included)
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//init offset
        unsigned int offset = 0;
        try{
	        offset = DirChunk::calcCellOffset(coords, hdr);
        }
        catch(const char* msg){
		string m("DataChunk::calcCellOffset ==> ");//"Exception from Chunk::scanFileForPrefix, in Chunk::createCostTree : ");
		m += msg;
                throw m.c_str();
        }

	// set empty-cell flag
	(!bmp[offset]) ? isEmpty = true : isEmpty = false;

	//ASSERTION4: proper offset in bitmap
	if(offset > bmp.size())
		throw "DataChunk::(static)calcCellOffset ==> ASSERTION4: wrong offset in bitmap\n";

	//calculate number of zeros
	int numZeros = 0;
	for(int bit=0; bit<offset; bit++){
		// if current bit is zero add one to the zero counter
		if (bmp[bit] == false)
		        numZeros++;
	}//end for

	return (offset - numZeros);
}// end of DataChunk::calcCellOffset(const Coordinates& coords, const bit_vector& bmp)

size_t DataChunk::calculateStgSizeInBytes(unsigned int depth, unsigned int maxDepth, unsigned int numDim,
				unsigned int totNumCells, unsigned int rlNumCells, unsigned int numfacts)
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
		throw "DataChunk::calculateStgSizeInBytes ==> Invalid depth\n";

	// first add the size of the static parts
	size_t size = sizeof(DiskDataChunk);
	
	//Now, the size of the dynamic parts:
	//1. the chunk id (in DiskChunkHeader)
	size += depth*(sizeof(DiskChunkHeader::Domain_t) + numDim*sizeof(DiskChunkHeader::ordercode_t));

	//2. the order-code ranges (in DiskChunkHeader)
	size += numDim * sizeof(DiskChunkHeader::OrderCodeRng_t);
	
	//3. The number of data entries (in DiskDataChunk)
	size_t entry_size = sizeof(DiskDataChunk::DataEntry_t) + (numfacts * sizeof(measure_t));
	int no_words = ::numOfWords(totNumCells); //number of words for bitmap
	size += (rlNumCells * entry_size) + no_words*sizeof(WORD);	
	
	
	/* **** In this version of Sisyphus ALL data chunks will maintain a bitmap ****
	
	if(float(rlNumCells)/float(totNumCells) > SPARSENESS_THRSHLD){
		// no need for compression : i.e. size = totNumCells * entry_size
		size += totNumCells * entry_size;
	}
	else {// need for compression bitmap		
		// number of unsigned integers used to represent this bitmap of size hdrp->totNumCells
		//int no_words = hdrp->totNumCells/BITSPERWORD + 1;
		int no_words = ::numOfwords(totNumCells);
		size += (rlNumCells * entry_size) + no_words*sizeof(WORD);	
	} */
	return size;	
}// end of DataChunk::calculateStgSizeInBytes


//----------------------- Coordinates --------------------------------------------------------------------//
inline void Coordinates::excludePseudoCoords(Coordinates & newcoords) const
// precondition: newcoords contains an empty vector of coordinates. (*this) contains a set of coordinate values,
//      which some of them might be pseudo order codes.
// postcondition:
//      newcoords contains all the coordinates of (*this) with the same order, without the pseudo coordinate values.
//      The state of "this" object remains unchanged!
//
{
        //ASSERTION1: newcoords is empty
        if(!newcoords.cVect.empty())
                throw "Coordinates::excludePseudoCoords ==> ASSERTION1: non-empty vector\n";
        for(vector<int>::const_iterator i = cVect.begin(); i != cVect.end(); i++){
                  // if this is not a pseudo coordinate
                  if(*i != Level_Member::PSEUDO_CODE){
                        //include it
                        newcoords.cVect.push_back(*i);
                        newcoords.numCoords++;
                  }//end if
        }//end for
}// end Coordinates::excludePseudoCoords(const Coordinates & coords)\

//---------------------------------- ChunkHeader ----------------------------------------------//

inline void ChunkHeader::excludeNULLRanges(vector<LevelRange>& newvectRange) const
// precondition: newvectRange is an empty vector. (*this).vectRange contains a set of level ranges,
//      which some of them might be NULL (i.e., ranges fro pseudo levels)
// postcondition:
//      newvectRange contains all the ranges of (*this).vectRanges with the same order,
//      without the pseudo ranges. The state of "this" object remains unchanged!
//
{
        //ASSERTION1: newVectRange is empty
        if(!newvectRange.empty())
                throw "ChunkHeader::excludeNULLRanges ==> ASSERTION1 : non-empty vector\n";
        for(vector<LevelRange>::const_iterator rng_i = vectRange.begin(); rng_i != vectRange.end(); rng_i++){
                  if(rng_i->leftEnd != LevelRange::NULL_RANGE && rng_i->rightEnd != LevelRange::NULL_RANGE)
                        newvectRange.push_back(*rng_i);
        }//end for
}// end of ChunkHeader::excludeNULLRanges

//---------------------------------- end of ChunkHeader ---------------------------------------//

//---------------------------------- LevelRange ------------------------------------------------//
LevelRange::LevelRange(const LevelRange& l)
	: dimName(l.dimName),lvlName(l.lvlName),leftEnd(l.leftEnd),rightEnd(l.rightEnd)
{
}
LevelRange const& LevelRange::operator=(LevelRange const& other)
{
	dimName = other.dimName;
	lvlName = other.lvlName;
	leftEnd = other.leftEnd;
	rightEnd = other.rightEnd;
	return (*this);
}

//---------------------------------- end of LevelRange ------------------------------------------------//

const int ChunkID::getChunkDepth() const
{
	if(cid == "root")
		return Chunk::MIN_DEPTH;

	int depth = Chunk::MIN_DEPTH + 1;
	for (int i = 0; i<cid.length(); i++)
	{
		if (cid[i] == '.')
			depth++;
	}
	return depth;
}//end of ChunkID::getChunkDepth()

const int ChunkID::getChunkNumOfDim(bool& isroot) const
// precondition:
//      this->cid, contains a valid chunk id && isroot is an output parameter
// postcondition:
//      case1: the number of dimensions derived from the chunk id is returned, therefore
//      the returned value is >0. Also, isroot is false, meaning that this is not the root chunk.
//      case2: isroot == true (&& returned value == 0), then no valid number of dimensions is returned
//      because this cannot be derived from the chunk id for a root chunk.
//      case3: isroot == false && returned value ==-1, this means that an error has been encountered
//      in the chunk id and the number of dims couldn't be derived.
{
        isroot = false; //flag initialization
	if(cid == "root"){
	        //if this is the root chunk
		isroot = true; //turn on flag		
		return 0; // no way to figure out the no of dims from this chunk id
	}

	int numDims = 1;
        bool first_domain = true;
	string::size_type begin = 0;	
        string::size_type end;
	do{
       	        //get next domain
                end = cid.find(".", begin);

		//get the appropriate substring
		string::size_type end = cid.find(".", begin); // get next "."
		// if end==npos then no "." found, i.e. this is the last domain
		// end-begin == the length of the domain substring => substring cid[begin]...cid[begin+(end-begin)-1]		
		string domain = (end == string::npos) ?
		                        string(cid, begin, cid.length()-begin) : string(cid, begin, end-begin);				
                //now we' ve got a domain
                int tmpNumDims = 1;		
                //search the domain for "|" denoting different dimensions
               	for (int i = 0; i<domain.length(); i++){
        		if (domain[i] == '|')
        			tmpNumDims++; //add one more dim			
        	}//end for
		if(first_domain){
		        //get the number of dims calculated from the 1st domain
		        numDims = tmpNumDims;
		        first_domain = false;
		}
                else //assert that the num of dims derived from the rest of the domains, is the same		
                     //as the one calculated from the 1st domain
                        if(numDims != tmpNumDims) //then some error must exist in the chunk id
                                return -1;		
		begin = end+1;			
	}while(end != string::npos); //while there are still (at least one more) domains
	
	return numDims;
}// end of ChunkID::getChunkNumOfDim

