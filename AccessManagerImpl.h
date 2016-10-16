/***************************************************************************
                          AccessManagerImpl.h  -  description
                             -------------------
    begin                : Tue Feb 26 2002
    copyright            : (C) 2002 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/

#ifndef ACCESS_MANAGER_IMPL_H
#define ACCESS_MANAGER_IMPL_H

#include <vector>
#include <map>
#include <queue>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>


#include "AccessManager.h"
#include "StdinThread.h"
#include "Cube.h"
#include "definitions.h"
#include "Chunk.h"



class CubeInfo; //fwd declarations
struct BucketID;
class CostNode;
struct DirEntry;
class DirChunk;

/**
 * This is the implementation of the AccessManager class.
 * (see Effective C++ item 34)
 */
class AccessManagerImpl{
public:
	/**
	 * AccessManager constructor
	 */
	AccessManagerImpl(ostream& out = cerr, ofstream& error = StdinThread::errorStream)
		:outputLogStream(out), errorLogStream(error){					
		if(!errorLogStream)
			cerr<<"The error log file has not been opened appropriately\n";
	}

	/**
	 * AccessManager destructor
	 */
	~AccessManagerImpl() {}
	
	/**
	 * Access error log-file stream
	 */
	ofstream& geterrorLogStream() const {return errorLogStream;}
	
	/**
	 * Access error output stream
	 */
	ostream& getoutputLogStream() const {return outputLogStream;}

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
	 * @param configFile	The file with CUBE File construction parameters	
	 */
	 cmd_err_t load_cube (const string& name, const string& dimFile, const string& factFile, const string& configFile);

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
	
	 /**
	  * This function returns true only if the input values correspond to a data chunk
	  *
	  * @param	depth	the global depth (input parameter)
	  * @param	local_depth	the local depth (input parameter)	
	  * @param	next_flag	next flag in chunk header (input parameter)
	  * @param max_depth	the maximum chunking depth (input parameter)
	  */
	  static bool isDataChunk(int depth, int local_depth, bool next_flag, int max_depth){
               	if(depth !=  max_depth)
               		return false; //not a data chunk
               	else {//global depth == maxDepth
                        if( 	(local_depth == Chunk::NULL_DEPTH) ||
                               	(local_depth > Chunk::MIN_DEPTH && next_flag == false)
                        )
               	       		return true; //is a data chunk
			return false;               	       	
        	}//end else	
	  }//IsDataChunk()
	
	 /**
	  * This function returns true only if the input values correspond to a dir chunk
	  *
	  * @param	depth	the global depth (input parameter)
	  * @param	local_depth	the local depth (input parameter)	
	  * @param	next_flag	next flag in chunk header (input parameter)
	  * @param max_depth	the maximum chunking depth (input parameter)
	  */
	  static bool isDirChunk(int depth, int local_depth, bool next_flag, int max_depth){
               	if(depth >=  Chunk::MIN_DEPTH && depth < max_depth)
               		return true; //is a dir chunk
               	else if (depth < Chunk::MIN_DEPTH)
               		return false; // inappropriate value
               	else {//global depth == maxDepth
                        if( 	(local_depth == Chunk::NULL_DEPTH) ||
                               	(local_depth > Chunk::MIN_DEPTH && next_flag == false)
                        )
               	       		return false; //not a dir chunk but a data chunk
			return true;               	       	
        	}//end else	
	  }//IsDirChunk()
	
	 /**
	  * This function returns true only if the input values correspond to a root chunk
	  *
	  * @param	depth	the global depth (input parameter)
	  * @param	local_depth	the local depth (input parameter)	
	  * @param	next_flag	next flag in chunk header (input parameter)
	  * @param max_depth	the maximum chunking depth (input parameter)
	  */
	  static bool isRootChunk(int depth, int local_depth, bool next_flag, int max_depth){
               	if(depth !=  Chunk::MIN_DEPTH)
               		return false; //not a root chunk
               	else {//global depth == Chunk::MIN_DEPTH
                        if( 	(local_depth == Chunk::NULL_DEPTH) ||
                               	(local_depth == Chunk::MIN_DEPTH && next_flag == true)
                        )
               	       		return true;
			return false;               	       	
        	}//end else	
	  }//IsRootChunk()
	
	 /**
	  * This function checks whether the size of a chunk (dir or data) exceeds the
	  * free space of a bucket. In this case the chunk can be characterized as a large
	  * chunk and the function returns true, otherwise returns false.
	  *
	  * @param size the input size
	  */	  	
	 static bool isLargeChunk(size_t size) {
	 	// for storing a single chunk in a diskBucket we will need also one entry
	 	// in the internal bucket directory
	 	return (
	 		size > (DiskBucket::bodysize - sizeof(DiskBucketHeader::dirent_t))
	 		);
	 }//isLargeChunk
	
	 static bool isArtificialChunk(int local_depth){
	        return (local_depth >= Chunk::MIN_DEPTH);
	 }

private:
//__________________________ CLASS/STRUCT DEFINITIONS ____________________________________________________	
	    				
        /**
         * A structure for the case-vectors in AccessManager::putIntoBuckets.
         * An instance corresponds to a tree (or single data chunk).
         */
        struct CaseStruct {
        	/**
        	 * The chunk id of the root of the tree
        	 */
        	ChunkID id;
        	/**
        	 * The size-cost of the tree
        	 */
        	unsigned int cost;
        };			     	        			
	
    	
	/**
	 * This function class represents a simple clustering algorithm.
	 */
	class SimpleClusteringAlg {
	      	private:
      			//no extra input parameters needed for this algorithm.
	      	public:
	      		/**
	      		 * This implements the algorithm.The algorithm sequencially scans the
	      		 * input vector and tries to put in the same bucket as many trees as possible.
	      		 * NOTE: the algorithm does not sort the input vector nor it assumes any specific order.
	      		 *	 However, due to the way the caseBvect is constructed, its entries are sorted in
	      		 * 	 ascending order of their chunk id. Therefore, the algorithm does a row-major style
	      		 *	 traversal of the cells based on the interleaving order of the chunk id.
	      		 */
         		void operator()( const vector<CaseStruct>& caseBvect,
         				 multimap<BucketID,ChunkID>& resultRegions,
         				 vector<BucketID>& resultBucketIDs
         				);					
	};
	
	/*********************************************************************************
	 	This is just an example of how to create on more algortihm:
	 	
	class CPTClusteringAlg {
	      	private:	      		
	      		//The Current Point in Time (CPT)	      		
      			DiskChunkHeader::ordercode_t cpt;
	      	public:
	      		CPTClusteringAlg() {
	      			// here the private members must be properly initialised
	      		}
			void operator()( const vector<CaseStruct>& caseBvect,
					 multimap<BucketID,ChunkID>& resultRegions,
					 vector<BucketID>& resultBucketIDs
					);
	};	
	**********************************************************************************/
	    	    	
	/**
	 * This function class represents a method for resolving the storage of a large chunk.
	 */
	class EquiGrid_EquiChildren {
       	public:
       		/**
       		 * Constructor of the Equi_grid-equi_children method of resolving the storage of a large data chunk.
       		 * This routine is responsible for initializing specific data members in order when operator() member function
       		 * is called will be able to apply the method. If it encounters a large data chunk consiting of only one cell(!!)
       		 * it throws a GeneralError exception.
       		 *
       		 * @param	e	Maximum number of entries that fit in a bucket
       		 * @param 	d 	Number of dimensions of the cube, on which  the method will be applied
       		 * @param	rangeVect A vector containing the order-code ranges for each dimension of the
       		 *				input large data chunk (the one that the method will be applied on)
       		 */
       		EquiGrid_EquiChildren(int e, int d, const vector<LevelRange>& rangeVect);
       		
       		/**
       		 * Destructor
       		 */
       		~EquiGrid_EquiChildren() {}
       		
       		/**
       		 * The implementation of the method.
       		 * This routine stores a large data chunk in the current CUBE file. It imposes an artificial chunking
                 * to this data chunk and therefore it creates a two-level tree: a root dir chunk and the children data chunks.
                 * The chunking applied creates the same number of partitions along each dimension (equi-grid)
                 * and each partition groups approx, the same number of children at the next level for each dimension (equi-children).
		 *
		 * @param accmgr	pointer to the current instance of the AccessManager
               	 * @param cbinfo		all schema and system-related info about the cube. (input)
        	 * @param costRoot	the root of the cost sub-tree. (input) 	
        	 * All the rest parameters are identical to the ones appearing in putChunksintoBuckets and are included in this
        	 * function because they will be passed to the latter. All that operator() will do is to replace the input CostNode
        	 * with a new CostNode tree corresponding to further chunking of the initial large chunk; then, putChunksIntoBuckets is
        	 * called to take care all the rest.
       		 */
  		void operator()(
  					const AccessManagerImpl* const accmgr, //input	
					const CubeInfo& cbinfo, //input
        				const CostNode* const costRoot,  //input
        				unsigned int where2store,      //input
        				const string& factFile,        //input
        				vector<DirChunk>* const dirChunksRootDirVectp,     //output
        				DirEntry& returnDirEntry,      //output
        				const AccessManager::CBFileConstructionParams& constructionParams //input
        			);					
       	private:
		/**
		 * Maximum number of entries that fit in a bucket
		 */
		unsigned int maxDirEntries;
		
		 /**
		  * Number of dimensions of the cube, on which  the method will be applied
		  */
		unsigned int noDims;	
		  	
		/**
		 * Number of artificial partitions (i.e., number of members of new level) along each dimension.This will be the
		 * same for all dimensions (equi-grid)
		 */
		unsigned int noMembersNewLevel;
		
		/**
	       	 * The maximum number of children under each new member, equal for all members of a dimension (equi-children).
	       	 * One entry per dimension. A value of zero in a entry means that the corresponding dimension level is a pseudo level.
	       	 * Actually, we require that a valid maxNoChildrenPerDim entry, in order to have a no pseudo level is >= 2,
	       	 * otherwise we insert a 0, which means than no chunking takes place along the corresponding dimension.
	       	 */
       		vector<unsigned int> maxNoChildrenPerDim;	
       		
       		/**
       		 * This number shows how many of the newly inserted members are pseudo levels
       		 */
       		unsigned int noPseudoLevels;
       		
              	/**
              	 * Create the new hierarchies per dimension due to the artificial chunking
              	 * and store them in a vector of maps (one map per dimension)
              	 *
              	 * @param	oldRangeVect 	A vector containing the order-code ranges for each dimension of the
              	 *					input large data chunk (the one that the method will be applied on) - input parameter
                 * @param	newHierarchyVect	output parameter storing the hierarchies for all dimensions
                 */       		
        	void createNewHierarchies(
        				const vector<LevelRange>& oldRangeVect, //input
                       			vector<map<int, LevelRange> >& newHierarchyVect //output
               	        		 );
       			
	};//end class EquiGrid_EquiChildren
	
	friend class EquiGrid_EquiChildren; //so that we can call putChunksIntoBuckets from
					    //the operator() method
	
	/**
	 * This function class represents a method for storing the root directory of the CUBE File.
	 * In particular, in this method the whole root directory is stored in a single (large) bucket
	 * in a depth 1st manner.
	 */
	class SingleBucketDepthFirst {	
	public:
		/**
		 * The bucket header of the root bucket, for this method
		 * This will be stored in the header part of a SSM record.
		 * We assume that in the record body, 1st the bucket directory
		 * will be placed and then the byte vector of DiskDirChunks will follow
		 */
		struct DiskRootBucketHeader {
                	/**
                	 * Define the type of a directory entry. This typically
                	 * denotes a byte offset within the byte vector.
                	 */
                        typedef memSize_t dirent_t;		

                	/**
                	 * Define the type of size in bytes
                	 */
                        typedef memSize_t bytesize_t;		

                	/**
                	 * Define the type of number of entries
                	 */
                        typedef unsigned int entriesnum_t;		

			/**
			 * The total number of chunks of the root directory. Equals
			 * with the number of VALID entries (i.e., chunk slots) in the
			 * root bucket directory. Note that the directory might have
			 * more entries than the no_chunks, in order to anticipate
			 * future insertions.
			 */
			entriesnum_t no_chunks;
			
			/**
			 * The total number of entries in the root bucket directory.
			 * This should be >= no_chunks
			 */
			entriesnum_t totDirEntries;			
					
			/**
			 * The total size (in bytes) of the root bucket body.
			 * I.e., directory + byte vector
			 */
			bytesize_t bodySz;
			
			/**
			 * The byte offset in the SSM record body, where the byte vector
			 * of DiskDirChunks starts. 1st byte in the body is at offset 0.
			 */
			dirent_t byteVectOffset;			
			
			DiskRootBucketHeader(entriesnum_t c, entriesnum_t t, bytesize_t s, dirent_t b):
				no_chunks(c), totDirEntries(t), bodySz(s), byteVectOffset(b){}
		};//struct DiskRootBucketHeader				
		
		/**
		 * Constructor
		 */
		 SingleBucketDepthFirst(memSize_t rdlb, memSize_t m, const AccessManagerImpl* const am):
		 		 rootDirSzLowBound(rdlb), memAvailable(m), accmgr(am) //just copy the pointer value,
		 		 						      // no need to "new" an AccessManagerImpl
		 {}
		
		 /**
		  * Destructor
		  */
		  ~SingleBucketDepthFirst() {
		  			      //It is not the responsibility of this class to delete		  			
		  			      //the pointed to AccessMangerImpl instance on destruction.
		  			    }
		
		  /**
		   * The implementation of the singleBucketDepthFirst method. It stores in a single bucket
		   * of special size the whole root directory, in a depth 1st manner.
               	   *
	           * @param cinfo		all schema and system-related info about the cube. (input)               	
               	   * @param	dirChunksRootDirVect	vector with the dir chunks of the root directory (input)
               	   * @param	rootBcktID	the bucket id where the root chunk will be stored (input)		
		   */
		  void operator()(const CubeInfo& cinfo, vector<DirChunk>& dirChunksRootDirVect, const BucketID& rootBcktID);

	private:
		/**
		 * A lower bound byte-size for the root directory. (this is a lower bound
		 * because the overhead of order-code mappings for artificial chunks has not been counted)
		 */
		 memSize_t rootDirSzLowBound;

		/**
		 * Shows how much memory we have available for storing the root directory during quering.
		 */
		 memSize_t memAvailable;
		
		 /**
		  * it holds the current instance of the access manager
		  */
		 const AccessManagerImpl* const accmgr;		
		
		/**
		 * This routine accepts as input a vector of DirChunks. Each input DirChunk will be ultimately
		 * transformed into a DiskDirChunk and stored in a byte vector, pointed to by "byteVectp".The storage
		 * sequence in the byte vector will be identical with the one in the input vector. Also a directory vector
		 * will be created (pointed to by "dirVectp"). This will store the mappings between "chunk slots" and actual
		 * byte offsets in the byte vector. (each DiskDirChunk is stored in a specific chunk slot independent of
		 * the actual byte offset).
		 * NOTE: The byte offsets stored in the directory make the assumption that the 1st byte in the byte vector (*byteVectp)
		 * is at offset 0.
		 *
		 * @param	maxDepth	The maximum (global) chunking depth for this cube
		 * @param inputChunkVect	input vector of DirChunks
		 * @param byteVectp		output pointer to byte vector of DiskDirChunks
		 * @param dirVectp		output pointer to internal bucket directory
		 */
		void createRootBucketVectorsInHeap(     unsigned int maxDepth,
							vector<DirChunk>& inputChunkVect,
							vector<char>* &byteVectp,
							vector<DiskRootBucketHeader::dirent_t>* &dirVectp);		
		
		/**
		 * Protection from copy construction
		 */		
		SingleBucketDepthFirst(SingleBucketDepthFirst& );
		
		/**
		 * Protection from assignement
		 */
		SingleBucketDepthFirst& operator=(const SingleBucketDepthFirst& );
		 	
	}; //class SingleBucketDepthFirst
	
	friend class SingleBucketDepthFirst; //so that we can call private methods of AccessMangerImpl
	
//______________________ PRIVATE DATA MEMBERS __________________________________________________________________________

	/**
     	 * The output stream used for logging system messages.
     	 */
      	ostream& outputLogStream;

        /**
         * The output stream used for logging error messages.
         */
    	ofstream& errorLogStream;
    	
//______________________ PRIVATE METHOD DECLARATIONS ____________________________________________________________________    	
    	    	    	                									     	
	/**
	 * This function implements the basic CUBE File construction
	 * algorithm, as it is described in the technical report.
	 *
	 * @param cinfo		all schema and system-related info about the cube
	 * @param factFile	file with fact values. Each fact value is associated with a chunk-id
	 * @param configFile	file with construction parameters, such as the clustering algorithm, large chunk resolution method, etc.
	 */
	void constructCUBE_File(CubeInfo& cinfo, const string& factFile, const string& configFile) const;
//	void constructCubeFile(CubeInfo& cinfo, string& factFile); // OLD
	

	
	/**
	 * Function for testing phase I of CUBE File construction.
	 *
	 * @param costRoot	Root of the CostNode tree created after phase I.
	 * @param cinfo  	the corresponding cube information
	 */				
	 void test_construction_phaseI(CostNode* costRoot, CubeInfo& cinfo)const;
	
	/**
	 * This recursive function receives a CostNode tree and the file with the fact values
	 * and has all the logic on how to construct the Cube File by packing into buckets (ssm records) all the
	 * chunks. It is first called from AccessManager::constructCubeFile in order to put the entire chunk tree
	 * into buckets. However, whenever we find a chunk sub-tree (or even a single large data chunk) that does not
	 * fit in a single bucket, then we recursively call it again.
	 *
 	 * @param cbinfo	all schema and system-related info about the cube. We assume that at least the
 	 *			following CubeInfo daa members are inititialized upon calling this routine: rootBucketID,
 	 *			maxDepth, numFacts, num_of_dimensions. -- input parameter
	 * @param costRoot	the root of the cost sub-tree. -- input parameter
	 * @param where2store	this is an index (i.e.offset) showing where in the DirChunk vector of the
	 *			root bucket to store the chunk which corresponds to the costRoot. --input parameter
	 *			NOTE: before a recursive call to putIntoBuckets, we reserve one more element
	 *			in the vector and the call putIntoBuckets
	 *			with where2store = rtDirDataVectp->capacity();
	 * @param factFile	file with fact values. Each fact value is associated with a chunk-id. -- input parameter
	 * @param dirChunksRootDirVect  This is a pointer to a vector containing the DirChunk instances of the root directory of the CUBE File.
	 *			Each call to putIntoBucket inserts one more chunk in it,
	 *			at the where2store position. -- output parameter
	 * @param returnDirEntry This is a returned DirEntry containing BucketId and chunkIndex info for
	 *			the chunk for which the putIntoBucket was called.The caller must store this
	 *			info in the corresponding parent chunk cell. -- output parameter
	 * @param constructionParams	input parameter containing values for the CUBE File construction options, such as
	 *			clustering method used, or large-chunk resolution method used, etc.
	 */	
	void putChunksIntoBuckets(const CubeInfo& cbinfo,
				CostNode* const costRoot,
				unsigned int where2store,
				const string& factFile,
				vector<DirChunk>* const dirChunksRootDirVectp,
				DirEntry& returnDirEntry,
				const AccessManager::CBFileConstructionParams& constructionParams
				) const;

/*	void putIntoBuckets(CubeInfo& cinfo,
				CostNode* costRoot,
				unsigned int where2store,
				string& factFile,
				vector<DirChunk>* rtDirDataVectp,
				DirEntry& returnDirEntry);	
*/

	/**
	 * This routine is responsible for storing the root directory of the CUBE File structure
	 * for a  cube, according to the method passed in the rootDirStg parameter. Actually it just
	 * invokes the appropriate routines according to the selected method. It receives as input a
	 * vector containing instances of the directory chunks of the root directory stored
	 * in a depth first manner. Finally, it receives a bucket id referencing the bucket within the CUBE file
	 * where the "root chunk" of the CUBE file will be stored and possibly (depends on the selected storage method)
	 * the whole root directory. Note that this does not correspond to an allocated bucket
	 * only the id has been created. In case where the root directory is stored in more than one buckets, this
	 * will play the role of the "entry point" bucket for accessing the root directory. On return the input chunk
	 * is empty (for memory saving reasons).
	 *
	 * @param cinfo		all schema and system-related info about the cube. (input)	
	 * @param	dirChunksRootDirVect	vector with the dir chunks of the root directory (input)
	 * @param	rootIndex	the position in the input vector where the root chunk resides.
	 * @param	rootBcktID	the bucket id where the root chunk will be stored (input)
	 * @param 	rootDirStg	token denoting the root directory stg method. (input)
	 * @param	memory_constraint	an input parameter showing the available memory for
	 *				holding the root directory when accessing the CUBE File, since
	 *				some storage methods keep the whole root directory in memory during quering.
	 */
	void storeRootDirectoryInCUBE_File(
					const CubeInfo& cinfo, //input
					vector<DirChunk>& dirChunksRootDirVect,
					unsigned int rootIndex,
					const BucketID& rootBcktID,
					AccessManager::rootDirectoryStorage_t rootDirStg,
					memSize_t memory_constraint) const;
					
	/**
	 * This procedure stores a single data chunk in a single bukcet. Therefore
	 * it is assumed that the total size of the chunk satisfies the
	 * following: BUCKET_THRESHOLD <= size <= DiskBucket::bodysize.
	 *
	 * @param cinfo		all schema and system-related info about the cube. (input)
	 * @param costRoot	the root of the cost node corresponding to this chunk. (input) 	
	 * @param factFile	file with fact values. Each fact value is associated with a chunk-id. (input)
	 * @param returnDirEntry This is a returned DirEntry containing BucketId and chunkIndex info for
	 *			the chunk for which the function was called. The caller must store this
	 *			info in the corresponding parent chunk cell. (output)	 		
	 */								
        void storeDataChunkInCUBE_FileBucket(
               				const CubeInfo& cinfo, //input
               				const CostNode* const costRoot, //input
               				const string& factFile,  //input
               				DirEntry& returnDirEntry  //output
        				) const;

	
        				        				
        /**
         * This routine receives as input a pointer to a CostNode that corresponds to a DataChunk
         * that has a size > free space of an empty bucket. This routine will call the appropriate
         * method for resolving the storage of the large data chunk and return the appropriate dir entry
         * to the parent.
         *
       	 * @param cbinfo		all schema and system-related info about the cube. (input)
	 * @param costRoot	the root of the cost sub-tree. (input) 	
	 * All the rest parameters are identical to the ones appearing in putChunksintoBuckets and are included in this
	 * function because they will be passed to the latter.(at least for methods that employ artificial chunking)
         */
        void storeLargeDataChunk(	const CubeInfo& cbinfo, //input
        				const CostNode* const costRoot,  //input
        				unsigned int where2store,      //input
        				const string& factFile,        //input
        				vector<DirChunk>* const dirChunksRootDirVectp,     //output
        				DirEntry& returnDirEntry,      //output
        				const AccessManager::CBFileConstructionParams& constructionParams //input
        			)const;
        			
	/**
	 * This procedure stores a single cost-node tree in a bucket of a CUBE File. Therefore
	 * it is assumed that the total size of the chunks corresponding to this tree satisfies the
	 * following: BUCKET_THRESHOLD <= size <= DiskBucket::bodysize.
	 *
	 * @param cinfo		all schema and system-related info about the cube. (input)
	 * @param costRoot	the root of the cost sub-tree. (input) 	
	 * @param factFile	file with fact values. Each fact value is associated with a chunk-id. (input)
	 * @param returnDirEntry This is a returned DirEntry containing BucketId and chunkIndex info for
	 *			the chunk for which the function was called. The caller must store this
	 *			info in the corresponding parent chunk cell. (output)	 	
         * @param howToTraverse a token indicating traversal of the tree method and storage method.
         *			Possible values: depthFirst, breadthFirst - (input parameter)	
	 */				
	void storeSingleTreeInCUBE_FileBucket(	const CubeInfo& cinfo, //input
						const CostNode* const costRoot, //input
						const string& factFile,  //input
						DirEntry& returnDirEntry,  //output
						const AccessManager::treeTraversal_t howToTraverse//input
						)const;

	/**
	 * This procedure receives as input a single cost-node tree and a
	 * text file with grain-level fact values for the cells at the data
	 * level of this tree. Each row in this file corresponds to a single cell
	 * and contains the chunk id of the cell and then the measure values following
	 * separated by whitespace. The procedure descends the cost-tree in some way
	 * and creates instances of DirChunks and DataChunks (the latter loaded with
	 * measure data from the input file). these instances are stored in two separate
	 * vectors.
	 *
         * @param maxDepth      the max chunking depth of the cube in question (input parameter)
         * @param numFacts      the number of facts in each data entry (i.e., cell) - (input parameter)
	 * @param costRoot	the root of the cost sub-tree. 	
	 * @param factFile	file with fact values. Each fact value is associated with a chunk-id	
	 * @param bcktID	Id of the bucket where the sub-tree will be stored.
	 * @param dirVectp	pointer to the DirChunk vector which will be filled with chunks	
	 * @param dataVectp	pointer to the DataChunk vector which will be filled with chunks
         * @param howToTraverse a flag indicating traversal of the tree method and storage method.
         *			Possible values: depthFirst, breadthFirst - (input parameter)		 		
	 */
        void traverseSingleCostTreeCreateChunkVectors(
        				unsigned int maxDepth,
        				unsigned int numFacts,
        				const CostNode* const costRoot,
        				const string& factFile,
        				const BucketID& bcktID,
        				vector<DirChunk>* &dirVectp,
        				vector<DataChunk>* &dataVectp,
        				const AccessManager::treeTraversal_t howToTraverse)const;						
	
	/**
	 * This recursive function is called  in order to descend a single
	 * cost tree (in a depth first way) and fill the DirChunk & Datachunk vectors with
	 * chunk objects. We store each chunk we meet as we descend the tree. Therefore,
	 * the root will be stored at position begin() of the corresponding vector.
	 *
         * @param maxDepth      the max chunking depth of the cube in question (input parameter)
         * @param numFacts      the number of facts in each data entry (i.e., cell) - (input parameter)
	 * @param costRoot	the root of the cost sub-tree. 	
	 * @param factFile	file with fact values. Each fact value is associated with a chunk-id	
	 * @param bcktID	Id of the bucket where the sub-tree will be stored.
	 * @param dirVectp	pointer to the DirChunk vector which will be filled with chunks	
	 * @param dataVectp	pointer to the DataChunk vector which will be filled with chunks		
	 */
        void descendDepth1stCostTree(	 unsigned int maxDepth,
        				 unsigned int numFacts,
        				 const CostNode* const costRoot,
        				 const string& factFile,
        				 const BucketID& bcktID,
        				 vector<DirChunk>* const dirVectp,
        				 vector<DataChunk>* const dataVectp) const;

	/**
	 * This recursive function is called  in order to descend a single
	 * cost tree (in a breadth first way) and fill the DirChunk & Datachunk vectors with
	 * chunk objects. We store each chunk the first time we visit it.  Therefore,
	 * the root will be stored at position begin() of the corresponding vector. We use
	 * a queue structure in order to implement the breadth first algorithm.
	 *
         * @param maxDepth      the max chunking depth of the cube in question (input parameter)
         * @param numFacts      the number of facts in each data entry (i.e., cell) - (input parameter)
	 * @param costRoot	the root of the cost sub-tree. 	
	 * @param factFile	file with fact values. Each fact value is associated with a chunk-id	
	 * @param bcktID	Id of the bucket where the sub-tree will be stored.
	 * @param nextToVisit	the breadth1st queue
	 * @param dirVectp	pointer to the DirChunk vector which will be filled with chunks	
	 * @param dataVectp	pointer to the DataChunk vector which will be filled with chunks		
	 */        				         				
        void descendBreadth1stCostTree(unsigned int maxDepth, unsigned int numFacts,
        				 const CostNode* const costRoot,
        				 const string& factFile,
        				 const BucketID& bcktID,
        				 queue<CostNode*>& nextToVisit,
        				 vector<DirChunk>* const dirVectp,
        				 vector<DataChunk>* const dataVectp) const;        				
        /**
         * This routine receives a vector of chunk ids and a number of domains nodoms(>0). It then removes from each chunk id
         * nodoms domains from the end (just before the last domain). It puts these new ids (i.e., the old ones without the removed part)
         * in a new vector. If nodoms <= 0, or, more than the intermediate domains are to be removed, it throws a GeneralError exception.
         *
         * NOTE : The input chunk ids are assumed to be the chunk ids of cells whose parent data chunks have been artificially chunked.
         * ^^^^                                                          ^^^^^
         * Therefore, this routine SHOULD NOT BE CALLED upon chunk ids of regular chunks (directory or data chunks)!!!
         *
         * @param	nodoms	number of removed domains
         * @param	inputVect	input vector of chunk ids
         * @param	outputVect 	output vector of chunk ids
         */				
	void removeArtificialDomainsFromCellChunkIDs(int nodoms, const vector<ChunkID>& inputVect,vector<ChunkID>& outputVect) const;

	/**
         * This procedure receives a reference to a const DataChunk and
         * a NULL pointer to a DiskBucket. It allocates in heap a new DiskBucket and fills it with only this chunk.
         *
         * @param maxDepth      the max chunking depth of the cube in question (input parameter)
         * @param numFacts      the number of facts in each data entry (i.e., cell) - (input parameter)
         * @param bcktID        the bucket id of the bucket corresponding to this DiskBucket - (input parameter)
	 * @param datachunk	reference to the data chunk - (input parameter)	
	 * @param dbuckp	returned pointer to the allocated DiskBucket - (output parameter)
	 */        				         				
	void createSingleDataChunkDiskBucketInHeap(
					unsigned int maxDepth,
					unsigned int numFacts,
					const BucketID& bcktID,	
					const DataChunk& datachunk,
					DiskBucket* &dbuckp) const;

        /**
         * This procedure receives 2 vectors containing DirChunks and DataChunks respectively and
         * a NULL pointer to a DiskBucket. It allocates in heap a new DiskBucket and fills it appropriatelly
         * with the chunks from the two input vectors. The input flag howToTraverse denotes if the chunks
         * should be stored in a depth first or a breadth first manner.
         *
         * @param maxDepth      the max chunking depth of the cube in question (input parameter)
         * @param numFacts      the number of facts in each data entry (i.e., cell) - (input parameter)
         * @param bcktID        the bucket id of the bucket corresponding to this DiskBucket - (input parameter)
	 * @param dirVectp	pointer to the DirChunk vector filled with dir chunks - (input parameter)	
	 * @param dataVectp	pointer to the DataChunk vector filled with data chunks	- (input parameter)	
	 * @param dbuckp	returned pointer to the allocated DiskBucket - (output parameter)
         * @param howToTraverse a flag indicating storage method. Possible values: depthFirst,
         *			breadthFirst - (input parameter)
         */
        void createSingleTreeDiskBucketInHeap(unsigned int maxDepth, unsigned int numFacts,
        		const BucketID& bcktID, const vector<DirChunk>*dirVectp,
        		const vector<DataChunk>*dataVectp, DiskBucket* &dbuckp,
        		const AccessManager::treeTraversal_t howToTraverse) const;

	/* This procedure receives as input two vectors containing the DirChunks and DataChunks respectively
	 * of a single tree and a byte pointer to the first free byte in the body of a DiskBucket structure
	 * and places these chunks according to a traversal method in the body of the DiskBucket.
	 *
         * @param maxDepth      the max chunking depth of the cube in question (input parameter)
         * @param numFacts      the number of facts in each data entry (i.e., cell) - (input parameter)
	 * @param dirVectp	pointer to the DirChunk vector filled with dir chunks - (input parameter)	
	 * @param dataVectp	pointer to the DataChunk vector filled with data chunks	- (input parameter)	
	 * @param dbuckp	pointer to the allocated DiskBucket - (input parameter)
	 * @param nextFreeBytep	a byte pointer (input+output parameter) that points at the beginning of
	 *			the DiskBucket's body and at the first free byte on return.
         * @param howToTraverse a flag indicating storage method. Possible values: depthFirst,
         *			breadthFirst - (input parameter)	
	 */        		         		        			        		
        void placeChunksOfSingleTreeInDiskBucketBody(
        		unsigned int maxDepth,
        		unsigned int numFacts,
        		const vector<DirChunk>* const dirVectp,
        		const vector<DataChunk>* const dataVectp,
        		DiskBucket* const dbuckp,
        		char* &nextFreeBytep,
        		const AccessManager::treeTraversal_t howToTraverse) const;
        		
	/**
	 * This procedure receives a reference to a const DataChunk and a byte pointer
	 * pointing at the first free byte in the body of a DiskBucket structure. Its job is to
	 * place the datachunk in the body of the bucket and return thebyte pointer pointing at the
	 * next free byte. A const pointer to the DiskBucket structure is also provided and it is assumed that its
	 * DiskBucketHeader members have been properly intitialized, since this routine does not update any header fields
	 * except from thos that are affected by the insertion of the datachunk in the body of the bucket.
	 *
         * @param maxDepth      the max chunking depth of the cube in question (input parameter)
         * @param numFacts      the number of facts in each data entry (i.e., cell) - (input parameter)
	 * @param datachunk	a reference to the datachunk - (input parameter)	
	 * @param dbuckp	pointer to the allocated DiskBucket - (input parameter)
	 * @param nextFreeBytep	a byte pointer (input+output parameter) that points at the beginning of	
	 *			the DiskBucket's body and at the first free byte on return.	
	 */        		
        void placeSingleDataChunkInDiskBucketBody(
        			unsigned int maxDepth,
        			unsigned int numFacts,
        			const DataChunk& datachunk,
        			DiskBucket* const dbuckp,
        			char* &nextFreeBytep)const;        			
        		
	/**
	 * This procedure is called from AccessManager::createDiskBucketInHeap in order to store
	 * the two input vectors of chunks in a newly allocated DiskBucket that is pointed to by the input
	 * parameter dbuckp, in a breadth first manner. The input DiskBucket struct has its header member
	 * initialized appropriately.
	 *
         * @param maxDepth      the max chunking depth of the cube in question (input parameter)
         * @param numFacts      the number of facts in each data entry (i.e., cell) - (input parameter)
	 * @param dirVectp	pointer to the DirChunk vector filled with dir chunks - (input parameter)	
	 * @param dataVectp	pointer to the DataChunk vector filled with data chunks	- (input parameter)	
	 * @param dbuckp	pointer to the allocated DiskBucket - (input parameter)
	 * @param nextFreeBytep	a byte pointer (input+output parameter) that points at the beginning of
	 *			the DiskBucket's body and at the first free byte on return.
	 */        		         		
        void _storeBreadth1stInDiskBucket(unsigned int maxDepth, unsigned int numFacts,
        		 const vector<DirChunk>* const dirVectp, const vector<DataChunk>* const dataVectp,
        		 DiskBucket* const dbuckp, char* &nextFreeBytep) const;		 		
        		
	/**
	 * This procedure is called from AccessManager::createDiskBucketInHeap in order to store
	 * the two input vectors of chunks in a newly allocated DiskBucket that is pointed to by the input
	 * parameter dbuckp, in a depth first manner. The input DiskBucket struct has its header member
	 * initialized appropriately.
	 *
         * @param maxDepth      the max chunking depth of the cube in question (input parameter)
         * @param numFacts      the number of facts in each data entry (i.e., cell) - (input parameter)
	 * @param dirVectp	pointer to the DirChunk vector filled with dir chunks - (input parameter)	
	 * @param dataVectp	pointer to the DataChunk vector filled with data chunks	- (input parameter)	
	 * @param dbuckp	pointer to the allocated DiskBucket - (input parameter)
	 * @param nextFreeBytep	a byte pointer (input+output parameter) that points at the beginning of
	 *			the DiskBucket's body and at the first free byte on return.
	 */        		         		        		
        void _storeDepth1stInDiskBucket(unsigned int maxDepth, unsigned int numFacts,
        		 const vector<DirChunk>* const dirVectp, const vector<DataChunk>* const dataVectp,
        		 DiskBucket* const dbuckp, char* &nextFreeBytep) const;		 		
        				 		
        /**
         * This function job is to receive a DirChunk and create and return a DiskDirChunk struct.
         * It allocates space for the latter and copies appropriately the contents of the former
         * to the new instance of DiskDirChunk.
         *
         * @param dirchnk	the input DirChunk instance
         * @param maxDepth	the maximum chunking depth for the cube in question (input parameter)
         */
	DiskDirChunk* dirChunk2DiskDirChunk(const DirChunk& dirchnk, unsigned int maxDepth) const;

        /**
         * This function receives a datachunk the number of facts in a cell and the maximum depth of the
         * cube in question. It allocates space for a DiskDataChunk in heap and copies appropriately the
         * contents of the input DirChunk to it. A pointer to the DiskDataChunk is returned.
         *
         * @param datachnk	the input DataChunk instance
         * @param numFacts	the number of facts (measures)  in a cell (input parameter)
         * @param maxDepth	the maximum chunking depth for the cube in question (input parameter)
         */		
         DiskDataChunk* dataChunk2DiskDataChunk(const DataChunk& datachnk, unsigned int numFacts,
         					unsigned int maxDepth) const;
	
	/**
	 * This function places a DiskDirChunk structure into the body
	 * of a DiskBucket. currentp initially point at the place (in the body of
	 * a DiskBucket) where the DiskDirChunk must be placed. On return it points at the
	 * next free byte in the body. Also, the bytes consumed by this placememnt of the DiskChunkHeader
	 * (inside the DiskDirChunk) and the DiskDataChunk are returned.
	 *
	 * @param chnkp		pointer to the DiskDirChunk
	 * @param maxDepth	the maximum depth of the cube in question	
	 * @param currentp	the byte pointer in the body of DiskBucket
	 * @param hdr_size	the returned size of the bytes consumed by the placemment of the
	 *			DiskChunkHeader
	 * @chnk_size		the returned size in bytes consumed by the placement of the DiskDirChunk
	 */		
	void placeDiskDirChunkInBcktBody(const DiskDirChunk* const chnkp, unsigned int maxDepth,
			char* &currentp, size_t& hdr_size, size_t& chnk_size) const;				

	/**
	 * This function places a DiskDataChunk structure into the body
	 * of a DiskBucket.currentp initially point at the place (in the body of
	 * a DiskBucket) where the DiskDataChunk must be placed. On return it points at the
	 * next free byte in the body. Also, the bytes consumed by this placememnt of the DiskChunkHeader
	 * (inside the DiskDataChunk) and the DiskDataChunk are returned.
	 *
	 * @param chnkp		pointer to the DiskDataChunk
	 * @param maxDepth	the maximum depth of the cube in question
	 * @param currentp	the byte pointer in the body of DiskBucket	
	 * @param hdr_size	the returned size of the bytes consumed by the placemment of the
	 *			DiskChunkHeader
	 * @chnk_size		the returned size in bytes consumed by the placement of the DiskDataChunk
	 */										
	void placeDiskDataChunkInBcktBody(const DiskDataChunk* const chnkp, int maxDepth,
			char* &currentp,size_t& hdr_size, size_t& chnk_size) const;		
		
//	/**
//	 * This function places a DiskChunkHeader structure into the body
//	 * of a DiskBucket.
//	 *
//	 * @param hdrp		pointer to the DiskChunkHeader
//	 * @param currentp	the byte pointer in the body of DiskBucket
//	 * @hdr_size		the returned size in bytes consumed for this placement
//	 */
//	void placeDiskChunkHdrInBody(const DiskChunkHeader* const hdrp,
//				char* currentp, size_t& hdr_size)

        /**
         * This procedure receives a set of chunk-trees hanging from the same parent
         * that each has a size-cost less than the bucket threshold, and stores them in
         * clusters (or bucket regions), where each cluster contains two or more of these
         * trees and fits in a single bucket.
	 *
	 * @param cinfo		all schema and system-related info about the cube (input parameter).		
	 * @param caseBvect	the input vector with the ids of the cells + the cost of sub-trees
	 *			these cells point to. Sub-trees have size < Bucket occupancy threshold (input parameter)
	 * @param costRoot	pointer to the parent node of the trees (input parameter)
	 * @param factFile	file with fact values. Each fact value is associated with a chunk-id			
	 * @param resultVect	output parameter containing the DirEntry for each tree (1-1 correspondance with caseBvect)
         * @param howToTraverse 	a flag indicating traversal of the tree method and storage method.
         *			Possible values: depthFirst, breadthFirst - (input parameter)		
	 * @param clustering_alorithm	A label denoting which clustering algorithm will be used for
	 *				the formulation of "Bucket regions".
	 */
        void storeTreesInCUBE_FileClusters(const CubeInfo& cinfo,
                                const vector<CaseStruct>& caseBvect,
                                const CostNode* const costRoot,
                                const string& factFile,
                                vector<DirEntry>& resultVect,
                                const AccessManager::treeTraversal_t howToTraverse,
                                const AccessManager::clustAlgToken_t clustering_algorithm)const;

        /**
         * This procedure receives a set of data chunks hanging from the same parent
         * that each has a size-cost less than the bucket threshold, and stores them in
         * clusters (or bucket regions), where each cluster contains two or more of these
         * data chunks (handled as diffrent sub-trees) and fits in a single bucket.
	 *
	 * @param cinfo		all schema and system-related info about the cube (input parameter).		
	 * @param caseBvect	the input vector with the ids of the cells + the cost of sub-trees
	 *			these cells point to. Sub-trees have size < Bucket occupancy threshold (input parameter)
	 * @param costRoot	pointer to the parent node of the data chunks (input parameter)
	 * @param factFile	file with fact values. Each fact value is associated with a chunk-id			
	 * @param resultVect	output parameter containing the DirEntry for each tree (1-1 correspondance with caseBvect)
	 * @param clustering_alorithm	A label denoting which clustering algorithm will be used for
	 *				the formulation of "Bucket regions".
	 */
        void storeDataChunksInCUBE_FileClusters(
        			const CubeInfo& cinfo,
        			const vector<CaseStruct>& caseBvect,
        			const CostNode* const costRoot,
        			const string& factFile,
        			vector<DirEntry>& resultVect,
                		const AccessManager::clustAlgToken_t clustering_algorithm) const;

	/**
	 * This procedure creates a DiskBucket instance in heap that contains
	 * a region (i.e. cluster) of data chunks hanging from the same parent chunk.
	 * It also provides  the DirEntries in the parent chunk for each data chunk after
	 * its placement to the DiskBucket.
	 *
         * @param maxDepth      the max chunking depth of the cube in question (input parameter)
         * @param numFacts      the number of facts in each data entry (i.e., cell) - (input parameter)
	 * @param treesOfRegion		Vector with pointers to the cost nodes that belong to the
	 *				same region -input parameter
	 * @param buckid		The bucket id where the cluster will be stored - input parameter.
	 * @param factFile	file with fact values. Each fact value is associated with a chunk-id - input param.
	 * @param dbuckp	returned pointer to the allocated DiskBucket - (output parameter)
	 * @param resultMap	The returned DirEntry in the parent chunk, associated to each tree
	 *			via the chunk id of its root - output parameter.
	 */			                		                		
        void createDataChunkRegionDiskBucketInHeap(
        			unsigned int maxDepth,
        			unsigned int numFacts,
        			const vector<CostNode*>& dataChunksOfregion,
        			const BucketID& bcktID,
        			const string& factFile,
        			DiskBucket* &dbuckp,
        			map<ChunkID, DirEntry>& resultMap) const;        			
                                				
		
	/**
	 * This function receives as input a vector of DirChunk cells that point to
	 * sub-trees with size less than the bucket occupancy threshold. This function
	 * invokes a clustering algorithm to decide on the formulation of clusters (i.e. "Bucket Regions")
	 * Each cluster will be stored in a single bucket. The result is returned in resultBucketIDs where
	 * the ids of the buckets that will hold the regions are contained. And in resultRegions where the
	 * mapping of a bucket to the corresponding trees is stored.
	 *
	 * @param caseBvect	the input vector with the ids of the cells + the cost of sub-trees
	 *			these cells point to. Sub-trees have size < Bucket occupancy threshold (input parameter)
	 * @param resultRegions		output parameter holding the association of each bucket with the trees that will be stored in it
	 * @param resultBucketIDs	output parameter holding the list of the bucket ids of the buckets that will store the
	 * 				formulated regions.
	 * @param clustering_alorithm	A label denoting which clustering algorithm will be used for
	 *				the formulation of "Bucket regions".
	 *				
	 */
	 void formulateBucketRegions(
			const vector<CaseStruct>& caseBvect,
			multimap<BucketID, ChunkID>& resultRegions,
			vector<BucketID>& resultBucketIDs,
			const AccessManager::clustAlgToken_t clustering_algorithm) const;

	/**
	 * This procedure creates a DiskBucket instance in heap that contains
	 * a region (i.e. cluster) of trees hanging from the same parent chunk.
	 * It also provides  the DirEntries in the parent chunk for each tree after
	 * its placement to the DiskBucket.
	 *
         * @param maxDepth      the max chunking depth of the cube in question (input parameter)
         * @param numFacts      the number of facts in each data entry (i.e., cell) - (input parameter)
	 * @param treesOfRegion		Vector with pointers to the cost trees that belong to the
	 *				same region -input parameter
	 * @param buckid		The bucket id where the cluster will be stored - input parameter.
	 * @param factFile	file with fact values. Each fact value is associated with a chunk-id - input param.
	 * @param dbuckp	returned pointer to the allocated DiskBucket - (output parameter)
	 * @param resultMap	The returned DirEntry in the parent chunk, associated to each tree
	 *			via the chunk id of its root - output parameter.
         * @param howToTraverse 	a flag indicating traversal of the tree method and storage method.
	 *				Possible values: depthFirst, breadthFirst - (input parameter)		
	 */			
	void createTreeRegionDiskBucketInHeap(
			unsigned int maxDepth,
			unsigned int numFacts,
			const vector<CostNode*>& treesOfregion,
			const BucketID& bcktID,
			const string& factFile,
			DiskBucket* &dbuckp,
			map<ChunkID, DirEntry>& resultMap,
			const AccessManager::treeTraversal_t howToTraverse)const;
						
	/**
	 * This procedure receives a pointer to a DiskBucket structure that contains a single tree
	 * or a cluster with trees (i.e a bucket region) allocated in heap space. It prints the contents
	 * to a file which has a unique name for each bucket of the form: OutputDiskBucket_XXX.dbug, where
	 * XXX is a unique code based on the Bucket id, or a simple static variable.
	 * NOTE: the DiskBucket structure contains some pointer members that should point at certains places
	 * in the buckets body. This function prior to reading updates these pointers in order to point at the
	 * right place. Therefore, on return the DiskBucket structure has been also updated.
	 *
	 * @param dbuckp	input+output parameter - points to the heap allocated DiskBucket structure
	 * @param maxDepth	the maximum chunking depth for this cube
	 */						
        void printDiskBucketContents_SingleTreeAndCluster(DiskBucket* const dbuckp, unsigned int maxDepth)const;

	/**
	 * This procedure receives a byte pointer that points at the beginning of a DiskDirChunk stored
	 * in the body of a DiskBucket structure. Also it receives an output file stream that it uses in
	 * order to print the contents of the chunk. Also, prior to printing it updates the pointer members
	 * of the chunk in order to point at the appropriate location in the chunk.
	 *
	 * @param out	the output file stream - input parameter.
	 * @param startp	byte pointer, pointer at the beginning of a DiskDirChunk - input+output parameter
         * @param maxDepth	maximum chunking depth for this cube	
	 */
        void printDiskDirChunk(ofstream& out, char* const startp, unsigned int maxDepth)const;

	/**
	 * This procedure receives a byte pointer that points at the beginning of a DiskDataChunk stored
	 * in the body of a DiskBucket structure. Also it receives an output file stream that it uses in
	 * order to print the contents of the chunk. Also, prior to printing it updates the pointer members
	 * of the chunk in order to point at the appropriate location in the chunk.
	 *
	 * @param out	the output file stream - input parameter.
	 * @param startp	byte pointer, pointer at the beginning of a DiskDataChunk - input+output parameter
         * @param maxDepth	maximum chunking depth for this cube	
	 */
        void printDiskDataChunk(ofstream& out, char* const startp, unsigned int maxDepth)const;

        /**
         * This procedure receives a reference to a DiskDirChunk. It updates its pointer members in
         * order to point at the appropriate positions in the chunk and thus can used for accessing
         * the contents of the chunk.
         *
         * @param maxDepth	maximum chunking depth for this cube
         * @param chnk	reference to the DiskDirChunk - input+output parameter
         */
        void updateDiskDirChunkPointerMembers(unsigned int maxDepth, DiskDirChunk& chnk)const;

        /**
         * This procedure receives a reference to a DiskDataChunk. It updates its pointer members in
         * order to point at the appropriate positions in the chunk and thus can used for accessing
         * the contents of the chunk.
         *
         * @param maxDepth	maximum chunking depth for this cube
         * @param chnk	reference to the DiskDataChunk - input+output parameter
         */
        void updateDiskDataChunkPointerMembers(unsigned int maxDepth, DiskDataChunk& chnk)const;			

}; //end class AccessManagerImpl

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
	CellMap::CellMap() : chunkidVectp(new vector<ChunkID>) {}
	CellMap::~CellMap() { if (chunkidVectp) delete chunkidVectp; }

	/**
	 * copy constructor
	 */
	CellMap(CellMap const & map){
        	// copy the data
        	if(map.getchunkidVectp())
			chunkidVectp = new vector<ChunkID>(*(map.getchunkidVectp()));
		else
			chunkidVectp = 0;
	}

	/**
	 * overload assignment operator
	 */
	CellMap& operator=(CellMap const& other);
	
	/**
	 * Returns true if cell map is empty
	 */
         bool empty() const{
                return chunkidVectp->empty();
         }//end empty()
	
	/**
	 * This function inserts a new id in the Chunk Id vector. If the id
	 * already exists then it returns false without inserting the id.
	 *
	 * @param	id	the chunk id string
	 */
	 bool insert(const string& id);

	/**
	 * This  routine searches for data points (i.e., chunk ids) in *this CellMap. The
	 * desired data points have coordinates within the ranges defined by the qbox input parameter.
	 * A pointer to a new CellMap containing the found data points is returned.In the returned CellMap
	 * a data point is represented by a chunk id composed of the prefix (input parameter) as a prefix
	 * and the domain corresponding to the data point's cooordinates as a suffix. If no data point is
	 * found  it is returned NULL.
	 *
	 * @param qbox	input parameter defining the rectangle into which the desired data point fall
	 * @param prefix input parameter reoresenting the prefix of the returned chunk ids.
	 */
	CellMap* searchMapForDataPoints(const vector<LevelRange>& qbox, const ChunkID& prefix) const;	 	
	

	/**
	 * get/set
	 */
	const vector<ChunkID>* const getchunkidVectp() const {return chunkidVectp;}
	//void setchunkidVectp(vector<ChunkID>* const chv){chunkidVectp = chv;}

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
	 CostNode(ChunkHeader* const hdr, CellMap* const map);
	 //CostNode(ChunkHeader* const hdr, CellMap* const map): chunkHdrp(hdr),cMapp(map),child() {}
	 //CostNode(ChunkHeader* const hdr, CellMap* const map, vector<CostNode>* const c): chunkHdrp(hdr),cMapp(map),child(c) {}
	
	 /**
	  * Constructor of a cost node wiuthout a CellMap
	  */
	  CostNode(ChunkHeader* const hdr);
	
	 /**	
	  * Destructor
	  */
	~CostNode();
	
	/**
	 * Prints the information stored in a cost tree hanging from root,
	 * into out. Used for debugging.
	 *
	 * @param root	pointer to the tree root
	 * @param maxDepth the maximum (global) chunking depth for this cube
	 * @param out	the output file
	 */
	static void printTree(CostNode* root, int maxDepth, ofstream& out);	
	/**
	 * Calculates the total size for the subtree pointed to by root.
	 * The subtree might also be a single data chunk. The routine
	 * calculates the storage cost for storing this tree in the body of a
	 * DiskBucket structure. Therefore it also considers the cost for the entries
	 * of the internal directory of the bucket.
	 * ***NOTE***: when called, the szBytes parameter should be zero!
	 *
	 * @param root	pointer to the tree root (input parameter)
	 * @param szBytes	the returned size in bytes (input+output parameter)
	 */
	static void calcTreeSize(const CostNode* const root, unsigned int& szBytes); //, unsigned int& szPages);

	/**
	 * Counts the directory chunks that hang under chunk tree costRoot
	 *
	 * @param costRoot	the pointer to the root node
	 * @param maxdepth	the maximum depth of the cube in question
	 * @param total		the returned total containing the number of dir chunks
	 */	
	static void countDirChunksOfTree(const CostNode* const costRoot,
					 unsigned int maxdepth,
					 unsigned int& total);

	/**
	 * Counts the data chunks that hang under chunk tree costRoot
	 *
	 * @param costRoot	the pointer to the root node
	 * @param maxdepth	the maximum depth of the cube in question
	 * @param total		the returned total containing the number of dir chunks
	 */	
	static  void countDataChunksOfTree(const CostNode* const costRoot,
						unsigned int maxdepth,
						unsigned int& total);
	
	/**
	 * Counts the chunks (dir+data) that hang under chunk tree costRoot
	 *
	 * @param costRoot	the pointer to the root node
	 * @param maxdepth	the maximum depth of the cube in question
	 * @param total		the returned total containing the number of dir chunks
	 */		
	static void countChunksOfTree(const CostNode* const costRoot,
					unsigned int maxdepth,
					unsigned int& total);	
	
	/**
	 * This (inline) function receives an input chunk id and returns a pointer to the child CostNode
	 * that has this id. If the id is not found it returns 0 (i.e. NULL).
	 *
	 * @param id	the search-key chunk id
	 */
	CostNode* getchildById(const ChunkID& id)const;
	
	/**
	 * This routine removes from the vector of children-node pointers the entry corresponding
	 * to the pointer pointing at the node with ChunkID equal with id (input parameter).Prior
	 * to removing the entry, it first frees the allocated memory occupied by the pointed to node.
	 *
	 * @param 	id 	Chunk id of the child that we want to remove
	 */
	void removeChildFromVector(const ChunkID& id);	
	
	/**
	 * get/set
	 */
	const ChunkHeader* const getchunkHdrp() const {return chunkHdrp;}
	//void setchunkHdrp(ChunkHeader* const chdr) {chunkHdrp = chdr;}

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
	
	/**
	 * This definiton of a function object is intended for use in the destructor
	 * ~CostNode(), in order to delete the pointers to children nodes from
	 * the "child" data member.
	 * @see "Effective STL", Item 7
	 */	
	 struct DeleteObject{
	 	template<typename T> void operator()(const T* ptr) const { if(ptr) delete ptr; }	 		 	
	 };
	
	/**
	 * Protect from copy constructor	
	 */
	 CostNode(const CostNode& cnode);
	
	/**
	 * Protect from assignment operator
	 */
	CostNode & operator=(const CostNode & other);
	
}; //end class CostNode

inline CostNode* CostNode::getchildById(const ChunkID& id)const
{
	for(vector<CostNode*>::const_iterator iter = child.begin(); iter != child.end(); iter++) {
		if((*iter)->getchunkHdrp()->id == id)
			return *iter;
	}//end for
	return 0;
}// end of CostNode::getchildById

#endif // ACCESS_MANAGER_IMPL_H
