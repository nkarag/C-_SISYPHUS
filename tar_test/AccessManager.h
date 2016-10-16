/***************************************************************************
                          AccessManager.h  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


#ifndef ACCESS_MANAGER_H
#define ACCESS_MANAGER_H

#include <string>
#include <vector>

//#include "Cube.h"
#include "definitions.h"

typedef char*   cmd_err_t;

/**
 * The AccessManager class parses user commands
 * and serves them by calling appropriate methods.
 *
 * @see Shore grid example command_server_t (command_server.h/C) and command_base_t (command.h/C)
 * @author: Nikos Karayannidis
 */
class CubeInfo; //fwd declarations
struct BucketID;
class CostNode;
struct DirEntry;
class DirChunk;

class AccessManager {

public:
	/**
	 * AccessManager constructor
	 */
	AccessManager();

	/**
	 * AccessManager destructor
	 */
	~AccessManager();

	/**
    	 * Parses a coomand. If command is valid call appropriate method to serve it.
    	 * Print errors to stderr.
    	 * Sets "quit" to true if quit command was found
	 *
	 * @param line	A line of input.
	 * @param quit  this parameter is set to true if the quit command is issued
	 *
	 * @see command_base_t::parse_command (command.C)
	 */
    	void parseCommand(char* line, bool& quit);

	/**
	 * Method for serving the create_cube command.
	 * Main tasks are:
	 *			- create shore file.
	 *			- update catalog.
	 * Return 0 on success, and a message on failure.
	 *
	 * @param name	The cube name.
	 */
	 cmd_err_t create_cube (string& name);

	/**
	 * Method for serving the drop_cube command.
	 * Main tasks are:
	 *			- delete shore file.
	 *			- update catalog.
	 * Return 0 on success, and a message on failure.
	 *
	 * @param name	The cube name.
	 */
	 cmd_err_t drop_cube (string& name);

	/**
	 * Method for serving the load_cube command.
	 * Main tasks are:
	 *			- retrieve CubeInfo obj. from catalog
	 *			- read dimension schema + data from file and update CubeInfo obj
	 *			- read fact schema from file and update CubeInfo obj.
	 *			- call "constructCubeFile" which implements the CUBE FIle construction algorithm
	 *			- store updated CubeInfo obj back on disk
	 * Return 0 on success, and a message on failure.
	 *
	 * @param name	The cube name.
	 * @param dimFile	The file with the dimension schema and data.
	 * @param factFile	The file with fact schema and data
	 */
	 cmd_err_t load_cube (string& name, string& dimFile, string& factFile);

	/**
	 * Method for serving the print_cube command.
	 * Main tasks are:
	 *			- pin shore record into memory.
	 *			- print record values.
	 * Return 0 on success, and a message on failure.
	 *
	 * @param name	The cube name.
	 */
	 cmd_err_t print_cube (string& name);

private:
	/**
	 * This function implements the basic CUBE File construction
	 * algorithm, as it is described in the technical report.
	 *
	 * @param cinfo		all schema and system-related info about the cube
	 * @param factFile	file with fact values. Each fact value is associated with a chunk-id
	 */
	void constructCubeFile(CubeInfo& cinfo, string& factFile);

	/**
	 * This recursive function receives the CostNode tree and the file with the fact values
	 * and has all the logic on how to construct the Cube File by packing into buckets (ssm records) all the
	 * chunks. It is first called from AccessManager::constructCubeFile in order to put the entire chunk tree
	 * into buckets. However, whenever we find a chunk sub-tree that does not fit in a single bucket,
	 * then we recursively call it again.
	 *
 	 * @param cinfo		all schema and system-related info about the cube. In this CubeInfo input
 	 * 			argument we can find the Bucket id of the root-bucket.
	 * @param costRoot	the root of the cost sub-tree. 	
	 * @param where2store	this is an index (i.e.offset) showing where in the DirChunk vector of the
	 *			root bucket to store the chunk which corresponds to the costRoot.
	 *			NOTE: before a recursive call to putIntoBuckets, we reserve one more element
	 *			in the vector and the call putIntoBuckets
	 *			with where2store = rtDirDataVectp->capacity();
	 * @param factFile	file with fact values. Each fact value is associated with a chunk-id	
	 * @param rtDirDataVectp  This is a pointer to the DirChunk vector of the root bucket.
	 *			Each call to putIntoBucket inserts one more chunk in it,
	 *			at the where2store position.
	 * @param returnDirEntry This is a returned DirEntry containing BucketId and chunkIndex info for
	 *			the chunk for which the putIntoBucket was called.The caller must store this
	 *			info in the corresponding parent chunk cell.
	 */
	
	void putIntoBuckets(CubeInfo& cinfo,
				CostNode* costRoot,
				unsigned int where2store,
				string& factFile,
				vector<DirChunk>* rtDirDataVectp,
				DirEntry& returnDirEntry);	
				
	/**
	 * Function for testing phase I of CUBE File construction.
	 *
	 * @param costRoot	Root of the CostNode tree created after phase I.
	 * @param cinfo  	the corresponding cube information
	 */				
	 void test_construction_phaseI(CostNode* costRoot, CubeInfo& cinfo);
	
	/**
	 * Private method for creating the root chunk
	 * Main tasks are:
	 *			- create an array of cells(root chunk) in memory
	 *			- allocate a bucket in Cube file to store the root chunk
	 *			- save the root bucket ID in the CubeInfo object
	 * Rerurn pointer to the root chunk in memory
	 *
	 * @param info The CubeInfo object that stores information about the cube
	 * @author Loukas Sinos
	 */
	//DirChunk* Create_root_chunk(CubeInfo& info);

	//int directoryCost(CubeInfo& info, ChunkID& cID, string& datafile);
	//int* expandChunk(CubeInfo& info, ChunkID& cID, string& datafile);
	//bool check_chunkID(ChunkID& cID, string& datafile);
	/*void create_vector_of_chunk_ids(string& cID, //the chunk id of the chunk for which we want to create the cells' IDs
									string& suffix, //the suffix of the chunk id or part of the suffix
									string& newID, //the new chunk id for a cell or part of it
									vector<Dimension>::iterator iter_dim, //points to the current dimension
									vector<string>& vec); //stores the ids of the cells
	*/
};

class ChunkID; //fwd declaration
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
	//vector<ChunkID>& chunkidVect;
}; //end of class CellMap

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

#endif // ACCESS_MANAGER_H
