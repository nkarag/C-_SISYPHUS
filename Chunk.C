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
#include "AccessManagerImpl.h"
#include "Cube.h"
#include "DiskStructures.h"
#include "Exceptions.h"


//-------------------------------- ChunkID -----------------------------------
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

const int ChunkID::getChunkGlobalDepth(int localdepth) const
{
	if(cid.empty())
		return -1;

	if(cid == "root")
		return Chunk::MIN_DEPTH;

	int depth = Chunk::MIN_DEPTH + 1;
	for (int i = 0; i<cid.length(); i++)
	{
		if (cid[i] == '.')
			depth++;
	}
	depth = (localdepth == Chunk::NULL_DEPTH)? depth : depth - localdepth;
	
	if(localdepth != Chunk::NULL_DEPTH && depth < 1)
	        return -1; //an invalid local depth must have been given
	
	return depth;
}//end of ChunkID::getChunkGlobalDepth()

const int ChunkID::getNumDomains() const
{
	if(cid.empty())
		return -1;
		
	if(cid == "root")
		return 0;		

	int nodoms = 1;
	for (int i = 0; i<cid.length(); i++)
	{
		if (cid[i] == '.')
			nodoms++;
	}
	return nodoms;
}//end of ChunkID::getNumDomains()

void ChunkID::coords2domain(const Coordinates& coords, string& domain){
        if(coords.empty()){
                domain = string("");
                return;
        }

        //create an output string stream
        ostrstream dom;
        //for each coordinate
        for(int cindex = 0; cindex < coords.numCoords; cindex++){
                //if this is not the last coordinate
                if(cindex < coords.numCoords - 1)
                        dom<<coords.cVect[cindex]<<"|";
                else
                        dom<<coords.cVect[cindex];
        }//end for
        dom<<ends;
        domain = string(dom.str());
}//ChunkID::coords2domain


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
                end = cid.find(".", begin); // get next "."
		//get the appropriate substring		
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

/*
bool operator==(const ChunkID& c1, const ChunkID& c2)
{
	return (c1.cid == c2.cid);
}

bool operator<(const ChunkID& c1, const ChunkID& c2)
{
	return (c1.cid < c2.cid);
}
*/
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
} // end of ChunkID::extractMbCode

void ChunkID::extractCoords(Coordinates& c) const
// precondition:
//	this->cid member contains a valid chunk id
// postcondition:
//	each coordinate from the last domain of the chunk id (member cid) has been stored
//	in the vector of the Coordinates struct in the same order as the interleaving order of the
//	chunk id: major-to-minor from left-to-right.
{
	if(cid == "root")
		throw GeneralError(__FILE__, __LINE__, "ChunkID::extractCoords ==> Can't extract coords from \"root\"\n");
		
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


//------------------------------------- end of ChunkID ----------------------------------------


//--------------------------------------- Chunk -----------------------------------------------
Chunk& Chunk::operator=(const Chunk& other)
{
	//call the ChunkHeader operator=()
	hdr = other.gethdr();
}//Chunk::operator=()

void Chunk::createRootChunkHeader(ChunkHeader* rootHdrp, const CubeInfo& info)
//precondition:
//	cinfo contains the following valid information:
//		- num_of_dimensions, vectDim
//	that will be used in this procedure.

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
    	for (vector<Dimension>::iterator iter_dim = const_cast<vector<Dimension>&>(info.getvectDim()).begin();
			iter_dim != info.getvectDim().end(); ++iter_dim){
		LevelRange rng;
		rng.leftEnd = 0;
		rng.dimName = (*iter_dim).get_name();
		iter_lev = (*iter_dim).get_vectLevel().begin(); //pointer to upper level
		rng.lvlName = (*iter_lev).get_name();
		rng.rightEnd = (*iter_lev).get_num_of_members() - 1;
		rootHdrp->vectRange.push_back(rng);
	}
}//end of Chunk::createRootChunkHeader

void Chunk::createChunkHeader(ChunkHeader* hdrp, const CubeInfo& cinfo, const ChunkID& chunkid)
{
       	#ifdef DEBUGGING
       		cerr<<"Chunk::createChunkHeader ==> Creating a Chunk header for chunk: "<<chunkid.getcid()<<endl;
       	#endif

	hdrp->depth = chunkid.getChunkGlobalDepth();
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
		vector<LevelMember>::const_iterator mbr= level.getMbrByMemberCode(parentMbrCode);
		if(mbr == level.get_vectMember().end()) {// then member does not exist!
			string msg = string("Chunk::createChunkHeader ==> can't find member ") + parentMbrCode + string(" in level ") + level.get_name() + string("\n");
			throw GeneralError(__FILE__, __LINE__, msg.c_str());
		}
		rng.leftEnd = (*mbr).get_first_child_order_code();
		rng.rightEnd = (*mbr).get_last_child_order_code();
		hdrp->vectRange.push_back(rng);
		++dim_pos;
	}//end for
}//end of Chunk::createChunkHeader

CostNode* Chunk::createCostTree(ChunkHeader* chunkHdrp, const CubeInfo& cbinfo, const string& factFile)
// precondition:
//	cinfo contains the following valid inforamtion:
//		- maxDepth, numFacts, num_of_dimensions, vectDim
//	that will be used in this procedure. Also chunkHdrp points at a ChunkHeader where
// 	the following info is valid and can be used:
//		- depth, numDim, id, totNumCells, vectRange
{
	CellMap* mapp = 0 ;
	CostNode* costNd = 0;

	// Get the cube's max depth
	unsigned int maxDepth = cbinfo.getmaxDepth();

	//if(chunkHdrp->depth == Chunk::MIN_DEPTH)
	if(AccessManager::isRootChunk(chunkHdrp->depth, chunkHdrp->localDepth, chunkHdrp->nextLocalDepth, maxDepth)){ // then this is the root chunk
		//create CostNode:
		//scan input file to check for non-empty cells
		string prfx("root");
		try{
			mapp = Chunk::scanFileForPrefix(factFile,prfx);
		}
		catch(GeneralError& error){
			GeneralError e("Chunk::createCostTree ==> ");//Ex. from Chunk::scanFileForPrefix, in Chunk::createCostTree : ");
			error += e;
			//if(mapp) delete mapp; note: cant be != 0 here
                 	throw error;
		}
                #ifdef DEBUGGING
                      cerr<<"Chunk::createCostTree ==> Just created the CellMap for the root chunk"<<endl;
                #endif		
		// real number of cells (i.e. non-empty cells)
		//chunkHdrp->rlNumCells = chunkHdrp->totNumCells;
		chunkHdrp->rlNumCells = mapp->getchunkidVectp()->size();
		
		// calculate the size of this chunk
		try{
        		chunkHdrp->size = DirChunk::calculateStgSizeInBytes(chunkHdrp->depth,
        							  maxDepth,
        							  chunkHdrp->numDim,
        							  chunkHdrp->totNumCells);
		}
		catch(GeneralError& error){
			GeneralError e("Chunk::createCostTree ==> ");
			error += e;
			delete mapp;
                 	throw error;
		}
		catch(...){
			delete mapp;
                 	throw;		
		}							
		//DirChunk::calculateSize(chunkHdrp);
		
		//vector<CostNode>* children = new vector<CostNode>;
		//children->reserve(mapp->getchunkidVectp()->size()); //reserve some space for the CostNode children
		//costNd = new CostNode(chunkHdrp, mapp, children);
		
		costNd = new CostNode(chunkHdrp, mapp);
		
		delete mapp; //to free memory
		mapp = 0;
		// NOTE: chunkHdrp will be deleted by the caller who "new-ed" it
		
	   	for (	vector<ChunkID>::iterator iter = const_cast<vector<ChunkID>*>(mapp->getchunkidVectp())->begin();
			iter != mapp->getchunkidVectp()->end();
			++iter	){

                        //create the chunk header of the child chunk
			ChunkHeader* childHdrp = new ChunkHeader;
			
			try{
				Chunk::createChunkHeader(childHdrp, cbinfo, (*iter));
			}
			catch(GeneralError& error){
                		GeneralError e("Chunk::createCostTree ==> ");
                		error += e;
                		delete childHdrp; //free up memory
                		delete costNd;
                         	throw error;
			}
			catch(...){
                		delete childHdrp; //free up memory
                		delete costNd;
                         	throw;			
			}
                        #ifdef DEBUGGING
                              cerr<<"Chunk::createCostTree ==> Just created a chunk header for a child of the root chunk"<<endl;
                        #endif		
			
			//hang a CostNode child
			CostNode* child = 0;
			try{
				child = Chunk::createCostTree(childHdrp, cbinfo, factFile);
			}
			catch(GeneralError& error) {
                 		GeneralError e("Chunk::createCostTree ==> ");
                 		error += e;
                 		//if(child) delete child; child wont have a chance to be != 0
                		delete childHdrp; //free up memory
                		delete costNd;                 		
                 		throw error;
                 	}
                 	catch(...){
                		delete childHdrp; //free up memory
                		delete costNd;                 		
                 		throw;                 	
                 	}
                 	
                 	delete childHdrp; //free up memory
                 	childHdrp = 0;

			const_cast<vector<CostNode*>&>(costNd->getchild()).push_back(child);
			child = 0; // no need to point at this CostNode any more
			
                        #ifdef DEBUGGING
                              cerr<<"Chunk::createCostTree ==> Just pushed_back child CostNode : "<<costNd->getchild().back()->getchunkHdrp()->id.getcid()<<endl;
                        #endif
                        // Add child to child vector
                        // NOTE: This should work because the CostNode copy constructor has been properly defined!!!
                        //costNd->getchild()->push_back(*child); 		
                        //delete child;
		}
		#ifdef DEBUGGING
			cerr<<"Chunk::createCostTree ==> Testing the CostNode children vector : \n";
			for(vector<CostNode*>::const_iterator i = costNd->getchild().begin();
			    i != costNd->getchild().end();
			    ++i) {
				cerr<<"\t"<<(*i)->getchunkHdrp()->id.getcid()<<endl;
			}
                #endif						
		/*#ifdef DEBUGGING
			cerr<<"Chunk::createCostTree ==> Testing the CostNode children vector : \n";
			for(vector<CostNode>::const_iterator i = costNd->getchild()->begin();
			    i != costNd->getchild()->end();
			    ++i) {
				cerr<<"\t"<<(*i).getchunkHdrp()->id.getcid()<<endl;
			}
                #endif*/				
		return costNd;
	}
	//else if(chunkHdrp->depth < cbinfo.getmaxDepth()) {
	else if(AccessManager::isDirChunk(chunkHdrp->depth, chunkHdrp->localDepth, chunkHdrp->nextLocalDepth, maxDepth)){// then this is a directory chunk
		//create CostNode:
		//scan input file to check for non-empty cells
		try{
			mapp = Chunk::scanFileForPrefix(factFile,chunkHdrp->id.getcid());
		}
		catch(GeneralError& error){
			GeneralError e("Chunk::createCostTree ==> ");//"Exception from Chunk::scanFileForPrefix, in Chunk::createCostTree : ");
			error += e;
			//if(mapp) delete mapp; note: cant be != 0 here
                        throw error;
		}
                #ifdef DEBUGGING
                      cerr<<"Chunk::createCostTree ==> Just created the CellMap for a directory chunk"<<endl;
                #endif		
		
		// real number of cells (i.e. non-empty cells)
		//chunkHdrp->rlNumCells = chunkHdrp->totNumCells;
		chunkHdrp->rlNumCells = mapp->getchunkidVectp()->size();
		
		// calculate the size of this chunk
		try{
        		chunkHdrp->size = DirChunk::calculateStgSizeInBytes(chunkHdrp->depth,
        							  maxDepth,
        							  chunkHdrp->numDim,
        							  chunkHdrp->totNumCells);
		}
		catch(GeneralError& error){
			GeneralError e("Chunk::createCostTree ==> ");
			error += e;
			delete mapp;
                 	throw error;
		}
		catch(...){
			delete mapp;
                 	throw;		
		}
		//DirChunk::calculateSize(chunkHdrp);
		
		//vector<CostNode>* children = new vector<CostNode>;		
		//children->reserve(mapp->getchunkidVectp()->size()); //reserve some space for the CostNode children
		//costNd = new CostNode(chunkHdrp, mapp, children);		
		
		costNd = new CostNode(chunkHdrp, mapp);
		
		delete mapp; //to free memory
		mapp = 0;
		// NOTE: chunkHdrp will be deleted by the caller who "new-ed" it
				
	        for (	vector<ChunkID>::iterator iter = const_cast<vector<ChunkID>*>(mapp->getchunkidVectp())->begin();
			iter != mapp->getchunkidVectp()->end();
			++iter	){
			
	                //create the chunk header of the child chunk
	                ChunkHeader* childHdrp = new ChunkHeader;
	
			try{
				Chunk::createChunkHeader(childHdrp, cbinfo, (*iter));
			}
			catch(GeneralError& error){
				GeneralError e("Chunk::createCostTree ==> ");
				error += e;
				delete costNd;
				delete childHdrp;
        	                throw error; //m.c_str();
			}
			catch(...){
				delete costNd;
				delete childHdrp;
        	                throw;
			}
                        #ifdef DEBUGGING
                              cerr<<"Chunk::createCostTree ==> Just created a chunk header for a child of a directory chunk"<<endl;
                        #endif		
			
			//hang a CostNode child
			CostNode* child = 0;
			try{
				child = Chunk::createCostTree(childHdrp, cbinfo, factFile);
			}
			catch(GeneralError& error) {
                 		GeneralError e("Chunk::createCostTree ==> ");
                 		error += e;
				delete costNd;
				delete childHdrp;
				//if(child) delete  child; note: child cant be != 0 here
                 		throw error;
                 	}
                 	catch(...){
				delete costNd;
				delete childHdrp;
				//if(child) delete  child; note: child cant be != 0 here
                 		throw;                 	
                 	}

                 	delete childHdrp; //free up memory
                 	childHdrp = 0;
                 	
			const_cast<vector<CostNode*>&>(costNd->getchild()).push_back(child);			
			child = 0; // no need to point at the CostNode anymore
			
                        #ifdef DEBUGGING
                              cerr<<"Chunk::createCostTree ==> Just pushed_back child CostNode : "<<costNd->getchild().back()->getchunkHdrp()->id.getcid()<<endl;
                        #endif
                        // Add child to child vector
                        // NOTE: This should work because the CostNode copy constructor has been properly defined!!!
                        //costNd->getchild()->push_back(*child); 		
                        //delete child;                        								
		}
		#ifdef DEBUGGING
			cerr<<"Chunk::createCostTree ==> Testing the CostNode children vector : \n";
			for(vector<CostNode*>::const_iterator i = costNd->getchild().begin();
			    i != costNd->getchild().end();
			    ++i) {
				cerr<<"\t"<<(*i)->getchunkHdrp()->id.getcid()<<endl;
			}
                #endif						
		/*#ifdef DEBUGGING
			cerr<<"Chunk::createCostTree ==> Testing the CostNode children vector : \n";
			for(vector<CostNode>::const_iterator i = costNd->getchild()->begin();
			    i != costNd->getchild()->end();
			    ++i) {
				cerr<<"\t"<<(*i).getchunkHdrp()->id.getcid()<<endl;
			}
                #endif*/
		
		return costNd;
	}
	else { // then this is a data chunk (depth == maxDepth)
		//if(chunkHdrp->depth != cbinfo.getmaxDepth())
		if(!AccessManager::isDataChunk(chunkHdrp->depth, chunkHdrp->localDepth, chunkHdrp->nextLocalDepth, maxDepth))
			throw GeneralError(__FILE__, __LINE__, "Chunk::createCostTree ==> error in chunk type: Data Chunk expected!\n");

		//create CostNode:
		//scan input file to check for non-empty cells
		try{
			mapp = Chunk::scanFileForPrefix(factFile,chunkHdrp->id.getcid(),true);
		}
		catch(GeneralError& error){
			GeneralError e("Chunk::createCostTree ==> ");//"Ex. from Chunk::scanFileForPrefix, in Chunk::createCostTree : ");
			error += e;
			//if(mapp) delete; note: cant be != 0 here
                        throw error;
		}
                #ifdef DEBUGGING
                      cerr<<"Chunk::createCostTree ==> Just created the CellMap for a data chunk"<<endl;
                #endif				
		// real number of cells (i.e. non-empty cells)
		chunkHdrp->rlNumCells = mapp->getchunkidVectp()->size();
		
		// calculate the size of this chunk
		try{
        		chunkHdrp->size = DataChunk::calculateStgSizeInBytes(chunkHdrp->depth,
        							  maxDepth,
        							  chunkHdrp->numDim,
        							  chunkHdrp->totNumCells,
        							  chunkHdrp->rlNumCells,
        							  cbinfo.getnumFacts());
		}
		catch(GeneralError& error){
			GeneralError e("Chunk::createCostTree ==> ");
			error += e;
			delete mapp;
                 	throw error;
		}
		catch(...){
			delete mapp;
                 	throw;		
		}							
		//DataChunk::calculateSize(chunkHdrp, cbinfo.getnumFacts());		

       		// **************************** NOTE ************************************
       		// * 	In order to save memory space: WE DONT HAVE TO KEEP THE CellMap OF
       		// * 	A DATA CHUNK because we dont need it later, since we will read the
       		// *	input fact file again in order to retrieve the measure values.
       		// *	We will keep it only for large data chunks because we will need it
       		// **********************************************************************
       		
       		//if this is a large data chunk
		if( AccessManager::isLargeChunk(chunkHdrp->size) ){
			//keep map
			costNd = new CostNode(chunkHdrp, mapp);
		}//end if
		else { // this is not a large data chunk
        		//delete mapp; //free space				
        		//costNd = new CostNode(chunkHdrp, mapp); //old code!
        		
        		//dont keep map to save space
        		costNd = new CostNode(chunkHdrp);
        		//costNd->setchunkHdrp(chunkHdrp);        				
		}//end else
		
		delete mapp;
		mapp = 0;
		// NOTE: chunkHdrp will be deleted by the caller who "new-ed" it
		
		return costNd;
	}
} //end Chunk::createCostTree

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
		throw GeneralError(__FILE__, __LINE__, "Chunk::scanFileForPrefix ==> Error in creating ifstream obj, in Chunk::scanFileForPrefix\n");
		
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
				delete mapp;
				throw GeneralError(__FILE__, __LINE__, "Chunk::scanFileForPrefix ==> Assertion 1: ChunkID syntax error: no \".\" in id in fact load file\n");
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
        				delete mapp;
                                        throw GeneralError(__FILE__, __LINE__, "Chunk::scanFileForPrefix ==> Assertion2:  ChunkID syntax error: no \".\" (after input prefix) in id in fact load file\n");
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
       					delete mapp;
       					throw GeneralError(__FILE__, __LINE__, msg.c_str());
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

//--------------------- end of Chunk -----------------------//

//--------------------- DirChunk ---------------------------//
unsigned int DirChunk::calcCellOffset(const Coordinates& coords, const vector<LevelRange>& vectRange)//const ChunkHeader& hdr)
// precondition:
//	coords contain a valid set of coordinates in a dir chunk. The order of the coords in the
//	vector member from left to right corresponds to major-to-minor positioning of the cells.
//	vectRange cooressponds to the vector of order code ranges of the dir chunk
//	hdr corresponds to the chunk header of the dir chunk in question.
// postcondition:
//	the corresponding cell offset is returned.  This offset is a value in the range [0..totNumCells-1] and
//	can be used as an index value for access of the cell in an array (or vector) of the form:
//      array[totNumCells].
{
	//ASSERTION1: not an empty vector
	if(coords.cVect.empty())
		throw GeneralError(__FILE__, __LINE__, "DirChunk::calcCellOffset ==> ASSERTION1: empty coord vector\n");

        //ASSERTION2: if there are pseudo levels, then the NULL ranges must correspond to the pseudo coordinates
        for(int c = 0; c<coords.numCoords; c++){
                  // if this is a pseudo coordinate
                  if(coords.cVect[c] == LevelMember::PSEUDO_CODE){
                        //then the corresponding range should be NULL
                        if(vectRange[c].leftEnd != LevelRange::NULL_RANGE ||
                                                vectRange[c].rightEnd != LevelRange::NULL_RANGE)
                                throw GeneralError(__FILE__, __LINE__, "DirChunk::calcCellOffset ==> ASSERTION2: mismatch in position of pseudo levels\n");
                  }//end if
        }//end for

        //first remove Pseudo-code coordinate values
        Coordinates newcoords;
        coords.excludePseudoCoords(newcoords);

        //Also remove null level-ranges
        vector<LevelRange> newvectRange;
        ChunkHeader::excludeNULLRanges(vectRange, newvectRange);

        //ASSERTION3: same number of pseudo levels found
        if(newvectRange.size() != newcoords.numCoords || newvectRange.size() != newcoords.cVect.size())
                throw GeneralError(__FILE__, __LINE__, "DirChunk::calcCellOffset ==> ASSERTION3: mismatch in no of pseudo levels\n");

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
		//ASSERTION4: coordinate is in range
		if(newcoords.cVect[d] < newvectRange[d].leftEnd ||
							newcoords.cVect[d] > newvectRange[d].rightEnd)
			throw GeneralError(__FILE__, __LINE__, "DirChunk::calcCellOffset ==> ASSERTION4: coordinate out of level range\n");
		//compute product of cardinalities
		// init product total
		unsigned int prod = newcoords.cVect[d] - newvectRange[d].leftEnd; // normalize coordinate to 0 origin
		//for each dimension (except for d) get cardinality and multiply total product
		for(int dd = d+1; dd<newcoords.numCoords; dd++){
			//ASSERTION5: proper level ranges
			if(newvectRange[dd].rightEnd < newvectRange[dd].leftEnd)
				throw GeneralError(__FILE__, __LINE__, "DirChunk::calcCellOffset ==> ASSERTION5: invalid level range\n");
			//ASSERTION6: not null range
			if(newvectRange[dd].leftEnd == LevelRange::NULL_RANGE ||
                                                newvectRange[dd].rightEnd == LevelRange::NULL_RANGE)
				throw GeneralError(__FILE__, __LINE__, "DirChunk::calcCellOffset ==> ASSERTION6: NULL level range\n");
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

/*
const DirEntry& DirChunk::readCellEntry(unsigned int offset) const {
	return entry[offset];
}

void DirChunk::updateCellEntry(unsigned int offset, const DirEntry& ent){
	entry[offset] = ent;
}

const DirCell DirChunk::getCell(unsigned int offset) const
{
}
*/

size_t DirChunk::calculateStgSizeInBytes(int depth, unsigned int maxDepth,
					unsigned int numDim, unsigned int totNumCells,
					int local_depth, bool nextLDflag,
					const unsigned int* const noMembers)
//precondition:
//	depth is the depth of the DirChunk we wish to calculate its storage size.
//	This DirChunk can be also the root chunk (depth==Chunk::MIN_DEPTH), or even an artificially chunked dir chunk.
//	 numDim is the number of dimensions of the cube and totNumCells is the total number of entries including
//	empty entries (i.e.,cells). Local depth, nextLDFlag and noMembers pertains to artificially chunked dir chunks.
//	If noMembers != 0 (i.e., for an artificial chunk) then its size should be equal with numDim
//postcondition:
//	the size in bytes consumed by the corresponding DiskDirChunk structure is returned.
{
	//ASSERTION 1: assert that this is a valid depth
	//if(depth > maxDepth)
	//	throw GeneralError(__FILE__, __LINE__, "DirChunk::calculateStgSizeInBytes ==> Invalid depth\n");
	
	//ASSERTION: this is a directory chunk
	if(!AccessManagerImpl::isDirChunk(depth, local_depth, nextLDflag, maxDepth))
		throw GeneralError(__FILE__, __LINE__, "DirChunk::calculateStgSizeInBytes ==> Wrong chunk type\n");

	//Is this the root chunk?
	bool isRootChunk = AccessManagerImpl::isRootChunk(depth, local_depth, nextLDflag, maxDepth);

	//Is this an artificially chunked dir chunk?
	bool isArtifChunk = AccessManagerImpl::isArtificialChunk(local_depth);	
							
	// first add the size of the static parts
	size_t size = sizeof(DiskDirChunk);
		
	//Now, the size of the dynamic parts:	

	//if this is not the root chunk, then count also the chunk id
	if(!isRootChunk){	
        	//1. the chunk id (in DiskChunkHeader)
                //first find the number of domains in the chunk id
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
        }//end if

	//2. the order-code ranges (in DiskChunkHeader)
	size += numDim * sizeof(DiskChunkHeader::OrderCodeRng_t);
	
	//3. The number of dir entries (in DiskDirChunk)
	size += totNumCells * sizeof(DiskDirChunk::DirEntry_t);		
	
	//4. If this is an artificially chunked dir chunk
	if(isArtifChunk){
		// Include the size of the range to order-code mappings
		
		// 1st the size of the range pointers
		size += numDim * sizeof(DiskDirChunk::Rng2oc_t);
		
		// and then the size of the range elements
		for(int i = 0; i<numDim; i++){
			size += noMembers[i] * sizeof(DiskDirChunk::Rng2ocElem_t);		
		}//end for
	}//end if
	
	return size;
}//end of DirChunk::calculateStgSizeInBytes

/*void DirChunk::calculateSize(ChunkHeader* hdrp)
{ 	
	// first add the size of the static parts
	hdrp->size = sizeof(DiskDirChunk);
	
	//Now, the size of the dynamic parts:
	//1. the chunk id (in DiskChunkHeader)
	hdrp->size += hdrp->depth * (sizeof(DiskChunkHeader::Domain_t) + hdrp->numDim * sizeof(DiskChunkHeader::ordercode_t));
	//hdrp->size += hdrp->depth * hdrp->numDim * sizeof(DiskChunkHeader::ordercode_t);

	//2. the order-code ranges (in DiskChunkHeader)
	hdrp->size += hdrp->numDim * sizeof(DiskChunkHeader::OrderCodeRng_t);
	
	//3. The number of dir entries (in DiskDirChunk)
	hdrp->size += hdrp->totNumCells * sizeof(DiskDirChunk::DirEntry_t);		
}*/

/*void DirChunk::calculateSize(ChunkHeader* hdrp)
{
 	// size = totNumCells * sizeof(DirEntry)
	hdrp->size = hdrp->totNumCells * sizeof(DirEntry);
}*/

//--------------------- end of DirChunk ---------------------------//

//--------------------- DataChunk ---------------------------//
unsigned int DataChunk::calcCellOffset(const Coordinates& coords, const deque<bool>& bmp,
					const vector<LevelRange>& vectRange, bool& isEmpty)
// precondition:
//	coords contain a valid set of coordinates in a data chunk. The order of the coords in the
//	vector member from left to right corresponds to major-to-minor positioning of the cells. Only the non-empty
//	cells have been allocated and we use the bitmap bmp to locate empty cells. vectRange corresponds to the
//	vector of order code ranges of the data chunk in question.
// postcondition:
//	the corresponding cell offset is returned. This offset is a value in the range [0..realNumCells-1] and
//	can be used as an index value for access of the cell in an array (or vector) of the form: array[realNumCells]
//      The flag isEmpty is set to true if the requested cell corresponds to a 0 bit in the bitmap.
{
	//ASSERTION1: not an empty vector
	if(coords.cVect.empty())
		throw GeneralError(__FILE__, __LINE__, "DataChunk::(static)calcCellOffset ==> ASSERTION1: empty coord vector\n");

	//ASSERTION2: not an empty vector
	if(bmp.empty())
		throw GeneralError(__FILE__, __LINE__, "DataChunk::(static)calcCellOffset ==> ASSERTION2: empty bitmap\n");

        //ASSERTION3: this is a data chunk therefore no pseudo coords must exist
        for(int c = 0; c<coords.numCoords; c++){
                  // if this is a pseudo coordinate
                  if(coords.cVect[c] == LevelMember::PSEUDO_CODE){
                        //then the corresponding range should be NULL
                                throw GeneralError(__FILE__, __LINE__, "DataChunk::calcCellOffset ==> ASSERTION3: pseudo coord in data chunk coordinates!\n");
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
	        offset = DirChunk::calcCellOffset(coords, vectRange);
        }
        catch(GeneralError& error){
		GeneralError e("DataChunk::calcCellOffset ==> ");//"Exception from Chunk::scanFileForPrefix, in Chunk::createCostTree : ");
		error += e;
                throw error;
        }

	// set empty-cell flag
	(!bmp[offset]) ? isEmpty = true : isEmpty = false;

	//ASSERTION4: proper offset in bitmap
	if(offset > bmp.size())
		throw GeneralError(__FILE__, __LINE__, "DataChunk::(static)calcCellOffset ==> ASSERTION4: wrong offset in bitmap\n");

	//calculate number of zeros
	int numZeros = 0;
	for(int bit=0; bit<offset; bit++){
		// if current bit is zero add one to the zero counter
		if (bmp[bit] == false)
		        numZeros++;
	}//end for

	return (offset - numZeros);
}// end of DataChunk::calcCellOffset(const Coordinates& coords, const deque<bool>& bmp)

/*
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
*/
size_t DataChunk::calculateStgSizeInBytes(int depth, unsigned int maxDepth, unsigned int numDim,
				unsigned int totNumCells, unsigned int rlNumCells, unsigned int numfacts, int local_depth,
				bool nextLDflag)
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
//	if(depth != maxDepth)
//		throw GeneralError(__FILE__, __LINE__, "DataChunk::calculateStgSizeInBytes ==> Invalid depth\n");

	//ASSERTION: this is a data chunk
	if(!AccessManagerImpl::isDataChunk(depth, local_depth, nextLDflag, maxDepth))
		throw GeneralError(__FILE__, __LINE__, "DataChunk::calculateStgSizeInBytes ==> Wrong chunk type\n");


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

/*
void DataChunk::calculateSize(ChunkHeader* hdrp, unsigned int numfacts)
// precondition:
// hdrp points at a ChunkHeader structure. NOTE: it is assumed that
{
	// first add the size of the static parts
	hdrp->size = sizeof(DiskDataChunk);
	
	//Now, the size of the dynamic parts:
	//1. the chunk id (in DiskChunkHeader)
	hdrp->size += hdrp->depth * (sizeof(DiskChunkHeader::Domain_t) + hdrp->numDim * sizeof(DiskChunkHeader::ordercode_t));
	//hdrp->size += hdrp->depth * hdrp->numDim * sizeof(DiskChunkHeader::ordercode_t);

	//2. the order-code ranges (in DiskChunkHeader)
	hdrp->size += hdrp->numDim * sizeof(DiskChunkHeader::OrderCodeRng_t);
	
	//3. The number of dir entries (in DiskDataChunk)
	size_t entry_size = sizeof(DiskDataChunk::DataEntry_t) + (numfacts * sizeof(measure_t));
	if(float(hdrp->rlNumCells)/float(hdrp->totNumCells) > SPARSENESS_THRSHLD){
		// no need for compression : i.e. size = totNumCells * entry_size
		hdrp->size += hdrp->totNumCells * entry_size;
	}
	else {// need for compression bitmap		
		// number of unsigned integers used to represent this bitmap of size hdrp->totNumCells
		//int no_words = hdrp->totNumCells/bmp::BITSPERWORD + 1;
		int no_words = bmp::numOfWords(hdrp->totNumCells);
		hdrp->size += (hdrp->rlNumCells * entry_size) + no_words*sizeof(bmp::WORD);	
	}
}
*/

/*void DataChunk::calculateSize(ChunkHeader* hdrp, unsigned int numfacts)
{	
	int entry_size = numfacts * sizeof(measure_t);
	if(float(hdrp->rlNumCells)/float(hdrp->totNumCells) > SPARSENESS_THRSHLD){
		// no need for compression : i.e. size = totNumCells * entry_size
		hdrp->size = hdrp->totNumCells * entrwy_size;
	}
	else {// need for compression
                // size = bitmap-size + rlNumCells * entry_size
        	hdrp->size = (int)ceil(hdrp->totNumCells/8.0) + (hdrp->rlNumCells * entry_size);	
	}
}*/
//--------------------- end of DataChunk ---------------------------//

//---------------------------------- ChunkHeader ----------------------------------------------//

ChunkHeader::ChunkHeader(): localDepth(Chunk::NULL_DEPTH), nextLocalDepth(false), artificialHierarchyp(0)
{
}//ChunkHeader::ChunkHeader()

ChunkHeader & ChunkHeader::operator=(const ChunkHeader & other)
{
	if(this != &other) {
		// deallocate current data
		if(artificialHierarchyp) delete artificialHierarchyp;

		// duplicate other's data
         	if(other.artificialHierarchyp)
         		artificialHierarchyp = new vector<map<int, LevelRange> >(*(other.artificialHierarchyp));
         	else
         		artificialHierarchyp = 0;
	}
	return (*this);
}//ChunkHeader::operator=()

void ChunkHeader::excludeNULLRanges(const vector<LevelRange>& sourcevectRange, vector<LevelRange>& newvectRange)
// precondition: newvectRange is an empty vector. sourcevectRange contains a set of level ranges,
//      which some of them might be NULL (i.e., ranges fro pseudo levels)
// postcondition:
//      newvectRange contains all the ranges of sourcevectRanges with the same order,
//      without the pseudo ranges.
//
{
        //ASSERTION1: newVectRange is empty
        if(!newvectRange.empty())
                throw GeneralError(__FILE__, __LINE__, "ChunkHeader::excludeNULLRanges ==> ASSERTION1 : non-empty vector\n");
        for(vector<LevelRange>::const_iterator rng_i = sourcevectRange.begin(); rng_i != sourcevectRange.end(); rng_i++){
                  if(rng_i->leftEnd != LevelRange::NULL_RANGE && rng_i->rightEnd != LevelRange::NULL_RANGE)
                        newvectRange.push_back(*rng_i);
        }//end for
}// end of ChunkHeader::excludeNULLRanges

//---------------------------------- end of ChunkHeader ---------------------------------------//

//--------------------- LevelRange ---------------------------------//
LevelRange::LevelRange(const LevelRange& l)
	: dimName(l.dimName),lvlName(l.lvlName),leftEnd(l.leftEnd),rightEnd(l.rightEnd)
{
}
LevelRange& LevelRange::operator=(LevelRange const& other)
{
	dimName = other.dimName;
	lvlName = other.lvlName;
	leftEnd = other.leftEnd;
	rightEnd = other.rightEnd;
	return (*this);
}
//--------------------- end of LevelRange --------------------------//

//----------------------- Coordinates --------------------------------------------------------------------//
void Coordinates::excludePseudoCoords(Coordinates & newcoords) const
// precondition: newcoords contains an empty vector of coordinates. (*this) contains a set of coordinate values,
//      which some of them might be pseudo order codes.
// postcondition:
//      newcoords contains all the coordinates of (*this) with the same order, without the pseudo coordinate values.
//      The state of "*this" object remains unchanged!
//
{
        //ASSERTION1: newcoords is empty
        if(!newcoords.cVect.empty())
                throw GeneralError(__FILE__, __LINE__, "Coordinates::excludePseudoCoords ==> ASSERTION1: non-empty vector\n");
        for(vector<int>::const_iterator i = cVect.begin(); i != cVect.end(); i++){
                  // if this is not a pseudo coordinate
                  if(*i != LevelMember::PSEUDO_CODE){
                        //include it
                        newcoords.cVect.push_back(*i);
                        newcoords.numCoords++;
                  }//end if
        }//end for
}// end Coordinates::excludePseudoCoords(const Coordinates & coords)\

//---------------------- end of Coordinates -------------------------------------------------------------//

// ---------------------------- Cell ---------------------------------------------------------------------
Cell::Cell(const Coordinates& c, const vector<LevelRange>& b): coords(c), boundaries(b){
        //if coordinates and boundaries have different dimensionality
        if(coords.numCoords != boundaries.size())
                throw GeneralError(__FILE__, __LINE__, "Cell::Cell ==> different dimensionality between coordinates and boundaries");			
}//Cell::Cell()

bool Cell::isFirstCell() const {
 	if(isCellEmpty())
 		return false;
 	bool allPseudo = true;
 	for(int coordIndex = 0; coordIndex < coords.numCoords; coordIndex++){
        	//if coord is not a pseudo
        	if(coords.cVect[coordIndex] != LevelMember::PSEUDO_CODE){
        		allPseudo = false;	 	
        		if(coords.cVect[coordIndex] != boundaries[coordIndex].leftEnd)
        			return false;	 	
        	}//end if
 	}//end for
 	 	
 	//if not all pseudo then we are indeed at the first cell
 	return (!allPseudo) ? true : false;
}//Cell::isFirstCell

bool Cell::isLastCell() const {
 	if(isCellEmpty())
 		return false;
 	
        bool allPseudo = true;	
 	for(int coordIndex = 0; coordIndex < coords.numCoords; coordIndex++){
        	//if coord is not a pseudo
        	if(coords.cVect[coordIndex] != LevelMember::PSEUDO_CODE){
        		allPseudo = false;	 		 	
 			if(coords.cVect[coordIndex] != boundaries[coordIndex].rightEnd)
       				return false;	 	
       		}//end if
 	}//end for
 	 	
 	//if not all pseudo then we are indeed at the last cell
 	return (!allPseudo) ? true : false;
}//Cell::isLastCell
 	
bool Cell::cellWithinBoundaries() const {
 	if(isCellEmpty())
 		return false;
 	
 	bool allPseudo = true;
 	for(int coordIndex = 0; coordIndex < coords.numCoords; coordIndex++){
        	//if coord is not a pseudo
        	if(coords.cVect[coordIndex] != LevelMember::PSEUDO_CODE){
        		allPseudo = false;	 		 	  	 	
  			if(coords.cVect[coordIndex] < boundaries[coordIndex].leftEnd
  				||
  			   coords.cVect[coordIndex] > boundaries[coordIndex].rightEnd)
        			return false;	 	
        	}//end if
 	}//end for
 	//if not all pseudo then we are indeed within boundaries
 	return (!allPseudo) ? true : false;
}//Cell:: cellWithinBoundaries()
 	
void Cell::reset(int startCoordIndex){
 	if(isCellEmpty())
 		throw GeneralError(__FILE__, __LINE__, "Cell::reset ==> empty cell\n");
 	
 	//assert that input index is within limits
 	if(startCoordIndex < 0 || startCoordIndex > coords.numCoords-1)
 		throw GeneralError(__FILE__, __LINE__, "Cell::reset ==> input index out of range\n");
 	 		
 	for(int coordIndex = startCoordIndex; coordIndex < coords.numCoords; coordIndex++){
        	//if coord is not a pseudo
        	if(coords.cVect[coordIndex] != LevelMember::PSEUDO_CODE){
        		coords.cVect[coordIndex] = boundaries[coordIndex].leftEnd;
        	}//end if	 	
 	}//end for
}//Cell::reset

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
       		throw GeneralError(__FILE__, __LINE__, "Cell::becomeNextCell ==> empty cell\n");

	//assert that current coordinates are within boundaries
	if(!cellWithinBoundaries())
		throw GeneralError(__FILE__, __LINE__, "Cell::becomeNextCell() ==> cell out of boundaries\n");
	
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

// ---------------------------- end of Cell --------------------------------------------------------------
