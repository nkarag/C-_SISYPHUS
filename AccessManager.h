/***************************************************************************
                          AccessManager.h  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


#ifndef ACCESS_MANAGER_H
#define ACCESS_MANAGER_H


//#include <vector>
//#include <map>
//#include <queue>
//#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <climits>

//#include "Cube.h"
//#include "definitions.h"
//#include "Chunk.h"
#include "StdinThread.h"
#include "definitions.h"

typedef char*   cmd_err_t;

//class CubeInfo; //fwd declarations
//struct BucketID;
//class CostNode;
//struct DirEntry;
//class DirChunk;
class AccessManagerImpl;

/**
 * The AccessManager class parses user commands
 * and serves them by calling appropriate methods.
 * The implementation of ths class is hidden inside the data member of type
 * class AccessManagerImpl (see Effective C++ item 34)
 *
 * @see Shore grid example command_server_t (command_server.h/C) and command_base_t (command.h/C)
 * @author: Nikos Karayannidis
 */
class AccessManager {
public:
//________________________________ TYPEDEFS ____________________________________________
	/**
	 * Enumeration to denote the alternative cost tree traversal methods and respective
	 * storage alternatives of chunks into a single bucket. Possible values correspond
	 * to a depth first traversal and a breadth first traversal.
	 */        				 	
        typedef enum {depthFirst,breadthFirst} treeTraversal_t ;

	/**
	 * This enumeration holds the identifiers (tokens) of the different clustering algorithms
	 * supported by Sisyphus on CUBE File construction
	 */
	typedef enum {
		simple //simple clustering algortihm,
		//cpt, // clustering algorithm that respects the Current Point in Time (CPT)
	} clustAlgToken_t ;


	/**
	 * This enumeration holds the identifiers (tokens) of the different methods for resolving
	 * the storage of a large chunk that does not fit in a single bucket,
	 * supported by Sisyphus on CUBE File construction
	 */
	typedef enum {
		large_bucket, //simple method of using a large bucket for storing the whole large chunk
		equigrid_equichildren //the same number of partitions (i.e., new members in the artificial level)
				    //for all dimensions (equi-grid),and the same number of children at the next
				    //level for each new member (equi-children)
		//adjustgrid_equichildren // grid partitions are adjusted to the chunk's data distribution.
	} largeChunkMethodToken_t ;
	
	/**
	 * This enumeration holds the identifiers (tokens) of the different methods for storing the
	 * root directory of the CUBE File structure supported by Sisyphus on CUBE File construction.
	 */
	typedef enum {
		singleBucketDepthFirst, //method of using a bucket of special size ("root bucket") for storing the whole
					//root directory in a depth first manner
		singleBucketBreadthFirst //method of using a bucket of special size ("root bucket") for storing the whole
					//root directory in a breadth first manner
	} rootDirectoryStorage_t ;
					
//________________________________ CLASS/STRUCT DEFINITIONS  ____________________________________________
	/**
	 * This structure holds the CUBE File construction parameters
	 */
	class CBFileConstructionParams {
	public:
		/**
		 * The token of the clustering algorithm used for forming bucket regions
		 */			
		clustAlgToken_t clustering_algorithm;
		
		/**
		 * The token of the traversal method used for storing the nodes of a chunk-tree in a DiskBucket
		 */							
		treeTraversal_t how_to_traverse;        	
		
		/**
		 * The token of the method used for storing large chunks that cannot fit in a single DiskBucket
		 */								
		largeChunkMethodToken_t large_chunk_resolution;
		
		/**
		 * The token of the method used for storing the root directory of the CUBE File structure
		 */										
		rootDirectoryStorage_t rootDirectoryStorage;
		
		/**
		 * Memory constraint for storing the root directory during quering
		 */
		 memSize_t rootDirMemConstraint;
		
		 /**
		  * The percent of extra space allocated in buckets in order to anticipate future
		  * appendings in the bucket body. This is especially used for the root bucket, in
		  * methods that exploit a dynamic allocated bucket size and not a fixed length one.
		  * This percent is w.r.t the actual size of he bucket. The value is typically in the range
		  * [0,1]
		  */
		  float prcntExtraSpace;
		
		/**
		 * The default constructor initializes parameters with default values.
		 */
		CBFileConstructionParams():clustering_algorithm(simple),
					   how_to_traverse(breadthFirst),
					   large_chunk_resolution(equigrid_equichildren),
					   rootDirectoryStorage(singleBucketBreadthFirst),
					   rootDirMemConstraint(ULONG_MAX),
					   prcntExtraSpace(0) //no extra space by default
					   {}
			
		~CBFileConstructionParams(){}
		
		/**
		 * Assignment operator
		 */
                CBFileConstructionParams & operator=(const CBFileConstructionParams & other)
                {
                	if(this != &other) {
				clustering_algorithm = other.clustering_algorithm;
				how_to_traverse = other.how_to_traverse;
				large_chunk_resolution = other.large_chunk_resolution;
				rootDirectoryStorage = other.rootDirectoryStorage;
				rootDirMemConstraint = other.rootDirMemConstraint;
                	}// end if
                	return (*this);
                }//CBFileConstructionParams::operator=()		
		
		/**
		 * Initialize construction parameters from a configuration file
		 */
		initParamsFromFile(const ifstream& configInput) {}
					
	}; //end struct CBFileConstructionParams

//___________________________________ Public INTERFACE _____________________________________________
	/**
	 * AccessManager constructor
	 */
	AccessManager(ostream& out = cout, ofstream& error = StdinThread::errorStream);

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
    	void parseCommand(char* line, bool& quit) const ;

	/**
	 * Method for serving the create_cube command.
	 * Main tasks are:
	 *			- create shore file.
	 *			- update catalog.
	 * Return 0 on success, and a message on failure.
	 *
	 * @param name	The cube name.
	 */
	 cmd_err_t create_cube (string& name) const ;

	/**
	 * Method for serving the drop_cube command.
	 * Main tasks are:
	 *			- delete shore file.
	 *			- update catalog.
	 * Return 0 on success, and a message on failure.
	 *
	 * @param name	The cube name.
	 */
	 cmd_err_t drop_cube (string& name) const ;

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
	 * @param configFile	The file with CUBE File construction parameters	
	 */
	 cmd_err_t load_cube (const string& name, const string& dimFile, const string& factFile, const string& configFile) const ;

	/**
	 * Method for serving the print_cube command.
	 * Main tasks are:
	 *			- pin shore record into memory.
	 *			- print record values.
	 * Return 0 on success, and a message on failure.
	 *
	 * @param name	The cube name.
	 */
	 cmd_err_t print_cube (string& name) const ;
	
	 /**
	  * This function returns true only if the input values correspond to a data chunk
	  *
	  * @param	depth	the global depth (input parameter)
	  * @param	local_depth	the local depth (input parameter)	
	  * @param	next_flag	next flag in chunk header (input parameter)
	  * @param max_depth	the maximum chunking depth (input parameter)
	  */
	  static bool isDataChunk(int depth, int local_depth, bool next_flag, int max_depth);
	
	 /**
	  * This function returns true only if the input values correspond to a dir chunk
	  *
	  * @param	depth	the global depth (input parameter)
	  * @param	local_depth	the local depth (input parameter)	
	  * @param	next_flag	next flag in chunk header (input parameter)
	  * @param max_depth	the maximum chunking depth (input parameter)
	  */
	  static bool isDirChunk(int depth, int local_depth, bool next_flag, int max_depth);
	
	 /**
	  * This function returns true only if the input values correspond to a root chunk
	  *
	  * @param	depth	the global depth (input parameter)
	  * @param	local_depth	the local depth (input parameter)	
	  * @param	next_flag	next flag in chunk header (input parameter)
	  * @param max_depth	the maximum chunking depth (input parameter)
	  */
	  static bool isRootChunk(int depth, int local_depth, bool next_flag, int max_depth);
	
	 /**
	  * This function checks whether the size of a chunk (dir or data) exceeds the
	  * free space of a bucket. In this case the chunk can be characterized as a large
	  * chunk and the function returns true, otherwise returns false.
	  *
	  * @param size the input size
	  */	  	
	 static bool isLargeChunk(size_t size);
	
	 static bool isArtificialChunk(int local_depth);

private:
	/**
	 * Pointer to the AccessManager implementation. (Technique for separating the public interface of a class
	 * with its implementation, see Effective C++ Item 34)
	 */
	AccessManagerImpl* accMgrImpl;
	
	/**
	 * Copy constructor is put in the private section and will stay UNDEFINED
	 * in order to prevent its use! (see Effective C++ item 27)
	 */
	 AccessManager(const AccessManager& am);
	
	/**
	 * Assignment operator is put in the private section and will stay UNDEFINED
	 * in order to prevent its use! (see Effective C++ item 27)
	 */
	AccessManager& operator=(AccessManager const& other);
			
};//class AccessManager

#endif // ACCESS_MANAGER_H
