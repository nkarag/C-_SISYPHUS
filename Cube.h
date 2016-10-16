/***************************************************************************
                          Cube.h  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


#ifndef CUBE_H
#define CUBE_H

#include <sm_vas.h>
#include <string>
#include <vector>
#include <iterator>
//#include <ifstream>
#include <iostream>
#include <iomanip>

#include "Bucket.h"
#include "DiskStructures.h"
#include "AccessManager.h"

typedef int cubeID_t;  // NOTE: The type of the cube id reflects to the type of the keys of the catalog!!!
		       //       See the constructor of CatalogManager

//typedef Basic_substring<char> substring; //substring of characters: type used in various functions in the code

/**
 * Class to encapsulate a File id
 */
class FileID {
private:
	/**
	 * Shore serial_t type, id of shore file
	 */
	serial_t shoreID;
public:
	FileID() { shoreID = serial_t::null; }
	FileID(serial_t& fid) { shoreID = fid; }

	~FileID() { }

	const serial_t& get_shoreID() const { return shoreID; }
	void set_shoreID(const serial_t& fid) { shoreID = fid; }
};

/**
 * Class to hold information about a member of a level
 */
class LevelMember {
public:
	static const short PSEUDO_CODE = -1;
	//enum {PSEUDO_CODE = -1};
	
	LevelMember() { }

	~LevelMember() { }

	// get/set name
	const string& get_name() const { return name; }
	void set_name(const string& nm) { name = nm; }

	// get/set order_code
	const DiskChunkHeader::ordercode_t get_order_code() const { return order_code; }
	void set_order_code(const DiskChunkHeader::ordercode_t ocode) { order_code = ocode; }

	// get/set member_code
	const string& get_member_code() const { return member_code; }
	void set_member_code(const string& mcode) { member_code = mcode; }

	// get/set parent_member_code
	const string& get_parent_member_code() const { return parent_member_code; }
	void set_parent_member_code(const string& pmcode) { parent_member_code = pmcode; }

	// get/set first_child_order_code
	const DiskChunkHeader::ordercode_t get_first_child_order_code() const { return first_child_order_code; }
	void set_first_child_order_code(const DiskChunkHeader::ordercode_t fcocode) { first_child_order_code = fcocode; }

	// get/set last_child_order_code
	const DiskChunkHeader::ordercode_t get_last_child_order_code() const { return last_child_order_code; }
	void set_last_child_order_code(const DiskChunkHeader::ordercode_t lcocode) { last_child_order_code = lcocode; }
	
private:
	string name; //name of member
	DiskChunkHeader::ordercode_t order_code; //order code in the level e.g. 2
	string member_code; //member code in the dimension e.g. 1.4.2
	string parent_member_code; //member code of the parent e.g 1.4
	DiskChunkHeader::ordercode_t first_child_order_code; //order code of first child e.g. 5 (the child will have member code 1.4.2.5)
	DiskChunkHeader::ordercode_t last_child_order_code; //order code of last child e.g. 9 (the child will have member code 1.4.2.9)

};

/**
 * Class to hold information about a level of a dimension
 */
class Dimension_Level {

private:

	string name; //name of level
	int level_number; //level number in the dimension:0, 1, ...
	int num_of_members; //number of members of the level
	vector<LevelMember> vectMember; //vector of LevelMember objects

public:

	Dimension_Level() { }

	~Dimension_Level() { }

	/**
	 * Returns the number of members under the same father
	 * e.g. give me how many members are under parent 0.1
	 *
	 * @param	parentMbrCode	the member code of the parent member
	 */
	unsigned int get_num_of_sibling_members(string parentMbrCode);

	/**
	 * Searches for a member in this level by its member-code
	 *
	 * @param	mbCode 	the member-code search key
 	 */
	vector<LevelMember>::const_iterator getMbrByMemberCode(string& mbCode);

	// get/set name
	const string& get_name() const { return name; }
	void set_name(const string& nm) { name = nm; }

	// get/set level_number
	const int& get_level_number() const { return level_number; }
	void set_level_number(const int& num) { level_number = num; }

	// get/set num_of_members
	const int& get_num_of_members() const { return num_of_members; }
	void set_num_of_members(const int& num) { num_of_members = num; }
	// increase num_of_members
	void increase_num_of_members() { num_of_members++; }

	// get vectMember
	vector<LevelMember>& get_vectMember() { return vectMember; }
};

/**
 * Class to hold information about a dimension
 */
class Dimension {

private:

	string name; //name of dimension
	int num_of_levels; //num of levels of dimension
	/**
	 * vector of Dimension_Level objects. At position begin() we
	 * store the more aggregated level, while at end()-1 the grain level.
	 */
	vector<Dimension_Level> vectLevel;

public:

	Dimension() { }

	~Dimension() { }

	// get/set name
	const string& get_name() const { return name; }
	void set_name(const string& nm) { name = nm; }

	// get/set num_of_levels
	int get_num_of_levels() const { return num_of_levels; }
	void set_num_of_levels(const int& num) { num_of_levels = num; }
	// increase num_of_levels
	void increase_num_of_levels() { num_of_levels++; }

	// get vectLevel
	vector<Dimension_Level>& get_vectLevel()  { return vectLevel; }
};

/**
 * Class to hold information about the cube
 */
class CubeInfo {

public:
//constants
static const cubeID_t null_id = -1000; // the null cube id
				       // **NOTE** null_id must be != from CatalogManager::MAXKEY !!!
private:
	/**
	 * The CUBE File construction parameters used for building this cube
	 */
	typename AccessManager::CBFileConstructionParams constructParams;

	/**
	 * ID of the file of the cube
	 */
	FileID fid;
	/**
	 * Name of the cube
	 */
	string name;

	/**
	 * Catalog internal cube id.
	 */
	cubeID_t cbID;

	/**
	 * Dimensions vector containing information about the dimensions of the cube.
	 * ***NOTE*** In the current version the order in which the dimensions appear in
	 * this vector, defines the "interleaving order" used for the chunk id construction.
	 * I.e., (dim1,dim2,dim3) in the vectDim means that we will have chunk ids of the
	 * form: dim1|dim2|dim3.dim1|dim2|dim3...
	 * Also note that this order is indirectly specified through the order that the dimensions
	 * appear in the input file *.dld (i.e., the file with the dimension data).
	 */
	vector<Dimension> vectDim;

	/**
	 * Number of dimensions
	 */
	int num_of_dimensions;

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

	/**
	 * The bucket ID where the root chunk is stored.
	 */
	BucketID rootBucketID;
	//serial_t rootBucket;
	
	/**
	 * The root chunk is always at offset 0 in the root bucket.
	 */
	const unsigned int rootChnkIndex;
	

	///////////// PRIVATE METHODS //////////////
	/**
	 * Inserts pseudo levels so as all dimensions to have the same number of levels
	 */
	void Insert_pseudo_levels();

	/**
	 * Creates the member codes of all members of the dimensions
	 */
	void Create_member_codes();

	/**
	 * Shows the children of a member on the screen
	 */
	void Print_children(vector<Dimension_Level>::iterator iter, const int levels, const int first, const int last);

public:
	/**
	 * CubeInfo constructor.
	 */
	CubeInfo();

	/**
	 * CubeInfo constructor.
	 *
	 * @param name	the name of the cube.
	 */
	CubeInfo(const string& name);

	/**
	 * CubeInfo destructor
	 */
	~CubeInfo();

	// get/set
	const FileID& get_fid() const {return fid; }
	void set_fid(const FileID& id) { fid = id; }

	const string& get_name() const { return name; }
	void set_name(const string& n) { name = n; }

	const cubeID_t get_cbID() const { return cbID; }
	void set_cbID(const cubeID_t cid) { cbID = cid; }

	// get/set num_of_dimensions
	const int get_num_of_dimensions() const { return num_of_dimensions; }
	void set_num_of_dimensions(const int& num) { num_of_dimensions = num; }
	// increase num_of_dimensions
	void increase_num_of_dimensions() { num_of_dimensions++; }

	// get/set vectDim
	const vector<Dimension>& getvectDim() const { return vectDim; }
	void setvectDim(const vector<Dimension>&  dim) { vectDim = dim; }

	const vector<string>& getfactNames() const {return factNames;}
	void setfactNames(const vector<string>& svect) {factNames = svect;}
	
	unsigned int getnumFacts() const {return numFacts;}
	void setnumFacts(unsigned int n) {numFacts = n;}

	const unsigned int getmaxDepth() const {return maxDepth;}
	void setmaxDepth(unsigned int d) {maxDepth = d;}

	// get/set rootBucket
	const BucketID& get_rootBucketID() const { return rootBucketID; }
	void set_rootBucketID(const BucketID& bucketid) { rootBucketID = bucketid; }
	
	// get rootChnkIndex
	const unsigned int get_rootChnkIndex() const {return rootChnkIndex;}
	
	typename AccessManager::CBFileConstructionParams const& getconstructParams() const {return constructParams;}
	void setconstructParams(AccessManager::CBFileConstructionParams const & cp) {constructParams = cp;}

	/**
	 * Gets information about the dimensions of the cube from a text file
	 */
	void Get_dimension_information(const string& filename);

	/**
	 * Gets information about the facts of the cube from a text file
	 */
	void getFactInfo(const string& filename);


	/**
	 * Shows information of the dimensions on the screen
	 */
	void Show_dimensions();
};//end class CubeInfo

struct BucketID; //fwd declarations
class Bucket;
class ChunkID;
class Chunk;
class DirChunk;
class Cell;

/**
 * This class represents the basic entity in our data model: a cube!
 *
 * @see: Shore grid example: grid_t (at grid.h)
 * @author: Nikos Karayannidis
 */
class Cube  {

private:
	/**
	 * Information about the cube
	 */
	CubeInfo* info;
	/**
	 * Information about the dimensions of the cube
	 */
	//Dimension* dimensions_info;

	/**
	 * in-memory pointer to root chunk
	 */
	DirChunk* rootChnkp;
	/**
	 * The bucket ID of the current bucket
	 */
	BucketID* currBcktId;
	/**
	 * An in-memory pointer to the current bucket.
	 */
	Bucket* currBcktp;
	/**
	 * The offset of the current chunk within its bucket.
	 */
	unsigned int chnkOffs;
	/**
	 * An in-memory pointer to the current chunk.
	 */
	Chunk* currChnkp;
	/**
	 * The current chunk's chunk-id
	 */
	ChunkID* currChnkId;
	/**
	 * An in-memory pointer to the current cell
	 */
	Cell* currCellp;
	/**
	 * The current cell's coordinates
	 */
	//Coordinates coords;
public:
	/**
	 * Constructor of the cube
	 */
	Cube();
	/**
	 * Destructor of the cube
	 */
	~Cube();
};

#endif // CUBE_H
