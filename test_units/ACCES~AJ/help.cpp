#include <algorithm>

#include "help.h"



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
		throw GeneralError("Error inside CellMap::insert : empty chunk id!\n");
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
}//end CellMap::insert

CellMap* CellMap::searchMapForDataPoints(const vector<LevelRange>& qbox, const ChunkID& prefix) const
// precondition:
//      *this is an non-empty CellMap and qbox a non-empty query box and prefix a non-empty Chunk id
// processing:
//      iterate through all data points of Cell Map and check wether they fall into the query box
//postcondition:
//      return pointer to new CellMap with retrieved data points. If no data points found return NULL (i.e.,0)
{
	//assert that CellMap is not empty
	if(empty())
		throw GeneralError("CellMap::searchMapForDataPoints ==> CellMap is empty!\n");

	// if qbox is empty
	if(qbox.empty())
	        throw GeneralError("CellMap::searchMapForDataPoints ==> query box is empty!\n");

	//create new cell map
	CellMap* newmapp = new CellMap;

	//for each chunk id stored in the cell map
	for(vector<ChunkID>::const_iterator id_iter = chunkidVectp->begin(); id_iter != chunkidVectp->end(); id_iter++){
		//get suffix domain
		string suffix = id_iter->get_suffix_domain();
		//turn domain to coordinates
		Coordinates c;
		ChunkID::domain2coords(suffix, c);
		//create corresponding cell
		Cell dataPoint(c, qbox);
		//if (cell is within qbox)
		if(dataPoint.cellWithinBoundaries()){ //then this is a data point of interest
			//create new id: add suffix domain to input prefix
			ChunkID newid(prefix);
       			//and add the new domain as suffix
       			try{
       				newid.addSuffixDomain(suffix);
       			}
         		catch(GeneralError& error){
         			GeneralError e("CellMap::searchMapForDataPoints ==> ");
         			error += e;
                          	throw error;
         		}
			//insert new id into new cell map
                	if(!newmapp->insert(newid.getcid())) {
                	        string msg = string("CellMap::searchMapForDataPoints ==> double entry in cell map: ") + newid.getcid();
                		throw GeneralError(msg.c_str());
                	}// end if
		}//end if
	}//end for

	//if new cellmap is empty: no data points where found within qbox
	if(newmapp->empty()){
		delete newmapp;//free up space
		return 0;
	}//end if
	else{
		return newmapp;//return pointer to new cell map
	}//end else

}// end CellMap::searchMapForDataPoints

const string ChunkID::get_prefix_domain() const
{
	//if chunk id is empty
	if(empty())
		return string(); //return empty string
		
	// Find character "." in cid starting from left
	string::size_type pos = cid.find(".");
	if (pos == string::npos)
	// If the character does not exist then we have a chunk of chunking depth D = Chunk::MIN_DEPTH
	// and the prefix domain is the string itself
		return cid;
	else
	// This is the normal case (D > Chunk::MIN_DEPTH). The prefix domain is the substring before the first "."
		return cid.substr(0, pos);
}//ChunkID::get_prefix_domain()

const string ChunkID::get_suffix_domain() const
{
	//if chunk id is empty
	if(empty())
		return string(); //return empty string
		
	// Find character "." in cid starting from right
	string::size_type pos = cid.rfind(".");
	if (pos == string::npos)
	// If the character does not exist then we have a chunk of chunking depth D = Chunk::MIN_DEPTH
	// and the suffix domain is the string itself
		return cid;
	else
	// The suffix domain is the substring after the last "."
		return cid.substr(pos+1, string::npos);
}//string ChunkID::get_suffix_domain()


void ChunkID::extractCoords(Coordinates& c) const
// precondition:
//	this->cid member contains a valid chunk id
// postcondition:
//	each coordinate from the last domain of the chunk id (member cid) has been stored
//	in the vector of the Coordinates struct in the same order as the interleaving order of the
//	chunk id: major-to-minor from left-to-right.
{
	if(cid == "root")
		throw GeneralError("ChunkID::extractCoords ==> Can't extract coords from \"root\"\n");

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
        	int coord = atoi(coordstr.c_str());
        	c.cVect.push_back(coord);
        	c.numCoords += 1;
        	// read on
        	begin = end + 1;
        	end = lastdom.find("|", begin);
        } //end while
        // repeat once more for the last dimension
       	string coordstr(lastdom, begin, lastdom.length()-begin); // substring lastdom[begin]...lastdom[begin+l-1], l=lastdom.length()-begin
       	int coord = atoi(coordstr.c_str());
       	c.cVect.push_back(coord);
       	c.numCoords += 1;
} // end of ChunkID::extractCoords

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
        return 50;
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
        return 100;
}// end of DataChunk::calculateStgSizeInBytes

ChunkHeader::ChunkHeader(): localDepth(Chunk::NULL_DEPTH), nextLocalDepth(false), artificialHierarchyp(0) {}

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
void Cell::becomeNextCell()
//precondition:
//	*this is a Cell instance contining valid coordinates that fall within the data space
//	defined by the boundaries data member
//processing:
//	finds the rightmost (i.e., less significant) no pseudo coordinate value c where c < MaxBoundaryVal. Then
//	it increases c by one and resets (i.e., sets to the corresponding MinBoundaryVal) all (no pseudo) coordinates
//	to the right of c.If c cannot be found, then all coordinates are reset.
//postcondition:
//	*this is now the "next cell" in the lexicographic order
{
       	if(isCellEmpty())
       		throw GeneralError("Cell::becomeNextCell ==> empty cell\n");

	//assert that current coordinates are within boundaries
	if(!cellWithinBoundaries())
		throw GeneralError("Cell::becomeNextCell() ==> cell out of boundaries\n");
	
	//find righmost coordinate cVect[i] such that cVect[i] < Boundaries[i].rightEnd
	//for each coordinate starting from the right to the left
	bool foundCoord = false;
	int desiredCoordIndex;
	for(int coordIndex = coords.numCoords - 1; coordIndex >= 0; coordIndex--){
		//if coord is not a pseudo
		if(coords.cVect[coordIndex] != LevelMember::PSEUDO_CODE){
			//if coord value precedes its corresponding right boundary
			if(coords.cVect[coordIndex] < boundaries[coordIndex].rightEnd) {
				//then we have found desired coordinate
				foundCoord = true;
				desiredCoordIndex = coordIndex;
				//exit loop
				break;
			}//end if
		}//end if
	}//end for
	
	//if such cell cannot be found
	if(!foundCoord) {
		//reset all coords
		reset();
		//return
		return;
	}//end if
	
	//Else,	increase c by one
	coords.cVect[desiredCoordIndex]++;
	
	//reset all coords to the right of c, if c is not the rightmost coordinate
	if(desiredCoordIndex < coords.numCoords - 1)
		reset(++desiredCoordIndex);	
	return;	
}// end Cell::becomeNextCell()

ostream& operator<<(ostream& stream, const Cell& c)
{
        int numCoords = c.getcoords().numCoords;
        for(int i = 0; i<numCoords; i++) {
                stream<<c.getcoords().cVect[i]<<", ";
        }
        stream << "Boundaries: (";
        for(int i = 0; i<numCoords; i++) {
                if(i == numCoords - 1)
                        stream<<"["<<c.getboundaries()[i].leftEnd<<", "<<c.getboundaries()[i].rightEnd<<"])"<<endl;
                else
                        stream<<"["<<c.getboundaries()[i].leftEnd<<", "<<c.getboundaries()[i].rightEnd<<"], ";
        }

        return stream;
}
