#include "descend.h"
#include "DiskStructures.h"

const int LevelRange::NULL_RANGE=-1;

void createChunkVectors(unsigned int& maxDepth, unsigned int& numFacts, vector<DirChunk>* &dirVectp,
		   vector<DataChunk>* &dataVectp, TreeTraversal_t &howToTraverse)
{
        cout<<"Choose traversal method (1 breadth1st, 2 depth1st) : ";
        int answr;
        cin>>answr;
        (answr == 1) ? howToTraverse = breadthFirst : howToTraverse = depthFirst;

        string factFile("facts.fld");

        //create a cost node tree
        CubeInfo cbinfo(string("lalacube"));
        CostNode* costRoot = createCostTree(cbinfo, maxDepth, numFacts);

        //test the created tree
	ofstream out("test-tree");

        if (!out)
        {
            cerr << "creating file \"test-tree\" failed\n";
        }

        out <<"**************************************************"<<endl;
        out <<"*	    CHUNK TREE INFORMATION              *"<<endl;
        out <<"**************************************************"<<endl;
        out <<"\n\n";

        // Start traversing the tree..
        CostNode::printTree(costRoot, out);



        // 1. In order to create a Bucket instance, we need a vector of DirChunks, a vector of DataChunks
	dirVectp = new vector<DirChunk>;
	unsigned int numDirChunks = 0;
	CostNode::countDirChunksOfTree(costRoot, cbinfo.getmaxDepth(), numDirChunks);
	dirVectp->reserve(numDirChunks);

	dataVectp = new vector<DataChunk>;
	unsigned int numDataChunks = 0;
	CostNode::countDataChunksOfTree(costRoot, cbinfo.getmaxDepth(), numDataChunks);
	dataVectp->reserve(numDataChunks);

        //call descendXXX1stCostTree
        switch(howToTraverse){
                case breadthFirst:
                        {
                        	BucketID bcktID;
                        	queue<CostNode*> nextToVisit; // breadth-1st queue
                        	try {
                        		descendBreadth1stCostTree(
                        		        cbinfo,
                        		        costRoot,
                        		        factFile,
                        		        bcktID,
                        		        nextToVisit,
                        		        dirVectp,
                        		        dataVectp);
                        	}
                              	catch(const char* message) {
                              		string msg("");
                              		msg += message;
                              		throw msg;
                              	}                              	
                      	}
                      	break;
                case depthFirst:
                        {
                        	BucketID bcktID;
                        	try {
                        		descendDepth1stCostTree(
                        		        cbinfo,
        				        costRoot,
        				        factFile,
        				        bcktID,
        				        dirVectp,
        				        dataVectp);				
                        	}
                              	catch(const char* message) {
                              		string msg("");
                              		msg += message;
                              		throw msg;
                              	}
                      	}
              		break;
                 default:
                        throw "createChunkVectors ==> Unkown traversal method\n";
                        break;
         }//end switch

      	//ASSERTION2: valid returned vectors
       	if(dirVectp->empty() && dataVectp->empty()){
       			cerr<< "AccessManager::storeTreeInBucket  ==>ASSERTION2 empty both chunk vectors!!\n";
       	}// end if
       	else if (dataVectp->empty()){
       			cerr<< "AccessManager::storeTreeInBucket  ==>ASSERTION2 empty data chunk vector!!\n";
	}//end else if
} // end createChunkVectors()

CostNode* createCostTree(CubeInfo& cbinfo, unsigned int& maxDepth, unsigned int& numFacts){
        CostNode* root = 0;
        //Test case 1 (chunk subtree in figure 5 in SISYPHUS TR)
        testCase1(cbinfo, maxDepth, numFacts, root);

        //Test case 2 (whole chunk tree in figure 5 in SISYPHUS TR)
        //testCase2(cbinfo, maxDepth, numFacts, root);

        return root;
}

// create the costNode tree for test-case 2(whole chunk tree in figure 5 in SISYPHUS TR). Updates
// accordingly cbinfo and root
void testCase2(CubeInfo& cbinfo, unsigned int& maxDepth, unsigned int& numFacts, CostNode* &root)
{
        // creates the cost node tree corresponding to Fig. 5 of SISYPHUS TR
        cbinfo.setmaxDepth(3);
        maxDepth = 3;
        cbinfo.setnumFacts(2);
        numFacts = 2;

        //create "root chunk"
        //create chunk header
        ChunkHeader* hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("root"));
        //      depth
        hdrp->depth = Chunk::MIN_DEPTH;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 6;
        //      real num of cells
        hdrp->rlNumCells = 6;
        //      level ranges
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("continent"),0,2));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("Category"),0,1));

        //create cell map
        CellMap* clmpp = new CellMap;
        clmpp->insert(string("0|0"));

        root = new CostNode(hdrp, clmpp);

        //create the single child node
        CostNode* child;
        testCase1(cbinfo, maxDepth, numFacts, child);
        const_cast<vector<CostNode*>&>(root->getchild()).push_back(child);
}//end testCase2


// create the costNode tree for test-case 1(chunk subtree in figure 5 in SISYPHUS TR). Updates
// accordingly cbinfo and root
void testCase1(CubeInfo& cbinfo, unsigned int& maxDepth, unsigned int& numFacts, CostNode* &root)
{
        // creates the cost node tree corresponding to Fig. 5 of SISYPHUS TR
        cbinfo.setmaxDepth(3);
        maxDepth = 3;
        cbinfo.setnumFacts(2);
        numFacts = 2;
        unsigned int numDims = 2;

        //create root node 0|0

        //create chunk header
        ChunkHeader* hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0"));
        //      depth
        hdrp->depth = 1;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 4;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DirChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("country"),0,1));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("type"),0,1));


        //create cell map
        CellMap* clmpp = new CellMap;
        clmpp->insert(string("0|0.0|0"));
        clmpp->insert(string("0|0.0|1"));
        clmpp->insert(string("0|0.1|0"));
        clmpp->insert(string("0|0.1|1"));

        root = new CostNode(hdrp, clmpp);

        //create the children nodes

        // create node 0|0.0|0

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|0"));
        //      depth
        hdrp->depth = 2;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DirChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}			
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("region"),0,1));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("Pseudo_level"),
                                                             LevelRange::NULL_RANGE,LevelRange::NULL_RANGE));
       			
        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.0|0.0|-1"));
        clmpp->insert(string("0|0.0|0.1|-1"));

        // add node to its father
        const_cast<vector<CostNode*>&>(root->getchild()).push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.0|0.0|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|0.0|-1"));
        //      depth
        hdrp->depth = 3;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
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
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}							
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),0,0));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"), 0,1));
       	

        //create cell map
        clmpp = new CellMap;
        //clmpp->insert(string("0|0.0|0.0|-1.0|0"));
        clmpp->insert(string("0|0.0|0.0|-1.0|1"));

        // add node to its father
        CostNode* father = root->getchild().back();
        const_cast<vector<CostNode*>&>(father->getchild()).push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.0|0.1|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|0.1|-1"));
        //      depth
        hdrp->depth = 3;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 4;
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
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}							
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),1,2));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"), 0,1));       	

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.0|0.1|-1.1|0"));
        clmpp->insert(string("0|0.0|0.1|-1.1|1"));
        clmpp->insert(string("0|0.0|0.1|-1.2|0"));
        clmpp->insert(string("0|0.0|0.1|-1.2|1"));

        // add node to its father
        father = root->getchild().back();
        const_cast<vector<CostNode*>&>(father->getchild()).push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.0|1
        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|1"));
        //      depth
        hdrp->depth = 2;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DirChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}					
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("region"),0,1));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("Pseudo_level"),
                                                             LevelRange::NULL_RANGE,LevelRange::NULL_RANGE));

       	
        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.0|1.0|-1"));
        clmpp->insert(string("0|0.0|1.1|-1"));

        // add node to its father
        const_cast<vector<CostNode*>&>(root->getchild()).push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.0|1.0|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|1.0|-1"));
        //      depth
        hdrp->depth = 3;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
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
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}							
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),0,0));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"),2,3));

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.0|1.0|-1.0|2"));
        clmpp->insert(string("0|0.0|1.0|-1.0|3"));

        // add node to its father
        father = root->getchild().back();
        const_cast<vector<CostNode*>&>(father->getchild()).push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.0|1.1|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|1.1|-1"));
        //      depth
        hdrp->depth = 3;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 4;
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
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}							
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),1,2));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"),2,3));

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.0|1.1|-1.1|2"));
        clmpp->insert(string("0|0.0|1.1|-1.1|3"));
        clmpp->insert(string("0|0.0|1.1|-1.2|2"));
        clmpp->insert(string("0|0.0|1.1|-1.2|3"));

        // add node to its father
        father = root->getchild().back();
        const_cast<vector<CostNode*>&>(father->getchild()).push_back(new CostNode(hdrp, clmpp));


        // create node 0|0.1|0

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|0"));
        //      depth
        hdrp->depth = 2;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DirChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}					
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("region"),2,3));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("Pseudo_level"),
                                                             LevelRange::NULL_RANGE,LevelRange::NULL_RANGE));
       	

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|0.2|-1"));
        clmpp->insert(string("0|0.1|0.3|-1"));

        // add node to its father
        const_cast<vector<CostNode*>&>(root->getchild()).push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.1|0.2|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|0.2|-1"));
        //      depth
        hdrp->depth = 3;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
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
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}							
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),3,3));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"),0,1));

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|0.2|-1.3|0"));
        clmpp->insert(string("0|0.1|0.2|-1.3|1"));

        // add node to its father
        father = root->getchild().back();
        const_cast<vector<CostNode*>&>(father->getchild()).push_back(new CostNode(hdrp, clmpp));


        // create node 0|0.1|0.3|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|0.3|-1"));
        //      depth
        hdrp->depth = 3;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 4;
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
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}							
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),4,5));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"),0,1));

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|0.3|-1.4|0"));
        clmpp->insert(string("0|0.1|0.3|-1.4|1"));
        clmpp->insert(string("0|0.1|0.3|-1.5|0"));
        clmpp->insert(string("0|0.1|0.3|-1.5|1"));

        // add node to its father
        father = root->getchild().back();
        const_cast<vector<CostNode*>&>(father->getchild()).push_back(new CostNode(hdrp, clmpp));


        // create node 0|0.1|1
        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|1"));
        //      depth
        hdrp->depth = 2;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
       	// calculate the size of this chunk
       	try{
       		hdrp->size = DirChunk::calculateStgSizeInBytes(hdrp->depth,
       							  maxDepth,
       							  numDims,
       							  hdrp->totNumCells);
       	}
       	catch(const char* msg){
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}					
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("region"),2,3));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("Pseudo_level"),
                                                             LevelRange::NULL_RANGE,LevelRange::NULL_RANGE));

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|1.2|-1"));
        clmpp->insert(string("0|0.1|1.3|-1"));

        // add node to its father
        const_cast<vector<CostNode*>&>(root->getchild()).push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.1|1.2|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|1.2|-1"));
        //      depth
        hdrp->depth = 3;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
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
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}							
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),3,3));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"),2,3));

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|1.2|-1.3|2"));
        clmpp->insert(string("0|0.1|1.2|-1.3|3"));

        // add node to its father
        father = root->getchild().back();
        const_cast<vector<CostNode*>&>(father->getchild()).push_back(new CostNode(hdrp, clmpp));


        // create node 0|0.1|1.3|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|1.3|-1"));
        //      depth
        hdrp->depth = 3;
        //      number of dimensions
        hdrp->numDim = 2;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 2;
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
       		string m("testCase1 ==> ");
       		m += msg;
                	throw m.c_str();
       	}							
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),4,5));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"),2,3));

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|1.3|-1.4|2"));
        //clmpp->insert(string("0|0.1|1.3|-1.4|3"));
        clmpp->insert(string("0|0.1|1.3|-1.5|2"));
        //clmpp->insert(string("0|0.1|1.3|-1.5|3"));

        // add node to its father
        father = root->getchild().back();
        const_cast<vector<CostNode*>&>(father->getchild()).push_back(new CostNode(hdrp, clmpp));

}// testCase1()


void descendBreadth1stCostTree(const CubeInfo& cbinfo,
				 const CostNode* const costRoot,
				 const string& factFile,
				 const BucketID& bcktID,
				 queue<CostNode*>& nextToVisit,
				 vector<DirChunk>* const dirVectp,				
				 vector<DataChunk>* const dataVectp)
// precondition:
//	costRoot points at a tree, where BUCKET_THRESHOLD<= tree-size <= DiskBucket::bodysize

// postcondition:
//	The dir chunks and the data chunks of this tree have been filled with values
//	and are stored in heap space in two vectors: dirVectp and dataVectp, in the following manner:
//	We descend in a breadth first manner and we store each node (Chunk) as we first visit it.				
{
        //ASSERTION1: non-empty sub-tree
	if(!costRoot) {
	 	//empty sub-tree
	 	throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION1: Emtpy sub-tree!!\n";
	}
	
	//case I: this corresponds to a directory chunk
	if(costRoot->getchunkHdrp()->depth < cbinfo.getmaxDepth()){
                //Target: create the dir chunk corresponding to the current node

                //allocate entry vector where size = total no of cells
	     	//      NOTE: the default DirEntry constructor should initialize the member BucketID
	     	//	with a BucketID::null constant.
	     	vector<DirEntry> entryVect(costRoot->getchunkHdrp()->totNumCells);
	     	
                //for each child of this node
                int childAscNo = 0;
                //The current chunk will be stored at chunk-slot = dirVectp->size()
                //The current chunk's children will be stored at
                //chunk slot = current node's chunk slot + size of queue +ascending
                //number of child (starting from 1)			
                int indexBase = dirVectp->size() + nextToVisit.size();
                for(vector<CostNode*>::const_iterator ichild = costRoot->getchild().begin();
                    ichild != costRoot->getchild().end(); ichild++) {
                        childAscNo++; //one more child
                        //create corresponding entry in current chunk
                        DirEntry e;
			e.bcktId = bcktID; //store the bucket id where the child chunk will be stored
                        //store the chunk slot in the bucket where the chunk will be stored
                        e.chnkIndex = indexBase + childAscNo;
			
			// insert entry at entryVect in the right offset (calculated from the Chunk Id)
			Coordinates c;
			(*ichild)->getchunkHdrp()->id.extractCoords(c);
			//ASSERTION2
			unsigned int offs = DirChunk::calcCellOffset(c, *costRoot->getchunkHdrp());
       			if(offs >= entryVect.size())
       				throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION2: entryVect out of range!\n";
                        //store  entry in the entry vector
			entryVect[offs] = e;

                        //push child node pointer into queue in order to visit it later
                        nextToVisit.push(*ichild);
                }//end for

                //create dir chunk instance
		DirChunk newChunk(*costRoot->getchunkHdrp(), entryVect);

	     	// store new dir chunk in the corresponding vector
	     	dirVectp->push_back(newChunk);

                //ASSERTION3: assert that queue is not empty at this point
                if(nextToVisit.empty())
                        throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION3: queue cannot be empty at this point!\n";
                //pop next node from the queue and visit it
                CostNode* next = nextToVisit.front();
                nextToVisit.pop(); // remove pointer from queue
               	try {
               		descendBreadth1stCostTree(cbinfo,
               		                next,               				
               				factFile,
               				bcktID,
               				nextToVisit,
               				dirVectp,
               				dataVectp);
               	}
               	catch(const char* message) {
               		string msg("");
               		msg += message;
               		throw msg.c_str();
               	}
               	//ASSERTION4
               	if(dirVectp->empty())
                        throw "AccessManager::descendBreadth1stCostTree  ==>ASSERTION4: empty chunk vectors (possibly both)!!\n";
        }//end if
	else{	//case II: this corresponds to a data chunk
                //Target: create the data chunk corresponding to the current node

		//ASSERTION5: depth check
		if(costRoot->getchunkHdrp()->depth != cbinfo.getmaxDepth())
			throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION5: Wrong depth in leaf chunk!\n";

               // allocate entry vector where size = real number of cells
	     	vector<DataEntry> entryVect(costRoot->getchunkHdrp()->rlNumCells);
               // allocate compression bitmap where size = total number of cells (initialized to 0's).	     	
	     	bit_vector cmprBmp(costRoot->getchunkHdrp()->totNumCells, false);

	     	// Fill in those entries
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
        	string prefix = costRoot->getchunkHdrp()->id.getcid();
        	do {
	        	input >> buffer;
	        	if(input.eof())
	        		throw "AccessManager::descendDepth1stCostTree ==> Can't find prefix in input file\n";
	        }while(buffer.find(prefix) != 0);

	        // now, we 've got a prefix match.
	        // Read the fact values for the non-empty cells of this chunk.
	        unsigned int factsPerCell = cbinfo.getnumFacts();
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

			//ASSERTION6: no such id already exists in the map
			if(helpmap.find(cellid) != helpmap.end())
                                throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION6: double entry for cell in fact load file\n";
			helpmap[cellid] = e;

			// insert entry at cmprBmp in the right offset (calculated from the Chunk Id)
			Coordinates c;
			cellid.extractCoords(c);
			//ASSERTION7
			unsigned int offs = DirChunk::calcCellOffset(c, *costRoot->getchunkHdrp());
       			if(offs >= cmprBmp.size()){
       				throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION7: cmprBmp out of range!\n";
       			}
			cmprBmp[offs] = true; //this cell is non-empty

			numCellsRead++;

	        	//read next cell id (i.e. chunk  id)
	        	input >> buffer;
	        } while(buffer.find(prefix) == 0 && !input.eof()); // we are still under the same prefix
	                                                           // (i.e. data chunk)
		input.close();

		//ASSERTION8: number of non-empty cells read
		if(numCellsRead != costRoot->getchunkHdrp()->rlNumCells)
			throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION8: Wrong number of non-empty cells\n";

		// now store into the entry vector of the data chunk
		for(map<ChunkID, DataEntry>::const_iterator map_i = helpmap.begin();
		    map_i != helpmap.end(); ++map_i) {
        		// insert entry at entryVect in the right offset (calculated from the Chunk Id)
        		Coordinates c;
        		map_i->first.extractCoords(c);

        		bool emptyCell = false;
        		unsigned int offs = DataChunk::calcCellOffset(c, cmprBmp,
        		                                             *costRoot->getchunkHdrp(), emptyCell);
        		//ASSERTION9: offset within range
       			if(offs >= entryVect.size())
       				throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION9: (DataChunk) entryVect out of range!\n";
    			
       			//ASSERTION10: non-empty cell
       			if(emptyCell)
               			throw "AccessManager::descendBreadth1stCostTree ==>ASSERTION10: access to empty cell!\n";
        		//store entry in vector
        		entryVect[offs] = map_i->second;		    		
		}//end for		
		
	     	// Now that entryVect and cmprBmp are filled, create dataChunk object
		DataChunk newChunk(*costRoot->getchunkHdrp(), entryVect, cmprBmp);	     	

	     	// Store new chunk in the corresponding vector
	     	dataVectp->push_back(newChunk);		

                //if the queue is not empty at this point
                if(!nextToVisit.empty()){
                        //pop next node from the queue and visit it
                        CostNode* next = nextToVisit.front();
                        nextToVisit.pop(); // remove pointer from queue
                	try {
                		descendBreadth1stCostTree(cbinfo,                            				
                		                next,
                				factFile,
                				bcktID,
                				nextToVisit,
                				dirVectp,
                				dataVectp);
                	}
                	catch(const char* message) {
                		string msg("");
                		msg += message;
                		throw msg.c_str();
                	}
                	//ASSERTION11
                	if(dirVectp->empty() || dataVectp->empty())
                                throw "AccessManager::descendBreadth1stCostTree  ==>ASSERTION11: empty chunk vectors (possibly both)!!\n";
                }//end if
        }//end else
}//AccessManager::descendBreadth1stCostTree()

void descendDepth1stCostTree(const CubeInfo& cbinfo,
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
	 	throw "AccessManager::descendDepth1stCostTree  ==> Emtpy sub-tree!!\n";
	}
	// I. If this is not a leaf chunk
	if(costRoot->getchunkHdrp()->depth < cbinfo.getmaxDepth()){
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
			throw "AccessManager::descendDepth1stCostTree ==> ASSERTION1: != vector sizes\n";
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
       				throw "AccessManager::descendDepth1stCostTree ==>ASSERTION2: entryVect out of range!\n";
       			}
			entryVect[offs] = e;
                        unsigned int numChunks = 0;
                	CostNode::countChunksOfTree(*ichild, cbinfo.getmaxDepth(), numChunks);
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
                		descendDepth1stCostTree(cbinfo,
                				(*ichd),
                				factFile,
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
		if(costRoot->getchunkHdrp()->depth != cbinfo.getmaxDepth())
			throw "AccessManager::descendDepth1stCostTree ==>ASSERTION4: Wrong depth in leaf chunk!\n";

		// 1. Create the vector holding the chunk's entries and the compression bitmap
		//    (initialized to 0's).
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
        	string prefix = costRoot->getchunkHdrp()->id.getcid();
        	do {
	        	input >> buffer;
	        	if(input.eof())
	        		throw "AccessManager::descendDepth1stCostTree ==> Can't find prefix in input file\n";
	        }while(buffer.find(prefix) != 0);

	        // now, we 've got a prefix match.
	        // Read the fact values for the non-empty cells of this chunk.
	        unsigned int factsPerCell = cbinfo.getnumFacts();
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
                                throw "AccessManager::descendDepth1stCostTree ==>ASSERTION5: double entry for cell in fact load file\n";
			helpmap[cellid] = e;

			// insert entry at cmprBmp in the right offset (calculated from the Chunk Id)
			Coordinates c;
			cellid.extractCoords(c);
			//ASSERTION6
			unsigned int offs = DirChunk::calcCellOffset(c, *costRoot->getchunkHdrp());
       			if(offs >= cmprBmp.size()){
       				throw "AccessManager::descendDepth1stCostTree ==>ASSERTION6: cmprBmp out of range!\n";
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
			throw "AccessManager::descendDepth1stCostTree ==>ASSERTION7: Wrong number of non-empty cells\n";

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
       				throw "AccessManager::descendDepth1stCostTree ==>ASSERTION8: (DataChunk) entryVect out of range!\n";
    			
       			//ASSERTION9: non-empty cell
       			if(emptyCell)
               			throw "AccessManager::descendDepth1stCostTree ==>ASSERTION9: access to empty cell!\n";
        		//store entry in vector
        		entryVect[offs] = map_i->second;		    		
		}//end for		
		
	     	// 3. Now that entryVect and cmprBmp are filled, create dataChunk object
		DataChunk newChunk(*costRoot->getchunkHdrp(), entryVect, cmprBmp);	     	

	     	// 4. Store new chunk in the corresponding vector
	     	dataVectp->push_back(newChunk);		
	} // end else		
}// end of AccessManager::descendDepth1stCostTree

				
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

void CostNode::countDirChunksOfTree(CostNode* costRoot, unsigned int maxdepth, unsigned int& total)
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

void CostNode::countDataChunksOfTree(CostNode* costRoot, unsigned int maxdepth, unsigned int& total)
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

void CostNode::countChunksOfTree(CostNode* costRoot, unsigned int maxdepth, unsigned int& total)
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

