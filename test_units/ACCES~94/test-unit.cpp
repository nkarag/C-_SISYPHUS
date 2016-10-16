#include <string>
#include <iostream>

#include "Cube.h"
#include "Chunk.h"

CostNode* load_cube (string& cbname,string& dimFile, string& factFile, CubeInfo& cbinfo);
void descendDepth1stCostTree(CubeInfo& cinfo,
				 CostNode* costRoot,
				 string& factFile,
				 BucketID& bcktID,
				 vector<DirChunk>*dirVectp,
				 vector<DataChunk>*dataVectp);
void constructCubeFile(CubeInfo& cinfo, string& factFile, CostNode* costRoot);				
				
struct ChunkHeader; //fwd declaration
class ofstream;
/**
 * This class represents a "chunk node" in the chunk hierarchy. It contains chunk header information
 * as well as a CellMap indicating which cells have NOT NULL values
 *
 * @author Nikos Karayannidis
 */
class CostNode {
public:
	/**
	 * Default constructor
	 */
	 CostNode() : chunkHdrp(0),cMapp(0),child(){}
	//CostNode() : chunkHdrp(0),cMapp(0),child(0){}
	/**
	 * constructor
	 */
	 CostNode(ChunkHeader* const hdr, CellMap* const map): chunkHdrp(hdr),cMapp(map),child() {}
	 //CostNode(ChunkHeader* const hdr, CellMap* const map, vector<CostNode>* const c): chunkHdrp(hdr),cMapp(map),child(c) {}
	~CostNode();
	/**
	 * overloaded assignment operator
	 */
	const CostNode & operator=(const CostNode & other);
	/**
	 * Prints the information stored in a cost tree hanging from root,
	 * into out.
	 *
	 * @param root	pointer to the tree root
	 * @param out	the output file
	 */
	static void printTree(CostNode* root, ofstream& out);	
	/**
	 * Calculates the total size for the subtree pointed to by root.
	 *
	 * @param root	pointer to the tree root
	 * @param szBytes	the returned size in bytes
	 * @param szPages	the returned size in disk pages
	 */
	static void calcTreeSize(CostNode* root, unsigned int& szBytes); //, unsigned int& szPages);
	
	/**
	 * get/set
	 */
	const ChunkHeader* const getchunkHdrp() const {return chunkHdrp;}
	void setchunkHdrp(ChunkHeader* const chdr) {chunkHdrp = chdr;}

	const CellMap* const getcMapp() const {return cMapp;}
	void setcMapp(CellMap* const map) {cMapp = map;}

	const vector<CostNode*>& getchild() const {return child;}
	void setchild(const vector<CostNode*> & chld) {child = chld;}
	//const vector<CostNode>*const getchild() const {return child;}
	//void setchild(vector<CostNode>* const chld) {child = chld;}
	
private:
	/**
	 * pointer to chunk header
	 */
	ChunkHeader* chunkHdrp;
	/**
	 * pointer to CellMap
	 */
	CellMap* cMapp;
	/**
	 * pointers to children nodes. Note: the order of the childs is identical
	 * to the one in the cMapp.
	 */
	 //vector<CostNode>* child;
	vector<CostNode*> child;
}; //end class CostNode
				

//************************* main program ************************************//
main(int argc char* argv[]){
        //get input file with dimension schema
        string dimFile(argv[1]);
        //get input file with fact data
        string factFile(argv[2]);
        //give a cube name
        string cbname("lalacube");

        //create a cost node tree
        CostNode* costRoot = 0;
        CubeInfo cbinfo(cbname);
        try{
                costRoot = load_cube (cbname,dimFile,factFile, cbinfo);
        }
	catch(const char* message) {
		cerr<<"Ex. from AccessManager::load_cube in main(): \n";
	}

        //call descendDepth1stCostTree
        // 1. In order to create a Bucket instance, we need a vector of DirChunks, a vector of DataChunks
	vector<DirChunk>* dirVectp = new vector<DirChunk>;
	dirVectp->reserve(CostNode::countDirChunksOfTree(costRoot));
	
	vector<DataChunk>* dataVectp = new vector<DataChunk>;
	dataVectp->reserve(CostNode::countDataChunksOfTree(costRoot));	
	
	// 1.1. Create a BucketID for the  Bucket.
        //      NOTE: no bucket allocation performed yet, just id generation!
	rc_t err;
	serial_t record_ID;
        err = ss_m::create_id(SystemManager::getDevVolInfo()->volumeID , 1, record_ID);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"AccessManager::storeTreeInBucket  ==> Error in ss_m::create_id"<< err <<endl;
		// throw an exeption
		throw error.str();
	}
	BucketID bcktID(record_ID);
		
	// 2. Fill the chunk vectors
	// 2.1 Descend the tree (depth-first) and insert each chunk in the corresponding vector
	//    as you first meet it, i.e. the root will be stored at position begin() of the vector.
	try {
		descendDepth1stCostTree(cinfo,costRoot,factFile,bcktID,dirVectp,dataVectp);
	}
      	catch(const char* message) {
      		string msg("");
      		msg += message;
      		throw msg.c_str();
      	}
      	//ASSERTION2: valid returned vectors
       	if(dirVectp->empty() && dataVectp->empty()){
       			throw "AccessManager::storeTreeInBucket  ==>ASSERTION2 empty both chunk vectors!!\n";
       	}
       	else if (dataVectp->empty()){
       			throw "AccessManager::storeTreeInBucket  ==>ASSERTION2 empty data chunk vector!!\n";
	}

        //output the resulting vectors with the chunks
        outputVectors(dirVectp, dataVectp);
} // end main

void descendDepth1stCostTree(CubeInfo& cinfo,
				 CostNode* costRoot,
				 string& factFile,
				 BucketID& bcktID,
				 vector<DirChunk>*dirVectp,
				 vector<DataChunk>*dataVectp)
// precondition:
//	costRoot points at a tree, where BUCKET_THRESHOLD<= tree-size <= DiskBucket::bodysize

// postcondition:
//	The dir chunks and the data chunks of this tree have been filled with values
//	and are stored in heap space in two vectors: dirVectp and dataVectp, in the following manner:
//	We descend in a depth first manner and we store each node (Chunk) as we first visit it.
{
	if(!costRoot) {
	 	//empty sub-tree
	 	throw "AccessManager::descendDepth1stCostTree  ==> Emtpy sub-tree!!\n";
	}
	// I. If this is not a leaf chunk
	if(costRoot->getchunkHdrp()->depth <= cbinfo.getmaxDepth()){
	     	// 1. Create the vector holding the chunk's entries.
	     	//      NOTE: the default DirEntry constructor should initialize the member BucketID
	     	//	with a BucketID::null constant.
	     	vector<DirEntry> entryVect(costRoot->getchunkHdrp()->totNumCells);
	     	
	     	// 2. Fill in those entries
	     	//the current chunk (costRoot) will be stored in chunk slot == dirVetp->size()
	     	// thh 1st child will be stored just after the current chunk
	     	unsigned int index = dirVectp->size() + 1;
		vector<ChunkID>::const_iterator icid = costRoot->getcMapp()->begin();
		vector<CostNode*>::const_iterator ichild = costRoot->getchild().begin();
		// ASSERTION1: size of CostNode vectors is equal
		if(costRoot->getchild().size() != costRoot->getcMapp()->size())
			throw "AccessManager::descendDepth1stCostTree ==> ASSERTION1: != vector sizes\n";		
		while(ichild != costRoot->getchild().end() && icid != costRoot->getcMapp()->end()){
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
			unsigned int offs = DirChunk::calcCellOffset(c);
       			if(offs >= entryVect.size()){
       				throw "AccessManager::descendDepth1stCostTree ==>ASSERTION2: entryVect out of range!\n";
       			}			
			entryVect[offs] = e;

			index = index + CostNode::countChunksOfTree(*ichild);	
			ichild++;
			icid++;					
		} //end while
	     	
	     	// 3. Now that entryVect is filled, create dirchunk object
		DirChunk newChunk((*costRoot->getchunkHdrp()), entryVect);	     	
	     	
	     	// 4. Store new chunk in the corresponding vector
	     	dirVectp->push_back(newChunk);
	     	
	     	// 5. Descend to children
	     	for(vector<CostNode*>::const_iterator ichd = costRoot->getchild().begin();
	     		ichd != costRoot->getchild().end();
	     		++ichd) {
                	try {
                		descendDepth1stCostTree(CubeInfo& cinfo,
                				(*ichd),
                				string& factFile,
                				bcktID,
                				dirVectp,
                				dataVectp);
                	}
                      	catch(const char* message) {
                      		string msg("");
                      		msg += message;
                      		throw msg.c_str();
                      	}
                      	//ASSERTION3
                       	if(dirVectp->empty() || dataVectp->empty()){
                       			throw "AccessManager::descendDepth1stCostTree  ==>ASSERTION3: empty chunk vectors (possibly both)!!\n";
                       	}
	     	} // end for
	} //end if
	else { // II. this is a leaf chunk
		//ASSERTION4: depth check
		if(costRoot->getchunkHdrp()->depth != cbinfo.getmaxDepth()+1)
			throw "AccessManager::descendDepth1stCostTree ==>ASSERTION4: Wrong depth in leaf chunk!\n";	
		
		// 1. Create the vector holding the chunk's entries and the compression bitmap (initialized to 0's).
	     	vector<DataEntry> entryVect(costRoot->getchunkHdrp()->rlNumCells);
	     	bit_vector cmprBmp(costRoot->getchunkHdrp()->totNumCells, false);
	     	
	     	// 2. Fill in those entries
        	// open input file for reading
        	ifstream input(factFile.c_str());
        	if(!input)
        		throw "AccessManager::descendDepth1stCostTree ==> Error in creating ifstream obj\n";

        	string buffer;
        	// skip all schema staff and get to the fact values section
        	do{
        		input >> buffer;
        	}while(buffer != "VALUES_START");
        	
        	// read on until you find prefix corresponding to current data chunk        	
        	string prefix = costRoot->getchunkHdrp()->getcid();
        	do {
	        	input >> buffer;
	        	if(input.eof())
	        		throw "AccessManager::descendDepth1stCostTree ==> Can't find prefix in input file\n";
	        }while(buffer.find(prefix) != 0);
	
	        // now, we 've got a prefix match.	
	        // Read the fact values for the non-empty cells of this chunk.
	        unsigned int factsPerCell = cinfo->getnumFacts();
	        vector<measure_t> factv(factsPerCell);
		map<ChunkID, DataEntry> helpmap; // for temporary storage of entries	
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
			helpmap[cellid] = e;
							
			// insert entry at cmprBmp in the right offset (calculated from the Chunk Id)			
			//ASSERTION5
			unsigned int offs = DirChunk::calcCellOffset(c);
       			if(offs >= cmprBmp.size()){
       				throw "AccessManager::descendDepth1stCostTree ==>ASSERTION5: cmprBmp out of range!\n";
       			}			
			cmprBmp[offs] = true; //this cell is non-empty							
													
	        	//read next cell id (i.e. chunk  id)
	        	input >> buffer;	
	        } while(buffer.find(prefix) == 0 && !input.eof()); // we are still under the same prefix	
		input.close();	     	

		// now store into the entry vector of the data chunk
		for(map<ChunkID, DataEntry>::const_iterator map_i = helpmap.begin();
		    map_i != helpmap.end(); ++map_i) {
        		// insert entry at entryVect in the right offset (calculated from the Chunk Id)			
        		Coordinates c;
        		map_i->first.extractCoords(c);
        		//ASSERTION6
        		unsigned int offs = DataChunk::calcCellOffset(c, cmprBmp);
       			if(offs >= entryVect.size()){
       				throw "AccessManager::descendDepth1stCostTree ==>ASSERTION6: (DataChunk) entryVect out of range!\n";
       			}			
        		entryVect[offs] = map_i->second;		    		
		}//end for		
		
	     	// 3. Now that entryVect and cmprBmp are filled, create dataChunk object
		DataChunk newChunk((*costRoot->getchunkHdrp()), entryVect, cmprBmp);	     	
	     	
	     	// 4. Store new chunk in the corresponding vector
	     	dataVectp->push_back(newChunk);		
	} // end else		
}// end of AccessManager::descendDepth1stCostTree


CostNode* AccessManager::load_cube (string& name, string& dimFile, string& factFile)
{

        //construct a CubeInfo
        CubeInfo info(name);

	// get information about the dimensions from the dimFile
	try {
		info.Get_dimension_information(dimFile);
	}
	catch(const char* message) {
		throw "Ex. from CubeInfo::Get_dimension_information in AccessManager::load_cube(): \n";
	}
	// show dimension information on the screen
	info.Show_dimensions();
        #ifdef DEBUGGING
              cerr << "Reading the fact schema info..." <<endl;
        #endif
	// get fact information  from file
	try {
		info.getFactInfo(factFile);
	}
	catch(const char* message) {
		throw "Ex. from CubeInfo::getFactInfo in AccessManager::load_cube(): ";
	}
        #ifdef DEBUGGING
              cerr << "Starting the Cube File construction algorithm ..." <<endl;
        #endif	
	// Construct CUBE File
	CostNode* costRoot = 0;
	try{
		constructCubeFile(info, factFile, costRoot);
	}
	catch(const char* message) {
		throw "Ex from constructCubeFile\n");//"Ex. from AccessManager::constructCubeFile in AccessManager::load_cube : ");
	}

	return costRoot;
} // load_cube


void constructCubeFile(CubeInfo& cinfo, string& factFile, CostNode* costRoot)
{
	// 1. Estimate the storage cost for the components of the chunk hierarchy tree

	// In this phase we will use only chunk headers.
	// 1.1 construct root chunk header.
	ChunkHeader* rootHdrp = new ChunkHeader;
	Chunk::createRootChunkHeader(rootHdrp, cinfo);

	// 1.2 create CostNode Tree
	//CostNode* costRoot = 0;
	try {
		costRoot = Chunk::expandChunk(rootHdrp, cinfo, factFile);
	}
	catch(const char* message) {
		//string msg("");//"Ex. from Chunk::expandChunk in AccessManager::constructCubeFile : ");
		//msg += message;
		throw; //msg.c_str();
	}	
}// constructCubeFile

// --------------------------- class CostNode -----------------------------------------------//
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
       	out <<"\tDepth = "<<root->getchunkHdrp()->depth<<", NumOfDims = "
       		<<root->getchunkHdrp()->numDim<<", totNumCells = "
       		<<root->getchunkHdrp()->totNumCells<<", rlNumCells = "
       		<<root->getchunkHdrp()->rlNumCells<<", size(bytes) = "
       		<<root->getchunkHdrp()->size<<endl;
	
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

void CostNode::calcTreeSize(CostNode* root, unsigned int& szBytes) //,unsigned int& szPages)
{
	if(!root){
		return;
	}
	szBytes += root->getchunkHdrp()->size;
	//szPages += int(ceil(float(szBytes)/float(PAGESIZE)));
   	// descend to children
      	for (	vector<CostNode*>::const_iterator j = root->getchild().begin();
   		j != root->getchild().end();
   		++j	){
		CostNode::calcTreeSize(*j, szBytes); //, szPages);  			
   	}   		
} // end of CostNode::calcTreeSize

// -------------------------------- end of CostNode --------------------------------------------
