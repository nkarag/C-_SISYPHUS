#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>

/**
 * This class represents a chunk id. i.e. the unique string identifier that is
 * derived from the interleaving of the member-codes of the members of the pivot-set levels
 * that define this chunk.
 * @author: Nikos Karayannidis
 */
class ChunkID {

private:
	string cid;

public:
	/**
	 * Default Constructor of the ChunkID
	 */
	ChunkID() : cid(""){ }

	/**
	 * Constructor of the ChunkID
	 */
	ChunkID(const string& s) : cid(s) {}
	/**
	 * copy constructor
	 */
	ChunkID(const ChunkID& id) : cid(id.getcid()) {}
	/**
	 * Destructor of the ChunkID
	 */
	~ChunkID() { }

	/**
	 * Required in order to use the find standard algortithm (see STL)
	 * for ChunkIDs
	 */
	friend bool operator==(const ChunkID& c1, const ChunkID& c2);

	/**
	 * This operator is required in order to define map containers (see STL)
	 * with a ChunkID as a key.
	 */
	friend bool operator<(const ChunkID& c1, const ChunkID& c2);

	/**
	 * This function updates a Coordinates struct with the cell
	 * coordinates that extracts from the last domain of a chunk id.
	 *
	 * @param	c	the Coordinates struct to be updated
	 */
	//void extractCoords(Coordinates& c);

	// get/set chunk id
	const string& getcid() const { return cid; }
	void setcid(const string& id) { cid = id; }
}; //ChunkID

/**
 * This class shows which cells inside a chunk have non-NULL values
 *
 * @author Nikos Karayannidis
 */
class CellMap {
public:
	/**
	 * default constructor, initializes an empty vector
	 */
	CellMap();
	~CellMap();

	/**
	 * copy constructor
	 */
	CellMap(CellMap const & map);

	/**
	 * overload assignment operator
	 */
	CellMap const& operator=(CellMap const& other);

	/**
	 * This function inserts a new id in the Chunk Id vector. If the id
	 * already exists then it returns false
	 *
	 * @param	id	the chunk id string
	 */
	 bool insert(const string& id);

	/**
	 * get/set
	 */
	const vector<ChunkID>* getchunkidVectp() const {return chunkidVectp;}
	void setchunkidVectp(vector<ChunkID>* const chv);
	//const vector<ChunkID>& getchunkidVectp() const {return chunkidVect;}
	//void setchunkidVect(const vector<ChunkID>& chv);


private:
	vector<ChunkID>* chunkidVectp;
}; //CellMap



/**
 * This is the header of a chunk, containing information about the chunk
 *
 * @author: Nikos Karayannidis
 */
struct ChunkHeader {
	/**
	 * The chunk-id
	 */
	ChunkID	id;

	/**
	 * the chunking depth
	 */
	unsigned int depth;

	/**
	 * Total number of cells in this chunk, i.e. the cross product of the chunk ranges
	 */
	unsigned int totNumCells;

	/**
	 * Total number of existing cells in this chunk. For DirChunks: rlNumCells == totNumCells
	 * while for DataChunks: rlNumCells <= totNumCells
	 */
	unsigned int rlNumCells;

	/**
	 * default constructor
	 */
	ChunkHeader() {}
	/**
	 * copy constructor
	 */
	ChunkHeader(const ChunkHeader& h)
		: depth(h.depth), totNumCells(h.totNumCells),
		  rlNumCells(h.rlNumCells) {}
	~ChunkHeader() {}
};  // end of struct ChunkHeader


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

	static void printTree(CostNode* root, ofstream& out);

	/**
	 * overloaded assignment operator
	 */
	//const CostNode & operator=(const CostNode & other);

	//static void countDirChunksOfTree(CostNode* costRoot, unsigned int maxdepth, unsigned int& total);
	static  void countDataChunksOfTree(CostNode* costRoot, unsigned int maxdepth, unsigned int& total);
	//static void countChunksOfTree(CostNode* costRoot, unsigned int maxdepth, unsigned int& total);
	/**
	 * get/set
	 */
	const ChunkHeader* const getchunkHdrp() const {return chunkHdrp;}
	void setchunkHdrp(ChunkHeader* const chdr) {chunkHdrp = chdr;}

	const CellMap* const getcMapp() const {return cMapp;}
	void setcMapp(CellMap* const map) {cMapp = map;}

	vector<CostNode*>& getchild() {return child;}
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


/**
 * Class to hold information about the cube
 */
class CubeInfo {

private:
	/**
	 * Name of the cube
	 */
	string name;

	/**
	 * the maximum chunking depth for this cube
	 */
	unsigned int maxDepth;

	/**
	 * names of the facts stored in each cube cell (i.e. data chunk cell)
	 */
	vector<string> factNames;

	/**
	 * Number of facts in a cell
	 */
	unsigned int numFacts;


public:
	/**
	 * CubeInfo constructor.
	 */
	CubeInfo(){}

	/**
	 * CubeInfo constructor.
	 *
	 * @param name	the name of the cube.
	 */
	CubeInfo(const string& name):name(name){}

	/**
	 * CubeInfo destructor
	 */
	~CubeInfo(){}

	const string& get_name() const { return name; }
	void set_name(const string& n) { name = n; }


	const unsigned int getmaxDepth() const {return maxDepth;}
	void setmaxDepth(unsigned int d) {maxDepth = d;}

	const vector<string>& getfactNames() const {return factNames;}
	void setfactNames(const vector<string>& svect) {factNames = svect;}

	unsigned int getnumFacts() const {return numFacts;}
	void setnumFacts(unsigned int n) {numFacts = n;}

}; //end class CubeInfo

class Chunk {
public:
	/**
	 * this is the depth at the root chunk level
	 */
	static const unsigned int MIN_DEPTH = 1;
};//end class Chunk

//------------------- function declaration
void testCase1(CubeInfo& cbinfo, CostNode* &root);
CostNode* createCostTree(string& dimFile, string& factFile, CubeInfo& cbinfo);


main()//int argc, char* argv[])
{
        //cout<<argv[1]<<endl;
        //cout<<argv[2]<<endl;
        //string dimFile = argv[1];
        //string factFile = argv[2];

        //create a cost node tree
        CubeInfo cbinfo(string("lalacube"));
        CostNode* costRoot = createCostTree(string(""),string(""), cbinfo);

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


        unsigned int numChunks = 0;
        CostNode::countDataChunksOfTree(costRoot, cbinfo.getmaxDepth(), numChunks);

        cout<<"number of Data chunks is: "<<numChunks<<endl;
}

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


CostNode* createCostTree(string& dimFile, string& factFile, CubeInfo& cbinfo){
        CostNode* root = 0;
        //Test case 1 (chunk tree in figure 5 in SISYPHUS TR)
        testCase1(cbinfo, root);


        return root;
}

// create the costNode tree for test-case 1(chunk tree in figure 5 in SISYPHUS TR). Updates
// accordingly cbinfo and root
void testCase1(CubeInfo& cbinfo, CostNode* &root)
{
        // creates the cost node tree corresponding to Fig. 5 of SISYPHUS TR
        cbinfo.setmaxDepth(3);
        cbinfo.setnumFacts(2);

        //create root node 0|0

        //create chunk header
        ChunkHeader* hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0"));
        //      depth
        hdrp->depth = 2;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 4;

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
        hdrp->depth = 3;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.0|0.0|-1"));
        clmpp->insert(string("0|0.0|0.1|-1"));

        // add node to its father
        root->getchild().push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.0|0.0|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|0.0|-1"));
        //      depth
        hdrp->depth = 4;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.0|0.0|-1.0|0"));
        clmpp->insert(string("0|0.0|0.0|-1.0|1"));

        // add node to its father
        CostNode* father = root->getchild().back();
        father->getchild().push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.0|0.1|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|0.1|-1"));
        //      depth
        hdrp->depth = 4;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 4;

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.0|0.1|-1.1|0"));
        clmpp->insert(string("0|0.0|0.1|-1.1|1"));
        clmpp->insert(string("0|0.0|0.1|-1.2|0"));
        clmpp->insert(string("0|0.0|0.1|-1.2|1"));

        // add node to its father
        father = root->getchild().back();
        father->getchild().push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.0|1
        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|1"));
        //      depth
        hdrp->depth = 3;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.0|1.0|-1"));
        clmpp->insert(string("0|0.0|1.1|-1"));

        // add node to its father
        root->getchild().push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.0|1.0|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|1.0|-1"));
        //      depth
        hdrp->depth = 4;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.0|1.0|-1.0|2"));
        clmpp->insert(string("0|0.0|1.0|-1.0|3"));

        // add node to its father
        father = root->getchild().back();
        father->getchild().push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.0|1.1|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|1.1|-1"));
        //      depth
        hdrp->depth = 4;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 4;

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.0|1.1|-1.1|2"));
        clmpp->insert(string("0|0.0|1.1|-1.1|3"));
        clmpp->insert(string("0|0.0|1.1|-1.2|2"));
        clmpp->insert(string("0|0.0|1.1|-1.2|3"));

        // add node to its father
        father = root->getchild().back();
        father->getchild().push_back(new CostNode(hdrp, clmpp));


        // create node 0|0.1|0

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|0"));
        //      depth
        hdrp->depth = 3;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|0.2|-1"));
        clmpp->insert(string("0|0.1|0.3|-1"));

        // add node to its father
        root->getchild().push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.1|0.2|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|0.2|-1"));
        //      depth
        hdrp->depth = 4;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|0.2|-1.3|0"));
        clmpp->insert(string("0|0.1|0.2|-1.3|1"));

        // add node to its father
        father = root->getchild().back();
        father->getchild().push_back(new CostNode(hdrp, clmpp));


        // create node 0|0.1|0.3|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|0.3|-1"));
        //      depth
        hdrp->depth = 4;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 4;

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|0.3|-1.4|0"));
        clmpp->insert(string("0|0.1|0.3|-1.4|1"));
        clmpp->insert(string("0|0.1|0.3|-1.5|0"));
        clmpp->insert(string("0|0.1|0.3|-1.5|1"));

        // add node to its father
        father = root->getchild().back();
        father->getchild().push_back(new CostNode(hdrp, clmpp));


        // create node 0|0.1|1
        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|1"));
        //      depth
        hdrp->depth = 3;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|1.2|-1"));
        clmpp->insert(string("0|0.1|1.3|-1"));

        // add node to its father
        root->getchild().push_back(new CostNode(hdrp, clmpp));

        // create node 0|0.1|1.2|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|1.2|-1"));
        //      depth
        hdrp->depth = 4;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|1.2|-1.3|2"));
        clmpp->insert(string("0|0.1|1.2|-1.3|3"));

        // add node to its father
        father = root->getchild().back();
        father->getchild().push_back(new CostNode(hdrp, clmpp));


        // create node 0|0.1|1.3|-1

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|1.3|-1"));
        //      depth
        hdrp->depth = 4;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 4;

        //create cell map
        clmpp = new CellMap;
        clmpp->insert(string("0|0.1|1.3|-1.4|2"));
        clmpp->insert(string("0|0.1|1.3|-1.4|3"));
        clmpp->insert(string("0|0.1|1.3|-1.5|2"));
        clmpp->insert(string("0|0.1|1.3|-1.5|3"));

        // add node to its father
        father = root->getchild().back();
        father->getchild().push_back(new CostNode(hdrp, clmpp));

}// testCase1()

//--------------------- CostNode
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
       	//for (	vector<LevelRange>::const_iterator i = root->getchunkHdrp()->vectRange.begin();
  	//	i != root->getchunkHdrp()->vectRange.end();	++i){
  	//		out<<"\t"<<(*i).dimName<<"("<<(*i).lvlName<<") : ["<<(*i).leftEnd<<", "<<(*i).rightEnd<<"]"<<endl;
 	//}
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


//----------------------- ChunkID
bool operator<(const ChunkID& c1, const ChunkID& c2)
{
	return (c1.cid < c2.cid);
}

bool operator==(const ChunkID& c1, const ChunkID& c2)
{
	return (c1.cid == c2.cid);
}

//---------------------- CellMap
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