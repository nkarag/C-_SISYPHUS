/***************************************************************************
                          Chunk.C  -  description
                             -------------------
    begin                : Fri Oct 13 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/
#include <fstream>
#include <strstream>
#include <math.h>

#include "definitions.h"
#include "Chunk.h"
#include "AccessManager.h"
#include "Cube.h"


const int LevelRange::NULL_RANGE=-1;
//-------------------------------- ChunkID -----------------------------------
const string ChunkID::get_prefix_domain() const
{
	// Find character "." in cid starting from left
	string::size_type pos = cid.find(".");
	if (pos == string::npos)
	// If the character does not exist then we have a chunk of chunking depth D = 1
	// and the prefix domain is the string itself
		return cid;
	else
	// This is the normal case (D > 1). The prefix domain is the substring before the first "."
		return cid.substr(0, pos);
}

const string ChunkID::get_suffix_domain() const
{
	// Find character "." in cid starting from right
	string::size_type pos = cid.rfind(".");
	if (pos == string::npos)
	// If the character does not exist then we have a chunk of chunking depth D = 1
	// and the suffix domain is the string itself
		return cid;
	else
	// This is the normal case (D > 1). The suffix domain is the substring after the last "."
		return cid.substr(pos+1, string::npos);
}

const int ChunkID::get_chunk_depth() const
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
}

bool operator==(const ChunkID& c1, const ChunkID& c2)
{
	return (c1.cid == c2.cid);
}

unsigned int ChunkID::getPivotLevelPos() const
{
	// by counting the dots we get the position of the levels of the PARENT chunk
	// since the chunk id is made up from the member codes of the parent members!
	int pos = 0;
	for (int i = 0; i<cid.length(); i++)
	{
		if (cid[i] == '.')
			pos++;
	}
	// increase by one to get the desired position
	pos++;
	return pos;
}

string ChunkID::extractMbCode(const unsigned int dim_pos) const
{
/*	#ifdef DEBUGGING
		cerr<<"ChunkID::extractMbCode : chunk id = "<<cid<<endl;
	#endif */

	if(cid == "root")
		return ""; // there is no parent member in the root chunk id
	istrstream input(cid.c_str());
	char c;
	int cnt = 0;
	string result("");
	while(input>>c){
	/*	#ifdef DEBUGGING
			cerr<<"ChunkID::extractMbCode ==> ChunkID::extractMbCode : char = "<<c<<endl;
		#endif*/
         	if(cnt == dim_pos){
			result += c;
			// keep reading digits until you find a '|', or a '.', or an end_of_string			
			while(input>>c){
				if(c == '.') {
               	         		result += c;
               				cnt = 0; //change of domain, so reset counter
               			        break;
				}
				if(c == '|'){
					cnt++; //next dimension inside this domain
					break;						
				}
				result += c;	// got another digit!		
			}
		}
		else if (c == '|') {
			cnt++; //next dimension inside this domain		
		}
		else if (c == '.') {
	         	result += c;
			cnt = 0; //change of domain, so reset counter
		}
	}	
	//#ifdef DEBUGGING
	//	cerr<<"ChunkID::extractMbCode ==> result = "<<result<<endl;
	//#endif
	
	return result;
}

//------------------------------------- end of ChunkID ----------------------------------------


//--------------------------------------- Chunk -----------------------------------------------
void Chunk::createRootChunkHeader(ChunkHeader* rootHdrp, const CubeInfo& info)
{
	rootHdrp->depth = Chunk::MIN_DEPTH;
	rootHdrp->id.setcid(string("root"));
	rootHdrp->numDim = info.get_num_of_dimensions();

	// calculate total number of cells (non-empty + empty) in root chunk
	int number_of_cells = 1; // counter of cells of the root chunk
	vector<Dimension_Level>::iterator iter_lev;
    	for (
		vector<Dimension>::iterator iter_dim = const_cast<vector<Dimension>&>(info.getvectDim()).begin();
		iter_dim != info.getvectDim().end();
		++iter_dim
	)
	{
		iter_lev = (*iter_dim).get_vectLevel().begin(); //pointer to upper level
		number_of_cells *= (*iter_lev).get_num_of_members();
	}
	rootHdrp->totNumCells = number_of_cells;

	// calculate the ranges on each dimension level for the root chunk
	LevelRange rng;
	rng.leftEnd = 0;
    	for (	vector<Dimension>::iterator iter_dim = const_cast<vector<Dimension>&>(info.getvectDim()).begin();
		iter_dim != info.getvectDim().end();
		++iter_dim){
		rng.dimName = (*iter_dim).get_name();
		iter_lev = (*iter_dim).get_vectLevel().begin(); //pointer to upper level
		rng.lvlName = (*iter_lev).get_name();
		rng.rightEnd = (*iter_lev).get_num_of_members() - 1;
		rootHdrp->vectRange.push_back(rng);
	}
}

void Chunk::createChunkHeader(ChunkHeader* hdrp, const CubeInfo& cinfo, const ChunkID& chunkid)
{
       	#ifdef DEBUGGING
       		cerr<<"Chunk::createChunkHeader ==> Creating a Chunk header for chunk: "<<chunkid.getcid()<<endl;
       	#endif

	hdrp->depth = chunkid.get_chunk_depth();
	hdrp->id.setcid(chunkid.getcid());
	hdrp->numDim = cinfo.get_num_of_dimensions();

	// calculate total number of cells (non-empty + empty) in chunk
	int number_of_cells = 1; // counter of cells of the chunk
	Dimension_Level level;
	unsigned int lvlpos;
	int dim_pos = 0;
    	for (
		vector<Dimension>::iterator iter_dim = const_cast<vector<Dimension>&>(cinfo.getvectDim()).begin();
		iter_dim != cinfo.getvectDim().end();
		++iter_dim)
	{
		// find the position in the hierachy of levels, corresponding to the levels of this chunk
		lvlpos = chunkid.getPivotLevelPos();
		//#ifdef DEBUGGING
		//	cerr<<"Chunk::createChunkHeader ==> current pivot level position is : "<<lvlpos<<endl;
		//#endif
		level = (*iter_dim).get_vectLevel()[lvlpos];
		/*if(level.get_name().find("Pseudo-level") != string::npos) {// then this is a pseudo level
			// use the previous (more aggr) level instead
			lvlpos--;
			level = (*iter_dim).get_vectLevel()[lvlpos];
		}*/
		#ifdef DEBUGGING
			cerr<<"Chunk::createChunkHeader ==> current pivot level is : "<<level.get_name()<<endl;
		#endif
		//from the chunk id retrieve the member code for this dimension
		string parentMbrCode = chunkid.extractMbCode(dim_pos); // e.g.0|0.1|0 => 0.1 (for dim at pos 0)
		number_of_cells *= level.get_num_of_sibling_members(parentMbrCode); //e.g. give me how many members are under parent 0.1 in dimension 0
		 ++dim_pos;
		#ifdef DEBUGGING
			cerr<<"Chunk::createChunkHeader ==> Corresponding member code for dimension_pos "<<dim_pos-1<<" = "<<parentMbrCode<<endl;
			cerr<<"Chunk::createChunkHeader ==> Number of children for "<<parentMbrCode<<" : "<<level.get_num_of_sibling_members(parentMbrCode)<<endl;;
		#endif		
	}
	hdrp->totNumCells = number_of_cells;

	// calculate the ranges on each dimension level for the chunk
	LevelRange rng;
	dim_pos=0;
    	for (	vector<Dimension>::iterator iter_dim = const_cast<vector<Dimension>&>(cinfo.getvectDim()).begin();
		iter_dim != cinfo.getvectDim().end();
		++iter_dim){
		rng.dimName = (*iter_dim).get_name();
		// find the position in the hierachy of levels corresponding to the levels of this chunk
		lvlpos = chunkid.getPivotLevelPos();
		level = (*iter_dim).get_vectLevel()[lvlpos];
		rng.lvlName = level.get_name();		
		
		if(rng.lvlName.find("Pseudo-level") != string::npos) {// then this is a pseudo level
			rng.leftEnd = LevelRange::NULL_RANGE;
			rng.rightEnd = LevelRange::NULL_RANGE;
			hdrp->vectRange.push_back(rng);
			++dim_pos;
			continue;
		}
		
		// In order to get the left and right ends, we got to get to the parent member and retrieve
		// the first and last child order codes!
		//from the chunk id retrieve the member code for this dimension
		string parentMbrCode = chunkid.extractMbCode(dim_pos); // e.g.0|0.1|0 => 0.1 (for dim at pos 0)
		lvlpos--; //move one level up (more aggr)
		level = (*iter_dim).get_vectLevel()[lvlpos];
		vector<Level_Member>::const_iterator mbr= level.getMbrByMemberCode(parentMbrCode);
		if(mbr == level.get_vectMember().end()) {// then member does not exist!
			string msg = string("Chunk::createChunkHeader ==> can't find member ") + parentMbrCode + string(" in level ") + level.get_name() + string("\n");
			throw msg.c_str();
		}
		rng.leftEnd = (*mbr).get_first_child_order_code();
		rng.rightEnd = (*mbr).get_last_child_order_code();
		hdrp->vectRange.push_back(rng);
		++dim_pos;
	}
}

CostNode* Chunk::expandChunk(ChunkHeader* chunkHdrp, const CubeInfo& cbinfo, const string& factFile)
{
	CellMap* mapp;
	CostNode* costNd;

	// Get the cube's max depth
	unsigned int maxDepth = cbinfo.getmaxDepth();

	if(chunkHdrp->depth == Chunk::MIN_DEPTH) { // then this is the root chunk
		//create CostNode:
		//scan input file to check for non-empty cells
		string prfx("root");
		try{
			mapp = Chunk::scanFileForPrefix(factFile,prfx);
		}
		catch(const char* msg){
			string m("");//Ex. from Chunk::scanFileForPrefix, in Chunk::expandChunk : ");
			m += msg;
                 	throw m.c_str();
		}
                #ifdef DEBUGGING
                      cerr<<"Chunk::expandChunk ==> Just created the CellMap for the root chunk"<<endl;
                #endif		
		// real number of cells (i.e. non-empty cells)
		chunkHdrp->rlNumCells = chunkHdrp->totNumCells;
		// calculate the size of this chunk
		DirChunk::calculateSize(chunkHdrp);
		//vector<CostNode>* children = new vector<CostNode>;
		//children->reserve(mapp->getchunkidVectp()->size()); //reserve some space for the CostNode children
		//costNd = new CostNode(chunkHdrp, mapp, children);
		costNd = new CostNode(chunkHdrp, mapp);
	   	for (	vector<ChunkID>::iterator iter = const_cast<vector<ChunkID>*>(mapp->getchunkidVectp())->begin();
			iter != mapp->getchunkidVectp()->end();
			++iter	){
                        //create the chunk header of the child chunk
			ChunkHeader* childHdrp = new ChunkHeader;
			try{
				Chunk::createChunkHeader(childHdrp, cbinfo, (*iter));
			}
			catch(const char* msg){
				string m("");//"Ex. from Chunk::createChunkHeader, in Chunk::expandChunk : ");
				m += msg;
        	    		throw m.c_str();
			}
                        #ifdef DEBUGGING
                              cerr<<"Chunk::expandChunk ==> Just created a chunk header for a child of the root chunk"<<endl;
                        #endif		
			
			//hang a CostNode child
			CostNode* child;
			try{
				child = Chunk::expandChunk(childHdrp, cbinfo, factFile);
			}
			catch(const char* message) {
                 		string msg("");
                 		msg += message;
                 		throw msg.c_str();
                 	}

			const_cast<vector<CostNode*>&>(costNd->getchild()).push_back(child);
                        #ifdef DEBUGGING
                              cerr<<"Chunk::expandChunk ==> Just pushed_back child CostNode : "<<costNd->getchild().back()->getchunkHdrp()->id.getcid()<<endl;
                        #endif
                        // Add child to child vector
                        // NOTE: This should work because the CostNode copy constructor has been properly defined!!!
                        //costNd->getchild()->push_back(*child); 		
                        //delete child;
		}
		#ifdef DEBUGGING
			cerr<<"Chunk::expandChunk ==> Testing the CostNode children vector : \n";
			for(vector<CostNode*>::const_iterator i = costNd->getchild().begin();
			    i != costNd->getchild().end();
			    ++i) {
				cerr<<"\t"<<(*i)->getchunkHdrp()->id.getcid()<<endl;
			}
                #endif						
		/*#ifdef DEBUGGING
			cerr<<"Chunk::expandChunk ==> Testing the CostNode children vector : \n";
			for(vector<CostNode>::const_iterator i = costNd->getchild()->begin();
			    i != costNd->getchild()->end();
			    ++i) {
				cerr<<"\t"<<(*i).getchunkHdrp()->id.getcid()<<endl;
			}
                #endif*/				
		return costNd;
	}
	else if(chunkHdrp->depth <= cbinfo.getmaxDepth()) { // then this is a directory chunk
		//create CostNode:
		//scan input file to check for non-empty cells
		try{
			mapp = Chunk::scanFileForPrefix(factFile,chunkHdrp->id.getcid());
		}
		catch(const char* msg){
			string m("");//"Exception from Chunk::scanFileForPrefix, in Chunk::expandChunk : ");
			m += msg;
                        throw m.c_str();
		}
                #ifdef DEBUGGING
                      cerr<<"Chunk::expandChunk ==> Just created the CellMap for a directory chunk"<<endl;
                #endif		
		
		// real number of cells (i.e. non-empty cells)
		chunkHdrp->rlNumCells = chunkHdrp->totNumCells;
		// calculate the size of this chunk
		DirChunk::calculateSize(chunkHdrp);
		//vector<CostNode>* children = new vector<CostNode>;		
		//children->reserve(mapp->getchunkidVectp()->size()); //reserve some space for the CostNode children
		//costNd = new CostNode(chunkHdrp, mapp, children);		
		costNd = new CostNode(chunkHdrp, mapp);		
	        for (	vector<ChunkID>::iterator iter = const_cast<vector<ChunkID>*>(mapp->getchunkidVectp())->begin();
			iter != mapp->getchunkidVectp()->end();
			++iter	){
	                //create the chunk header of the child chunk
	                ChunkHeader* childHdrp = new ChunkHeader;
			try{
				Chunk::createChunkHeader(childHdrp, cbinfo, (*iter));
			}
			catch(const char* msg){
				//string m("");//"Ex. from Chunk::createChunkHeader, in Chunk::expandChunk : ");
				//m += msg;
        	                throw; //m.c_str();
			}
                        #ifdef DEBUGGING
                              cerr<<"Chunk::expandChunk ==> Just created a chunk header for a child of a directory chunk"<<endl;
                        #endif		
			
			//hang a CostNode child
			CostNode* child;
			try{
				child = Chunk::expandChunk(childHdrp, cbinfo, factFile);
			}
			catch(const char* message) {
                 		string msg("");
                 		msg += message;
                 		throw msg.c_str();
                 	}

			const_cast<vector<CostNode*>&>(costNd->getchild()).push_back(child);			
                        #ifdef DEBUGGING
                              cerr<<"Chunk::expandChunk ==> Just pushed_back child CostNode : "<<costNd->getchild().back()->getchunkHdrp()->id.getcid()<<endl;
                        #endif
                        // Add child to child vector
                        // NOTE: This should work because the CostNode copy constructor has been properly defined!!!
                        //costNd->getchild()->push_back(*child); 		
                        //delete child;                        								
		}
		#ifdef DEBUGGING
			cerr<<"Chunk::expandChunk ==> Testing the CostNode children vector : \n";
			for(vector<CostNode*>::const_iterator i = costNd->getchild().begin();
			    i != costNd->getchild().end();
			    ++i) {
				cerr<<"\t"<<(*i)->getchunkHdrp()->id.getcid()<<endl;
			}
                #endif						
		/*#ifdef DEBUGGING
			cerr<<"Chunk::expandChunk ==> Testing the CostNode children vector : \n";
			for(vector<CostNode>::const_iterator i = costNd->getchild()->begin();
			    i != costNd->getchild()->end();
			    ++i) {
				cerr<<"\t"<<(*i).getchunkHdrp()->id.getcid()<<endl;
			}
                #endif*/
		
		return costNd;
	}
	else { // then this is a data chunk (depth == maxDepth + 1)
		if(chunkHdrp->depth != (cbinfo.getmaxDepth()+ 1))
			throw "Chunk::expandChunk ==> error in depth member, inside Chunkheader\n";

		//create CostNode:
		//scan input file to check for non-empty cells
		try{
			mapp = Chunk::scanFileForPrefix(factFile,chunkHdrp->id.getcid(), true);
		}
		catch(const char* msg){
			string m("");//"Ex. from Chunk::scanFileForPrefix, in Chunk::expandChunk : ");
			m += msg;
                        throw m.c_str();
		}
                #ifdef DEBUGGING
                      cerr<<"Chunk::expandChunk ==> Just created the CellMap for a data chunk"<<endl;
                #endif				
		// real number of cells (i.e. non-empty cells)
		chunkHdrp->rlNumCells = mapp->getchunkidVectp()->size();
		// calculate the size of this chunk
		DataChunk::calculateSize(chunkHdrp, cbinfo.getnumFacts());
		costNd = new CostNode(chunkHdrp, mapp);
		//costNd = new CostNode(chunkHdrp, mapp, 0);
		return costNd;
	}
} //end Chunk::expandChunk

CellMap* Chunk::scanFileForPrefix(const string& factFile,const string& prefix, bool isDataChunk = false)
{
	// open input file for reading
	ifstream input(factFile.c_str());
	if(!input)
		throw "Chunk::scanFileForPrefix ==> Error in creating ifstream obj, in Chunk::scanFileForPrefix\n";
		
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
				throw "Chunk::scanFileForPrefix ==> ChunkID syntax error: no \".\" in id\n";
			}
			string child_chunk_id(buffer,0,pos);
                        //#ifdef DEBUGGING
                        //      cerr<<"Chunk::scanFileForPrefix ==> child_chunk_id = "<<child_chunk_id<<endl;
                        //#endif		         				
			mapp->insert(child_chunk_id);
		}
		else {
			string::size_type pos = buffer.find(prefix);
			if(pos==0) { // then we got a *prefix* match (i.e. a match at the beginning)			
        			//get the next domain
        			// find pos of first character not of prefix
        			//string::size_type pos = buffer.find_first_not_of(prefix); // get the position after the prefix (this should be a ".")
        			pos = prefix.length(); // this must point to a "."
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
        				throw msg.c_str();
        			}
        		}// end if
		} // end else
		// read on until the '\n' in order to skip the fact values
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

//--------------------- end of Chunk -----------------------//

//--------------------- DirChunk ---------------------------//
unsigned int DirChunk::calcCellOffset(const Coordinates& coords){
	return 0;
}

const DirEntry& DirChunk::readCellEntry(unsigned int offset) const {
	return entry[offset];
}

void DirChunk::updateCellEntry(unsigned int offset, const DirEntry& ent){
	entry[offset] = ent;
}

const DirCell DirChunk::getCell(unsigned int offset) const
{
}

void DirChunk::calculateSize(ChunkHeader* hdrp)
{
 	// size = totNumCells * sizeof(DirEntry)
	hdrp->size = hdrp->totNumCells * sizeof(DirEntry);
}

//--------------------- end of DirChunk ---------------------------//

//--------------------- DataChunk ---------------------------//
unsigned int DataChunk::calcCellOffset(const Coordinates& coords){
	return 0;
}

const DataEntry& DataChunk::readCellEntry(unsigned int offset) const
{
	return entry[offset];
}

void DataChunk::updateCellEntry(unsigned int offset, const DataEntry& ent){
	entry[offset] = ent;
}

const DataCell DataChunk::getCell(unsigned int offset) const
{

}

void DataChunk::calculateSize(ChunkHeader* hdrp, unsigned int numfacts)
{	
	int entry_size = numfacts * sizeof(float);
	if(float(hdrp->rlNumCells)/float(hdrp->totNumCells) > SPARSENESS_THRSHLD){
		// no need for compression : i.e. size = totNumCells * entry_size
		hdrp->size = hdrp->totNumCells * entry_size;
	}
	else {// need for compression
                // size = bitmap-size + rlNumCells * entry_size
        	hdrp->size = (int)ceil(hdrp->totNumCells/8.0) + (hdrp->rlNumCells * entry_size);	
	}
}
//--------------------- end of DataChunk ---------------------------//
//--------------------- LevelRange ---------------------------------//
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
//--------------------- end of LevelRange --------------------------//