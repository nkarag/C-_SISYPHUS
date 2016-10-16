#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <map>
#include <algorithm>
#include <queue>

class CubeInfo;
class CostNode;
struct BucketID;
class DirChunk;
class DataChunk;
//************** function declarations ****************************//
void descendBreadth1stCostTree(const CubeInfo& cinfo,
				 const CostNode* const costRoot,
				 const string& factFile,
				 const BucketID& bcktID,
				 queue<CostNode*>& nextToVisit,
				 vector<DirChunk>* const dirVectp,
				 vector<DataChunk>* const dataVectp);

void testCase1(CubeInfo& cbinfo, CostNode* &root);
void testCase2(CubeInfo& cbinfo, CostNode* &root);
CostNode* createCostTree(string& dimFile, string& factFile, CubeInfo& cbinfo);
void outputVectors(vector<DirChunk>* dirVectp, vector<DataChunk>* dataVectp);

//***************** class definitions ***************************//

struct BucketID{
        int id;

        BucketID():id(0){}
};

/**
 * This is a simple range structure over the order-codes of the members of a level
 * @author: Nikos Karayannidis
 */
struct LevelRange {
	// Note: the condition to check for a null range is: ?(leftEnd==rightEnd)
	static const int NULL_RANGE;
	string dimName;
	string lvlName;
	unsigned int leftEnd;
	unsigned int rightEnd;
	//LevelRange() : NULL_RANGE(0) {}
	LevelRange(){}
	LevelRange(const string& dn, const string& ln, unsigned int le, unsigned int re):
		dimName(dn),lvlName(ln),leftEnd(le),rightEnd(re){}	
	/**
	 * assignment operator, because the compiler complains that it cant use
	 * the default, due to the const member
	 */
	LevelRange const& operator=(LevelRange const& other);
	/**
	 * copy constructor
	 */
	 LevelRange::LevelRange(const LevelRange& l);
};

/**
 * This is a simple coordinates structure
 * @author: Nikos Karayannidis
 */
struct Coordinates {
	int numCoords; //number of coordinates
	vector<int> cVect;
	/**
	 * default constructor
	 */
	Coordinates() : numCoords(0), cVect(){}

	/**
	 * constructor
	 */
	Coordinates(int nc, vector<int>& cds) : numCoords(nc), cVect(cds){}

	/**
	 * copy constructor
	 */
	Coordinates(Coordinates const& c) : numCoords(c.numCoords), cVect(c.cVect) {}

	/**
	 * It fills in another Coordinates struct the coordinates that this->cVect contains
	 * excluding the pseudo coordinates (i.e. coordinates that correspond to pseudo-levels)
	 *
	 * @param coords        the initially empty coord struct that will be filled with the coordinate values
	 */
        void excludePseudoCoords(Coordinates & coords)const;
};

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
	void extractCoords(Coordinates& c) const;

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
	 * Vector with order-code ranges on each dimension
	 */
	vector<LevelRange> vectRange;

	/**
	 * default constructor
	 */
	ChunkHeader() {}
	/**
	 * copy constructor
	 */
	ChunkHeader(const ChunkHeader& h)
		: id(h.id), depth(h.depth), totNumCells(h.totNumCells),
		  rlNumCells(h.rlNumCells), vectRange(h.vectRange) {}
	~ChunkHeader() {}

        /**
	 * This method receives an empty vector of level ranges and fills it with the ranges
	 * included in this->vectRange, in the same order,  WITHOUT the NULL ranges.
	 *
	 * @param newvectRange	the initially empty vector of ranges that will be filled
	 */
	void excludeNULLRanges(vector<LevelRange>& newvectRange)const;

};  // end of struct ChunkHeader

/**
 * This is a single entry in a Directory Chunk.
 * @author: Nikos Karayannidis
 */
struct DirEntry {
	/**
	 * This is the bucket id that the pointed to chunk resides
	 */
	BucketID bcktId;
	/**
	 * This is the vector index, that we can use in order to access
	 * the pointed to chunk, through the vector of chunks of each bucket.
	 */
	unsigned int chnkIndex;

	/**
	 * default constructor
	 */
	DirEntry() : bcktId(), chnkIndex(0) {}
	/**
	 * constructor
	 */
	DirEntry(BucketID const & b, unsigned int i) : bcktId(b), chnkIndex(i) {}
	/**
	 * copy constructor
	 */
	DirEntry(DirEntry const& e) : bcktId(e.bcktId), chnkIndex(e.chnkIndex) {}
};

typedef float measure_t;
/**
 * This is a single entry in a Data Chunk.
 * @author: nikos Karayannidis
 */
struct DataEntry {
	/**
	 * Number of facts in a cell
	 */
	unsigned int numFacts;

	/**
	 * Vector of facts. In the current implementation only floats are supported.
	 */
	vector<measure_t> fact;

	/**
	 * default constructor
	 */
	DataEntry() {}
	/**
	 * constructor
	 */
	DataEntry(unsigned int n, vector<measure_t> const & f) : numFacts(n), fact(f) {}
	/**
	 * copy constructor
	 */
	DataEntry(DataEntry const & e) : numFacts(e.numFacts), fact(e.fact) {}
};


class Chunk {
public:
	/**
	 * this is the depth at the root chunk level
	 */
	static const unsigned int MIN_DEPTH = 0;
	/**
	 * Chunk default constructor
	 */
	Chunk(){}

	/**
	 * Chunk constructor
	 */
	Chunk(const ChunkHeader& h) : hdr(h){}

	/**
	 * Copy constructor
	 */
	Chunk(const Chunk & c) : hdr(c.gethdr()) {}

	/**
	 * Chunk destructor
	 */
	virtual ~Chunk(){}
	/** get/set */
	const ChunkHeader& gethdr() const {return hdr;}
	void sethdr(const ChunkHeader& h) {hdr = h;}

protected:
	/**
	 * The chunk header.
	 */
	ChunkHeader hdr;
}; //end of class Chunk

/**
 * This is a directory chunk
 * @author: Nikos Karayannidis
 */
class DirChunk : public Chunk {
public:
	/**
	 * default constructor
	 */
	DirChunk(){}
	/**
	 * constructor
	 */
	DirChunk(const ChunkHeader& h, const vector<DirEntry>& ent) : Chunk(h), entry(ent) {}
	/**
	 * copy constructor
	 */
	DirChunk(const DirChunk& c) : Chunk(c.gethdr()), entry(c.getentry()) {}

	~DirChunk() {}

	/**
	 * static method for calculating the cell offset within a DirChunk. Returns the offset
	 * in the range [0..totNumCells-1]
	 *
	 * @param coords	input set of coordinates
	 * @param hdr		chunk header of the dir chunk in question
	 */
	static unsigned int calcCellOffset(const Coordinates& coords, const ChunkHeader& hdr);

	//get/set
	const vector<DirEntry>& getentry() const {return entry;}
	void setentry(const vector<DirEntry>& e) { entry = e; }

private:
	/**
	 * vector of directory entries. Num of entries == totNumCells
	 */
	vector<DirEntry> entry;
}; // end of class DirChunk

/**
 * This is a data chunk
 * @author: Nikos Karayannidis
 */
class DataChunk : public Chunk {
public:
	/**
	 * default constructor
	 */
	DataChunk() {}

	/**
	 * constructor
	 */
	DataChunk(const ChunkHeader& h, const vector<DataEntry>& ent, const bit_vector& bmap)
		: Chunk(h), comprBmp(bmap),entry(ent) {}
	/**
	 * copy constructor
	 */
	DataChunk(const DataChunk& c)
		: Chunk(c.gethdr()), comprBmp(c.getcomprBmp()), entry(c.getentry()) {}

	~DataChunk(){}
	/**
	 * virtual method that returns the offset (i.e. index in the chunk vector of entries)
         * in order to access the cell.
	 */
	unsigned int calcCellOffset(const Coordinates& coords);

       	/**
	 * Static method for calculating the cell offset within a DataChunk. Returns the offset
	 * in the range [0..realNumCells-1]
	 *
	 * @param coords	input set of coordinates
	 * @param bmp		input compression bitmap
	 * @param hdr		the chunk header of the data chunk in question
	 * @param isEmpty	returned flag. Set on when requested cell is empty (i.e.,0 bit in bitmap)
	 */
	static unsigned int calcCellOffset(const Coordinates& coords, const bit_vector& bmp, const ChunkHeader& hdr, bool& isEmpty);

	//get/set
	const bit_vector& getcomprBmp() const { return comprBmp;}
	void setcomprBmp(const bit_vector &  bmp) {comprBmp = bmp;}
	const vector<DataEntry>& getentry() const {return entry;}
	void setentry(const vector<DataEntry>& e) { entry = e; }

private:
	/**
	 * The compression bitmap is used in order to avoid
	 * a full allocation of the cells of a data chunk and still
	 * be able to compute the offset efficiently.
	 */
	bit_vector comprBmp;

	/**
	 * vector of data entries. Num of entries != totNumCells (== num of NOT NULL cells)
	 */
	vector<DataEntry> entry;
}; // end of class DataChunk


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

	static void countDirChunksOfTree(CostNode* costRoot, unsigned int maxdepth, unsigned int& total);
	static  void countDataChunksOfTree(CostNode* costRoot, unsigned int maxdepth, unsigned int& total);
	static void countChunksOfTree(CostNode* costRoot, unsigned int maxdepth, unsigned int& total);

        static void printTree(CostNode* root, ofstream& out);
	/**
	 * get/set
	 */
	const ChunkHeader* const getchunkHdrp() const {return chunkHdrp;}
	void setchunkHdrp(ChunkHeader* const chdr) {chunkHdrp = chdr;}

	const CellMap* const getcMapp() const {return cMapp;}
	void setcMapp(CellMap* const map) {cMapp = map;}

	vector<CostNode*>& getchild() const {return child;}
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


const int LevelRange::NULL_RANGE=-1;

class Level_Member{
public:
	enum {PSEUDO_CODE = -1};
};


//************************* main program ************************************//
main(int argc, char* argv[])
{
        string dimFile(argv[1]);
        string factFile(argv[2]);

        //create a cost node tree
        CubeInfo cbinfo(string("lalacube"));
        CostNode* costRoot = createCostTree(dimFile,factFile, cbinfo);

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


        //call descendBreadth1stCostTree
        // 1. In order to create a Bucket instance, we need a vector of DirChunks, a vector of DataChunks
	vector<DirChunk>* dirVectp = new vector<DirChunk>;
	unsigned int numDirChunks = 0;
	CostNode::countDirChunksOfTree(costRoot, cbinfo.getmaxDepth(), numDirChunks);
	dirVectp->reserve(numDirChunks);

	vector<DataChunk>* dataVectp = new vector<DataChunk>;
	unsigned int numDataChunks = 0;
	CostNode::countDataChunksOfTree(costRoot, cbinfo.getmaxDepth(), numDataChunks);
	dataVectp->reserve(numDataChunks);

	// 1.1. Create a BucketID for the  Bucket.

	// 2. Fill the chunk vectors
	// 2.1 Descend the tree (depth-first) and insert each chunk in the corresponding vector
	//    as you first meet it, i.e. the root will be stored at position begin() of the vector.
	BucketID bcktID;
	queue<CostNode*> nextToVisit; // breadth-1st queue
	try {
		descendBreadth1stCostTree(cbinfo,costRoot,factFile,bcktID,nextToVisit,dirVectp,dataVectp);
	}
      	catch(const char* message) {
      		string msg("");
      		msg += message;
      		cerr<< msg;
      	}
      	//ASSERTION2: valid returned vectors
       	if(dirVectp->empty() && dataVectp->empty()){
       			cerr<< "AccessManager::storeTreeInBucket  ==>ASSERTION2 empty both chunk vectors!!\n";
       	}
       	else if (dataVectp->empty()){
       			cerr<< "AccessManager::storeTreeInBucket  ==>ASSERTION2 empty data chunk vector!!\n";
	}

        //output the resulting vectors with the chunks
        outputVectors(dirVectp, dataVectp);
} // end main

void outputVectors(vector<DirChunk>* dirVectp, vector<DataChunk>* dataVectp)
{
	ofstream out("output.text");

        if (!out)
        {
            cerr << "creating file \"output.text\" failed\n";
        }

        //initialize the chunk slot indicator
        unsigned int chunk_slot = 0;

        //for each dirchunk in the vector
        for(vector<DirChunk>::const_iterator diri = dirVectp->begin();
                diri != dirVectp->end(); diri++){
                //print the chunk id and the chunk slot
                cout<<diri->gethdr().id.getcid()<<", chunk slot: "<<chunk_slot<<endl;
                out<<diri->gethdr().id.getcid()<<", chunk slot: "<<chunk_slot<<endl;
                //print entries
                out<<"\tENTRIES:\n";
                //for each entry
                for(vector<DirEntry>::const_iterator ent_i = diri->getentry().begin();
                        ent_i != diri->getentry().end(); ent_i++){
                        out<<"\t"<<ent_i->chnkIndex<<", ";
                }//end for
                out<<endl;
                chunk_slot++;
        }//end for

        //for each datachunk in the vector
        for(vector<DataChunk>::const_iterator datai = dataVectp->begin();
                datai != dataVectp->end(); datai++){
                //print the chunk id and the chunk slot
                cout<<datai->gethdr().id.getcid()<<", chunk slot: "<<chunk_slot<<endl;
                out<<datai->gethdr().id.getcid()<<", chunk slot: "<<chunk_slot<<endl;
                //print entries
                out<<"\tENTRIES:\n";
                //for each entry
                for(vector<DataEntry>::const_iterator ent_i = datai->getentry().begin();
                        ent_i != datai->getentry().end(); ent_i++){
                        out<<"\t"<<ent_i->fact[0]<<"/"<<ent_i->fact[1]<<", ";
                }//end for
                out<<endl;
                //print bitmap
                out<<"\tBITMAP:\n";
                // for each bit
                out<<"\t";
                for(bit_vector::const_iterator bit_i = datai->getcomprBmp().begin();
                        bit_i != datai->getcomprBmp().end(); bit_i++){
                        out<<*bit_i<<" ";
                }//end for
                out<<endl;

                chunk_slot++;
        }//end for

}// end outputVectors

CostNode* createCostTree(string& dimFile, string& factFile, CubeInfo& cbinfo){
        CostNode* root = 0;
        //Test case 1 (chunk subtree in figure 5 in SISYPHUS TR)
        //testCase1(cbinfo, root);

        //Test case 2 (whole chunk tree in figure 5 in SISYPHUS TR)
        testCase2(cbinfo, root);

        return root;
}

// create the costNode tree for test-case 2(whole chunk tree in figure 5 in SISYPHUS TR). Updates
// accordingly cbinfo and root
void testCase2(CubeInfo& cbinfo, CostNode* &root)
{
        // creates the cost node tree corresponding to Fig. 5 of SISYPHUS TR
        cbinfo.setmaxDepth(3);
        cbinfo.setnumFacts(2);

        //create "root chunk"
        //create chunk header
        ChunkHeader* hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("root"));
        //      depth
        hdrp->depth = Chunk::MIN_DEPTH;
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
        testCase1(cbinfo, child);
        root->getchild().push_back(child);
}//end testCase2


// create the costNode tree for test-case 1(chunk subtree in figure 5 in SISYPHUS TR). Updates
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
        hdrp->depth = 1;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 4;
        //      level ranges
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
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
        //      level ranges
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("region"),0,1));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("Pseudo_level"),LevelRange::NULL_RANGE,LevelRange::NULL_RANGE));


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
        hdrp->depth = 3;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 1;
        //      level ranges
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),0,0));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"),0,1));


        //create cell map
        clmpp = new CellMap;
        //clmpp->insert(string("0|0.0|0.0|-1.0|0"));
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
        hdrp->depth = 3;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 4;
        //      level ranges
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),1,2));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"),0,1));


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
        hdrp->depth = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
        //      level ranges
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("region"),0,1));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("Pseudo_level"),LevelRange::NULL_RANGE,LevelRange::NULL_RANGE));


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
        hdrp->depth = 3;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
        //      level ranges
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),0,0));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"),2,3));


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
        hdrp->depth = 3;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 4;
        //      level ranges
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
        father->getchild().push_back(new CostNode(hdrp, clmpp));


        // create node 0|0.1|0

        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|0"));
        //      depth
        hdrp->depth = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
        //      level ranges
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("region"),2,3));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("Pseudo_level"),LevelRange::NULL_RANGE,LevelRange::NULL_RANGE));


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
        hdrp->depth = 3;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
        //      level ranges
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),3,3));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"),0,1));


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
        hdrp->depth = 3;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 4;
        //      level ranges
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
        father->getchild().push_back(new CostNode(hdrp, clmpp));


        // create node 0|0.1|1
        //create chunk header
        hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.1|1"));
        //      depth
        hdrp->depth = 2;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
        //      level ranges
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("region"),2,3));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("Pseudo_level"),LevelRange::NULL_RANGE,LevelRange::NULL_RANGE));


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
        hdrp->depth = 3;
        //      total number of cells
        hdrp->totNumCells = 2;
        //      real num of cells
        hdrp->rlNumCells = 2;
        //      level ranges
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("city"),3,3));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("item"),2,3));


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
        hdrp->depth = 3;
        //      total number of cells
        hdrp->totNumCells = 4;
        //      real num of cells
        hdrp->rlNumCells = 2;
        //      level ranges
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
        father->getchild().push_back(new CostNode(hdrp, clmpp));

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
