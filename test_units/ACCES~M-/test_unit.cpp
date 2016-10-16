#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <strstream>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <climits>

#include "help.h"

const unsigned int PAGESIZE = 16384;

struct DiskBucket {
	/**
	 * Define the type of a directory entry
	 */
        typedef unsigned int dirent_t;

        /**
         * Size of the body of the bucket.
         */
        //enum {bodysize = PAGESIZE-sizeof(DiskBucketHeader)-sizeof(dirent_t*)};
        static const unsigned int bodysize = PAGESIZE-sizeof(dirent_t*);

        /**
         * Minimum bucket occupancy threshold (in bytes)
         */
        static const unsigned int BCKT_THRESHOLD = int(bodysize*0.5);
	
	/**
	 * The body of the bucket. I.e. where the chunks and the
         * directory entries will be stored.
	 */
	char body[bodysize];
	
	/**
	 * The bucket directory (grows backwards!).
	 * (Initialize this directory pointer to point one beyond last byte of body).
	 * The chunk at slot x (where x = 0 for the 1st chunk) resides at the byte offset:
	 * offsetInBucket[-x-1]
	 */
	dirent_t* offsetInBucket;	
}; //end of DiskBucket

struct DiskDirChunk {
	/**
	 * Define the type of a directory chunk entry.
	 */
        struct Entry {
        	BucketID bucketid;
        	unsigned short chunk_slot;
        };
        typedef Entry DirEntry_t;
};


class AccessManager {
public:
	/**
	 * AccessManager constructor
	 */
	AccessManager()	{}

	/**
	 * AccessManager destructor
	 */
	~AccessManager() {}

	
	 /**
	  * This function returns true only if the input values correspond to a data chunk
	  *
	  * @param	depth	the global depth (input parameter)
	  * @param	local_depth	the local depth (input parameter)
	  * @param	next_flag	next flag in chunk header (input parameter)
	  * @param max_depth	the maximum chunking depth (input parameter)
	  */
	  static bool isDataChunk(int depth, int local_depth, bool next_flag, int max_depth){
	        return true;
	  /*
               	if(depth !=  max_depth)
               		return false; //not a data chunk
               	else {//global depth == maxDepth
                        if( 	(local_depth == Chunk::NULL_DEPTH) ||
                               	(local_depth > Chunk::MIN_DEPTH && next_flag == false)
                        )
               	       		return true; //is a data chunk
			return false;               	       	
        	}//end else
           */	
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
	  }//IsDirChunk()
	
	 /**
	  * This function checks whether the size of a chunk (dir or data) exceeds the
	  * free space of a bucket. In this case the chunk can be characterized as a large
	  * chunk and the function returns true, otherwise returns false.
	  *
	  * @param size the input size
	  */	  	
	 static bool isLargeChunk(size_t size) {
	        return true;
	        /*
	 	// for storing a single chunk in a diskBucket we will need also one entry
	 	// in the internal bucket directory
	 	return (
	 		size > (DiskBucket::bodysize - sizeof(DiskBucket::dirent_t))
	 		);
	 	*/
	 }//isLargeChunk

private:
//________________________________ TYPEDEFS ____________________________________________
	/**
	 * Enumeration to denote the alternative cost tree traversal methods and respective
	 * storage alternatives of chunks into a single bucket. Possible values correspond
	 * to a depth first traversal and a breadth first traversal.
	 */        				 	
        typedef enum {depthFirst,breadthFirst} TreeTraversal_t ;

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
	
//__________________________ CLASS/STRUCT DEFINITIONS ____________________________________________________	
	
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
		TreeTraversal_t how_to_traverse;        	
		
		/**
		 * The token of the method used for storing large chunks that cannot fit in a single DiskBucket
		 */								
		largeChunkMethodToken_t large_chunk_resolution;
		
		/**
		 * The default constructor initializes parameters with default values.
		 */
		CBFileConstructionParams():clustering_algorithm(simple),
					   how_to_traverse(breadthFirst),
					   large_chunk_resolution(equigrid_equichildren) {}
			
		~CBFileConstructionParams(){}
		
		/**
		 * Initialize construction parameters from a configuration file
		 */
		initParamsFromFile(const string& configFileName) {}
					
	}; //end struct CBFileConstructionParams
    				
    	    	
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
       		EquiGrid_EquiChildren(int e, int d, vector<LevelRange>& rangeVect);
       		
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
               	 * @param cbinfo		all schema and system-related info about the cube. (input)
        	 * @param costRoot	the root of the cost sub-tree. (input) 	
        	 * All the rest parameters are identical to the ones appearing in putChunksintoBuckets and are included in this
        	 * function because they will be passed to the latter. All that operator() will do is to replace the input CostNode
        	 * with a new CostNode tree corresponding to further chunking of the initial large chunk; then, putChunksIntoBuckets is
        	 * called to take care all the rest.
       		 */
  		void operator()(	
					const CubeInfo& cbinfo, //input
        				const CostNode* const costRoot,  //input
        				unsigned int where2store,      //input
        				const string& factFile,        //input
        				vector<DirChunk>* rtBcktDirVectp,     //output
        				DirEntry& returnDirEntry,      //output
        				const CBFileConstructionParams& constructionParams //input
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
	
	friend class EquiGrid_EquiChildren;


//______________________ PRIVATE DATA MEMBERS __________________________________________________________________________

    	
//______________________ PRIVATE METHOD DECLARATIONS ____________________________________________________________________    	
    	    	    	                									     	
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
	static void putChunksIntoBuckets(const CubeInfo& cbinfo,
				const CostNode* const costRoot,
				unsigned int where2store,
				const string& factFile,
				vector<DirChunk>* rtBcktDirVectp,
				DirEntry& returnDirEntry,
				const CBFileConstructionParams& constructionParams
				);				       					
								
};//class AccessManager

CostNode* createCostNode(unsigned int maxDepth, unsigned int numFacts, unsigned int numDims);

int main(){
        //create large data chunk CostNode
	// maxdepth numfacts init
	unsigned int maxDepth = 3;
	unsigned int numFacts = 2;
        unsigned int numDims = 2;

        //create a CubeInfo
        CubeInfo cinfo;
        cinfo.setmaxDepth(maxDepth);
        cinfo.setnumFacts(numFacts);

        //create a cost Node corresponding to a data chunk
        CostNode* costRoot = createCostNode(maxDepth,numFacts,numDims);

        //create EquiGrid_EquiChildren  instance
	// find the maximum number of directory entries (E) that fit in a bucket
	// Since this bucket will store only a single DirChunk,it will occupy one entry in the bucket internal directory
	// (DiskBucket::direntry_t)
	unsigned int maxDirEntries = ( DiskBucket::bodysize - sizeof(DiskBucket::dirent_t) )/sizeof(DiskDirChunk::DirEntry_t);
	cout << "maxDirEntries = " << maxDirEntries << ". Accept? (y/n): "<<endl;
	string ans;
	cin >> ans;
	if(ans[0] != 'y') {
                cout << "Give your maxDirEntries: "<<endl;
                cin >> maxDirEntries;
	}//end if
		
   	//call equi-grid-equi-children method
       	AccessManager::EquiGrid_EquiChildren method(maxDirEntries, numDims, const_cast<ChunkHeader*>(costRoot->getchunkHdrp())->vectRange);

        //call operator()
       	unsigned int where2store = 0;
       	string factFile = string("mpouroumpourou");
       	vector<DirChunk>* rtBcktDirVectp = 0;
       	DirEntry returnDirEntry;
	AccessManager::CBFileConstructionParams constructionParams;
   	try{
   		method(cinfo,costRoot,where2store,factFile,rtBcktDirVectp,returnDirEntry,constructionParams);
   	}
       	catch(GeneralError& error) {
       		GeneralError e("main ==> ");
       		error += e;
       		cerr << error <<endl;
       	}			

}//end main

CostNode* createCostNode(unsigned int maxDepth, unsigned int numFacts, unsigned int numDims)
{
       //create chunk header
        ChunkHeader* hdrp = new ChunkHeader;
        //      chunk id
        hdrp->id.setcid(string("0|0.0|0.0|-1"));
        //      depth
        hdrp->depth = maxDepth;
        //      number of dimensions
        hdrp->numDim = numDims;
        //      total number of cells
        hdrp->totNumCells = 1000;
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
       		string m("createCostNode ==> ");
       		m += msg;
                	throw m;
       	}
       	// insert order-code ranges per dimension level
        hdrp->vectRange.push_back(LevelRange(string("Location"),string("country"),9,16));
        hdrp->vectRange.push_back(LevelRange(string("Product"),string("type"),29,34));


        //create cell map
        CellMap* clmpp = new CellMap;
        //clmpp->insert(string("0|0.0|0.0|-1.0|0"));
        clmpp->insert(string("0|0.0|0.0|-1.9|30"));
        clmpp->insert(string("0|0.0|0.0|-1.9|33"));
        clmpp->insert(string("0|0.0|0.0|-1.10|30"));
        clmpp->insert(string("0|0.0|0.0|-1.10|31"));
        clmpp->insert(string("0|0.0|0.0|-1.11|32"));
        clmpp->insert(string("0|0.0|0.0|-1.11|33"));
        clmpp->insert(string("0|0.0|0.0|-1.12|29"));
        clmpp->insert(string("0|0.0|0.0|-1.12|31"));
        clmpp->insert(string("0|0.0|0.0|-1.12|34"));
        clmpp->insert(string("0|0.0|0.0|-1.13|32"));
        clmpp->insert(string("0|0.0|0.0|-1.14|31"));
        clmpp->insert(string("0|0.0|0.0|-1.14|33"));
        clmpp->insert(string("0|0.0|0.0|-1.15|31"));
        clmpp->insert(string("0|0.0|0.0|-1.15|32"));
        clmpp->insert(string("0|0.0|0.0|-1.16|30"));

        return new CostNode(hdrp, clmpp);
}//createCostNode


void AccessManager::putChunksIntoBuckets(const CubeInfo& cbinfo,
				const CostNode* const costRoot,
				unsigned int where2store,
				const string& factFile,
				vector<DirChunk>* rtBcktDirVectp,
				DirEntry& returnDirEntry,
				const CBFileConstructionParams& constructionParams
				)
{
        //print new tree
	ofstream out("treeresult.dbug");

        if (!out)
        {
            cerr << "creating file \"treeresult.dbug\" failed\n";
            throw GeneralError("AccessManager::putChunksIntoBuckets ==> creating file \"construction_phase_I.dbug\" failed\n");
        }

        out <<"**************************************************"<<endl;	
        out <<"*	    CHUNK TREE INFORMATION              *"<<endl;
        out <<"**************************************************"<<endl;	
        out <<"\n\n";

        // Start traversing the tree..
        CostNode::printTree(const_cast<CostNode*>(costRoot), out);

}//AccessManager::putChunksIntoBuckets

AccessManager::EquiGrid_EquiChildren::EquiGrid_EquiChildren(int e, int d, vector<LevelRange>& rangeVect): maxDirEntries(e), noDims(d)       		
//precondition:
//      It is assumed that this routine is called on behalf of a *large data chunk*. Therefore, for example
//      it does not allow pseudo levels at all for input chunk (since it is a data chunk).
//processing:
//      calculates the number of members (equal for all dimensions) of the newly inserted levels. Decides
//      which of these new levels will be pseudo levels. Also calculates the maximum number of children
//      (equal for all members of the same dimension) of a member of a new level.
//postcondition:
//      All data members have valid values.
{
       		
	//find the number of artificial partitions (i.e., No of members of new level) along each dimension (m = floor(E**1/N))
	// this will be the same for all dimensions (equi-grid)       		
       	noMembersNewLevel = int( ::floor( ::pow(double(maxDirEntries),1.0/double(noDims)) ) );
       			
       	//find the maximum number of children under each new member (c = N/m), equal for all members of a dimension (equi-children).
       	// One entry per dimension
       	maxNoChildrenPerDim.reserve(noDims);//the order of the dimensions in this vector are the same with the interleaving order
       	noPseudoLevels = 0; //init pseudo level counter
       	vector<unsigned int> noMembersGrainVect(noDims); //init vector to hold no of grain members per dim                       	
       	for(int i=0; i<noDims; i++){
       		//if this not a pseudo level
       		if(rangeVect[i].lvlName.find("Pseudo-level") == string::npos){
               		// calc no of members on each dimension at the grain level of the original chunk                       		
        		noMembersGrainVect[i] = rangeVect[i].rightEnd - rangeVect[i].leftEnd + 1;
                        		
        		// calc the max number of children at the grain level that a parent member can have
        		// if the new members are fewer than the existing members at the grain level
        		if(noMembersGrainVect[i] >= noMembersNewLevel){
        			//then the hierarchy is meaningful
                		maxNoChildrenPerDim.push_back(int(::ceil(double(noMembersGrainVect[i])/double(noMembersNewLevel))));
                	}//end if
                	else {
                		//there is no meaning in creating a hierarchy along this dimension; instead create a pseudo level
                		maxNoChildrenPerDim.push_back(0);
                		noPseudoLevels++; //one more pseudo level
                	}//end else
        	}//end if
        	else {  //this is a Pseudo level
        		//since we apply (for now) this method only to large DATA chunks (and not dir chunks)
        		//there shouldn't be any pseudo level at the data level
        		throw GeneralError("AccessManager::EquiGrid_EquiChildren() ==> Pseudo level encountered in data chunk\n");
        	}//end else
       	}//for
                       	
       	//If all new levels will be pseudo levels
       	if(noPseudoLevels == noDims){                       	
                //then we have to reduce the number of members of the new levels
                // we will make it equal with the minimum number of grain level members for each dimension
                //noMembersNewLevel = minimum(noMembersGrain)  such that minimum(noMembersGrain) != 1

                // find the minimum number of grain members in the original chunk.
                vector<unsigned int>::const_iterator minimumMembersGrainIter = min_element(noMembersGrainVect.begin(), noMembersGrainVect.end());
                //if the min number is not equal with 1
                if(*minimumMembersGrainIter != 1) {
                        // The *new* number of new members inserted will be:
                        noMembersNewLevel = *minimumMembersGrainIter;
                        //Now, the maximum number of children per dim becomes:
                        for(int i=0; i<noDims; i++){
                                maxNoChildrenPerDim[i] = int(::ceil(double(noMembersGrainVect[i])/double(noMembersNewLevel)));
                        }//for
                        // No pseudo levels now
                        noPseudoLevels = 0;
                }//end if
                else {//else if min number equals 1
                        //find all dimensions with only 1 member and make them pseudo levels
                        noPseudoLevels = 0;
                        for(int i=0; i<noDims; i++){
                                if(noMembersGrainVect[i] == 1){
                                        //then this must be a
                                        maxNoChildrenPerDim[i] = 0; //pseudo level
                                        noPseudoLevels++;
                                        //also put maxint in then num of members vect in order to get it out of the way
                                        noMembersGrainVect[i] = UINT_MAX; //numeric_limits<unsigned int>::max();
                                }//end if
                        }//for
                        if(noPseudoLevels == noDims)
                                throw GeneralError("AccessManager::EquiGrid_EquiChildren() ==> Found large data chunk consisting of only ONE(!!!) cell. Sorry, cant handle this case, I think you should consider a re-design of your cube!!! :-)");
                        //Now that the 1's are out of the way,
                        //find the minimum No of grain members in the original chunk that is != 1
                        minimumMembersGrainIter = min_element(noMembersGrainVect.begin(), noMembersGrainVect.end());
                        // The *new* number of new members inserted will be:
                        noMembersNewLevel = *minimumMembersGrainIter;
                        //Now, the maximum number of children per dim becomes:
                        for(int i=0; i<noDims; i++){
                                //if not a pseudo level
                                if(maxNoChildrenPerDim[i] != 0)
                                        maxNoChildrenPerDim[i] = int(::ceil(double(noMembersGrainVect[i])/double(noMembersNewLevel)));
                        }//for
                }//end else
       	} //end if
                       	
       	cout << "noMembersNewLevel = " << noMembersNewLevel << endl;
       	cout << "maxNoChildrenPerDim: ";
       	for(int i=0; i<noDims; i++){
       	        cout << maxNoChildrenPerDim[i] <<", ";
       	}//for
       	cout << endl;
       	cout << "noPseudoLevels = " << noPseudoLevels << endl;
}//end AccessManager::EquiGrid_EquiChildren::EquiGrid_EquiChildren()


void AccessManager::EquiGrid_EquiChildren::operator()(
					const CubeInfo& cbinfo, //input
        				const CostNode* const costRoot,  //input
        				unsigned int where2store,      //input
        				const string& factFile,        //input
        				vector<DirChunk>* rtBcktDirVectp,     //output
        				DirEntry& returnDirEntry,      //output
        				const CBFileConstructionParams& constructionParams //input
					)
//precondition:
//	The input CostNode corresponds to a large data chunk. The maximum number of dir entries that fit in a DiskBucket
//	has been calculated and stored as a member in this class function. Also in the constructor of the class function
//	the number of new members inserted (equi-grid) has been calculated and the number of children of these members in
//	the grain level (equi-children) per dimension has also been calculated. Both are kept as data members of the
//	function class EquiGrid_EquiChildren.
//processing:
//	This routine will create a two level costNode tree, which results from the application of the method on the
//	input large data chunk. Then it will call AccessManager::putchunksIntoBuckets in order to take care the storage
//	of the newly created tree into the CUBE File. The AccessManager::putchunksIntoBuckets might recursively call again
//	this routine, if it encounters again a large data chunk.
//postcondition:
//	The chunk corresponding to the CostNode pointed to by costRoot has been instantiated in the form of a DirChunk and
//	is stored in the vector that are stored the root bucket's dir chunks. All the children chunks pointed to by this root are
//	stored in separate buckets. Note that in the case that a child is still a large data chunk this routine will be called
//	recursively (by putChunksIntoBuckets()) and therefore the corresponding DirChunk will be also inserted in the root bucket
//	vector of dir chunks. Finally, a valid directory entry is returned that points to the dirChunk corresponding to costRoot.
{
	unsigned int maxDepth = cbinfo.getmaxDepth();
	unsigned int numFacts = cbinfo.getnumFacts();

	//Assert that this is a large data chunk

	//ASSERTION 1: assert that this is a data chunk
	if(!isDataChunk(costRoot->getchunkHdrp()->depth, costRoot->getchunkHdrp()->localDepth, costRoot->getchunkHdrp()->nextLocalDepth, maxDepth))
		throw GeneralError("AccessManager::EquiGrid_EquiChildren::operator() ==> ASSERTION1: input chunk is NOT a data chunk");

	//ASSERTION 2: assert that the data chunk's size is greater than the free space in the bucket
	if( !AccessManager::isLargeChunk(costRoot->getchunkHdrp()->size) )
		throw GeneralError("AccessManager::EquiGrid_EquiChildren::operator() ==> ASSERTION2: wrong size for large data chunk");

	// create a new chunk header for the input chunk by copying the old one
	// the following members must be updated to reflect the new dir chunk that will be created
	// in the place of the original large data chunk:
	//		localDepth, nextLocalDepth, rlNumCells, totNumCells, vectRange, size.
	ChunkHeader* newHeaderForRootp = new ChunkHeader( *(costRoot->getchunkHdrp()) );

	// Update the local depth and the next field
	// if this is a classic data chunk
	if(newHeaderForRootp->localDepth == Chunk::NULL_DEPTH) {
		//the local depth should take the minimum value
		newHeaderForRootp->localDepth = Chunk::MIN_DEPTH;
		newHeaderForRootp->nextLocalDepth = true; //indicates that another level will follow
	}//end if
	//else if this is an artificially formed data chunk then
	else if(newHeaderForRootp->localDepth > Chunk::MIN_DEPTH && newHeaderForRootp->nextLocalDepth == false){
		// the local depth should remain the same, just the next flag must be set on
		newHeaderForRootp->nextLocalDepth = true;
	}//end else if
	else{ //Something is wrong here!!
		throw GeneralError("AccessManager::EquiGrid_EquiChildren::operator() ==> wrong local depth or next flag value");
	}//end else

	//Update the number of total cells
	//*NOTE* The number of newly inserted  members is the same for all dimensions (equi-grid). Therefore the total num of cells
	//equals with this number raised in the number of dimensions.However we have to exclude the dimensions whose new level
	//will be a pseudo level.
	newHeaderForRootp->totNumCells = int(::pow(double(noMembersNewLevel), double(noDims-noPseudoLevels)));
	
	//Update the new header with the new ranges on each dimension (equal for all dims) for the 1st level
	for(int dimi = 0; dimi < noDims; dimi++){
		//if this does not correspond to a pseudo level
		if(maxNoChildrenPerDim[dimi] != 0){
			newHeaderForRootp->vectRange[dimi].leftEnd = Chunk::MIN_ORDER_CODE;
			newHeaderForRootp->vectRange[dimi].rightEnd = noMembersNewLevel-1;
		}//end if
		else { //then this will be a pseudo level
			newHeaderForRootp->vectRange[dimi].lvlName = string("Pseudo-level");
			newHeaderForRootp->vectRange[dimi].leftEnd = LevelRange::NULL_RANGE;
			newHeaderForRootp->vectRange[dimi].rightEnd = LevelRange::NULL_RANGE;
		}//end else
	}//end for

	//if a cell map does not exist for the input chunk then create one, we will need it next
	if(!costRoot->getcMapp()){
	       	//Scan input file for prefix matches with the chunk id of the original (large) chunk and create
	       	//corresponding cell map
		const_cast<CostNode*>(costRoot)->setcMapp(Chunk::scanFileForPrefix(factFile, newHeaderForRootp->id.getcid(), true));
	}//end if

	// create an empty cell map for the new root
	CellMap* newmapp = new CellMap;

	//create the corresponding CostNode without the children attached yet and an empty cell map
	CostNode* newCostRoot = new CostNode(newHeaderForRootp, newmapp);

       	//now, create and store the new hierarchies per dimension
       	//use a vector of maps (one map per dimension)
       	vector<map<int, LevelRange> >* newHierarchyVectp = new vector<map<int, LevelRange> >(noDims);
       	try{
       		createNewHierarchies(costRoot->getchunkHdrp()->vectRange, *newHierarchyVectp);
       	}
      	catch(GeneralError& error) {
      		GeneralError e("AccessManager::EquiGrid_EquiChildren::operator ==> ");
      		error += e;
      		throw error;
      	}
      	
      	#ifdef DEBUGGING
      	//print new hierarchy
      	cerr << "The artificial hierarchy will be the following: " << endl;
        for(int dimi = 0; dimi < noDims; dimi++) {
                cout << "____ Dim: " << dimi << " ____" << endl;     	
                for(map<int, LevelRange>::const_iterator mapiter = (*newHierarchyVectp)[dimi].begin(); mapiter != (*newHierarchyVectp)[dimi].end(); mapiter++){
                        cout << mapiter->first << " ==> " << "[" << mapiter->second.leftEnd << ", " << mapiter->second.rightEnd << "]" << endl;
                }//for
                cout << endl;      	
        }//for      	
      	#endif

       	// initialize the number of real cells in the parent node
       	const_cast<ChunkHeader*>(newCostRoot->getchunkHdrp())->rlNumCells = 0;

       	//For each cell of the (new) parent cost node (form it from the new ranges):
       	//first initialize cell-coords with the lower-left cell (***NOTE*** pseudo codes included)
       	Coordinates currCellCoords(noDims, vector<DiskChunkHeader::ordercode_t>(noDims));
	for(int dimi = 0; dimi < noDims; dimi++){
		//if this does not correspond to a pseudo level
		if(maxNoChildrenPerDim[dimi] != 0)
			currCellCoords.cVect[dimi] = Chunk::MIN_ORDER_CODE;
		else { //then this will be a pseudo level
			currCellCoords.cVect[dimi] = LevelMember::PSEUDO_CODE;
		}//end else
	}//end for
	//Create a cell with these coordinates, in the data space defined by the parent chunk
	Cell currentCell(currCellCoords, newCostRoot->getchunkHdrp()->vectRange);
       	#ifdef DEBUGGING
       	int numCellsVisited = 0;
       	#endif	
       	do{
       		//loop invariant: in each iteration a cost node corresponding to a child chunk must be created
       		//		  and attached to the parent ONLY if this child is NOT EMPTY, i.e., it contains at least
       		//		  one data point.
       		
		#ifdef DEBUGGING
		cerr << "Current cell is: " << currentCell << ". Is  it the first cell? "<< currentCell.isFirstCell() <<endl;
		numCellsVisited++;		
		#endif
       		
       		// create a cost node for the corresponding child chunk
       		// We will need a chunk header and a cell map:
       		ChunkHeader* childHeaderp = new ChunkHeader;

       		//chunk id
       		//create chunk id domain from the coordinates
       		string newdomain;
       		ChunkID::coords2domain(currentCell.getcoords(), newdomain);
       		//add new domain as a suffix to the parent chunk id
       		 //take the parent id
       		ChunkID childid(newCostRoot->getchunkHdrp()->id.getcid());
       		//and add the new domain as suffix
       		try{
       			childid.addSuffixDomain(newdomain);
       		}
		catch(GeneralError& error){
			GeneralError e("AccessManager::EquiGrid_EquiChildren::operator() ==> ");
			error += e;
                 	throw error;
		}							       		       		       		       		
       			
       		childHeaderp->id.setcid(childid.getcid());
      		
       		#ifdef DEBUGGING
       		cerr << "New chunk id is: "<< childHeaderp->id.getcid() <<endl;
       		#endif
       		
       		//global depth remains the same with the parent
       		childHeaderp->depth = newCostRoot->getchunkHdrp()->depth;
       		
       		//local depth and next flag
       		childHeaderp->localDepth = newCostRoot->getchunkHdrp()->localDepth + 1; //one more than the parent
       		childHeaderp->nextLocalDepth = 	false; // this is a leaf chunk (i.e., data chunk)
       		
       		//number of dimensions
       		childHeaderp->numDim = noDims;
       		       		
       		//range vector for the child chunk:
       		childHeaderp->vectRange.reserve(noDims);
       		//for each coordinate value of the parent node
       		for(int dimindex = 0; dimindex < noDims; dimindex++){
       			// get the corresponding range from the newly created hierarchy
       			childHeaderp->vectRange.push_back( (*newHierarchyVectp)[dimindex][currentCell.getcoords().cVect[dimindex]] );
       		}//end for
       		
       		#ifdef DEBUGGING
       		cerr << "The order code ranges for chunk: " << childHeaderp->id.getcid() << " is:" << endl;
       		for(vector<LevelRange>::const_iterator rngiter = childHeaderp->vectRange.begin(); rngiter != childHeaderp->vectRange.end(); rngiter++, cerr<<" x "){
                        cerr << "[" << rngiter->leftEnd << ", " <<rngiter->rightEnd << "]";       		
       		}//end for
       		cerr << endl;
       		#endif       		
       		
       		// calculate total number of cells as the product of the number of members on each dimension for this chunk
       		int mbproduct = 1; //init product
       		//for each dimension
       		for(int dimindex = 0; dimindex < noDims; dimindex++){
			int nomembers = childHeaderp->vectRange[dimindex].rightEnd - childHeaderp->vectRange[dimindex].leftEnd +1 ;
       			mbproduct *= nomembers;
       		}//end for
       		childHeaderp->totNumCells = mbproduct;

		//create empty cell map for child
       		CellMap* childMapp = 0;
       		
      		// Search original (input) large data chunk's cell map for cells in the range of the child chunk
      		// and create the childs cell map
      		try{
      			childMapp = costRoot->getcMapp()->searchMapForDataPoints(childHeaderp->vectRange, childHeaderp->id);
      		}
		catch(GeneralError& error){
			GeneralError e("AccessManager::EquiGrid_EquiChildren::operator() ==> ");
			error += e;
                 	throw error;
		}
		
		#ifdef DEBUGGING
		cerr << "Printing the data points found in the parent's CellMap\n";
                if(childMapp) {
                        //print new map
                        for(vector<ChunkID>::const_iterator citer = childMapp->getchunkidVectp()->begin(); citer != childMapp->getchunkidVectp()->end(); citer++){
                                cout << citer->getcid() << endl;
                        }//end for
                }//if
                #endif
									       		       		       		       		      		
      		//if no data point was found in the range of this child chunk
      		if(!childMapp){
      			//DO NOT CREATE any cost node for the child
      			delete childHeaderp;
               		//get next cell
               		try{
        	       		currentCell.becomeNextCell();
        	       	}
        		catch(GeneralError& error){
        			GeneralError e("AccessManager::EquiGrid_EquiChildren::operator() ==> ");
        			error += e;
                         	throw error;
        		}      			
      			continue; //next iteration
      		}//end if
      		
      		//else, at least one data point was found
       		       		       		       		       		       		
       		//update number of real cells for child chunk
		childHeaderp->rlNumCells = childMapp->getchunkidVectp()->size();
		
		// calculate size of child chunk
		try{
        		childHeaderp->size = DataChunk::calculateStgSizeInBytes(childHeaderp->depth,
        							  maxDepth,
        							  childHeaderp->numDim,
        							  childHeaderp->totNumCells,
        							  childHeaderp->rlNumCells,
        							  numFacts,
        							  childHeaderp->localDepth);
		}
		catch(GeneralError& error){
			GeneralError e("AccessManager::EquiGrid_EquiChildren::operator() ==> ");
			error += e;
                 	throw error;
		}							       		       		       		       		
       		       			       		
       		//Also, update parent's members:
   		//insert chunk id into parent node cell map
		if(!const_cast<CellMap*>(newCostRoot->getcMapp())->insert(childHeaderp->id.getcid()))
			throw GeneralError("AccessManager::EquiGrid_EquiChildren::operator() ==> double entry in cell map");
		        				
       		//update parent number of real cells in the new header by adding one cell, since this parent cell
       		// points to a non-empty child
		const_cast<ChunkHeader*>(newCostRoot->getchunkHdrp())->rlNumCells++; //add one more cell       		
       			
		// keep the cell map for data chunks derived from artificial chunking. This is necessary, because we wont be
       		// able to created it in a possible future call of scanFileForPrefix (if this is again a large data chunk)
       		// since this would mean that we would have to change the chunk ids in the fact load file to include the extra domains.
       		// Also, in order to retrieve the actual values of the cells from the load file we will need it for artificially chunked
       		// data chunks.Therefore,       				
       		// create cost node for child with cell map
       		CostNode* costNodeChild = new CostNode(childHeaderp, childMapp);
       		
       		//attach cost node to parent
       		const_cast<vector<CostNode*>&>(newCostRoot->getchild()).push_back(costNodeChild);
       		
       		//get next cell
       		try{
	       		currentCell.becomeNextCell();
	       	}
		catch(GeneralError& error){
			GeneralError e("AccessManager::EquiGrid_EquiChildren::operator() ==> ");
			error += e;
                 	throw error;
		}
       	}while(!currentCell.isFirstCell()); //continue while there are still more cells to visit
       	
       	#ifdef DEBUGGING
       	cerr << "Number of cells visited: " << numCellsVisited << endl;		
       	#endif
       	       		
       	//Update the size of the parent node, now that you have the real number of cells.
       	// ***NOTE*** As long as we dont compress dirchunks (current convention) this computation
       	// could also be done before computing the real number of cells, since we dont use it  anyway.
       	// However, in case this convention changes the calculation should take place here       		       	
       	try{
       		const_cast<ChunkHeader*>(newCostRoot->getchunkHdrp())->size = DirChunk::calculateStgSizeInBytes(newCostRoot->getchunkHdrp()->depth,
        							  maxDepth,
        							  newCostRoot->getchunkHdrp()->numDim,
        							  newCostRoot->getchunkHdrp()->totNumCells,
        							  newCostRoot->getchunkHdrp()->localDepth);
       	}
       	catch(GeneralError& error){
       		GeneralError e("AccessManager::EquiGrid_EquiChildren::operator() ==> ");
       		error += e;
                	throw error;
       	}							       		       		       		       		
       	

/*       	update the look-up table which records data chunk cells (i.e., their chunk-ids) that have been moved to a
   	larger depth due to artificial chunking
   	IF such a table does not already exist
   		call appropriate method to create it
   	For each cell of the original large chunk (empty cells included)
   		create new chunk id by inserting the new domain
		insert into B+tree, old chunk id of parent cell as a key and the new one as the associated value  	
*/   	
        //Instead of using a look-up table, the parent-child relationship of the new members and the grain level order codes
        //will be stored in the chunk header of the parent node and then finally in an appropriate form will be stored along the
        // corresponding DiskDirChunk
        const_cast<ChunkHeader*>(newCostRoot->getchunkHdrp())->artificialHierarchyp = newHierarchyVectp;

   	delete costRoot; //free up space, discard input chunk
	
   	//call putChunkIntoBuckets for the new cost-tree
	try {
       		AccessManager::putChunksIntoBuckets(
			cbinfo,newCostRoot,where2store,factFile,rtBcktDirVectp,returnDirEntry,constructionParams );       			
       	}
       	catch(GeneralError& error) {
       		GeneralError e("AccessManager::EquiGrid_EquiChildren::operator() ==> ");
     		error += e;
       		throw error;
       	}   	
       	
   	//From the arguments updated by the previous call, will be returned the vector with root bucket's chunks,
   	//containing the new root that we have created and the
   	//directory entry pointing at this root.
   	
}//AccessManager::EquiGrid_EquiChildren::operator()


void AccessManager::EquiGrid_EquiChildren::createNewHierarchies(
			const vector<LevelRange>& oldRangeVect, //input
			vector<map<int, LevelRange> >& newHierarchyVect //output
			)
//precondition:
//	The EquiGrid_EquiChildren() has calculated the number of artificial partitions (i.e., number of members of new level - equal for all dims))
//	along each dimension, stored in the EquiGrid_EquiChildren::noMembersNewLevel data member and the maximum number of children under each new member,
//      equal for all members of a dimension (equi-children), stored in the EquiGrid_EquiChildren::maxNoChildrenPerDim data member.
//      (NOTE: The latter is only needed for discriminating between pseudo and non-pseudo levels.)
//	The oldRangeVect contains the ranges of the original large (data) chunk. The output parameter
//      newHierarchyVect will store the result hierarchy. When the routine is called it is assumed that the newHierarchyVect has been
//      initialized like this: vector<map<int, LevelRange> > newHierarchyVect(noDims), where nodims is the number of dimensions of the cube.
//processing:
//	compute the parent child relationships between the newly inserted members and the grain-level members for each dimension (oldRangeVect);
//      then,store them in a vector of maps (each map corresponds to dimension). NOTE: if for a dimension the new level inserted is a
//	pseudo level, then in the map we store the original range (i.e., no chunking takes place along this dimension) associated with
//	the PSEUDO_CODE constant. The method used for uniformly distibuting the children among the parents is: iterate parents in a
//      cyclic way and each time assign a child to a parent, until no more children are left.
//postcondition:
//	the vector of maps contains for each dimension, the association of each parent order code to a range of grain level members
{
       	//for each dimension
       	for(int dimi = 0; dimi < noDims; dimi++) {
       		//if this is not a pseudo level in the new chunk
       		if(maxNoChildrenPerDim[dimi] != 0){
       			//init vector which counts the number of children of each parent along this dimension
       			vector<unsigned int> childrenCounterPerParent(noMembersNewLevel, 0);
       			
       			//the total number of children is
       			int totalChildren = oldRangeVect[dimi].rightEnd - oldRangeVect[dimi].leftEnd + 1;
       		
       		    	//loop
       		    	int childrenLeft = totalChildren;
       		    	int parent_index = 0;
       		    	do{
       		    		//assign one child to each parent
       		    		childrenCounterPerParent[parent_index]++;
       		    		//one more child has been assigned
       		    		childrenLeft--;
       		    		//cycle to the next parent
       		    		parent_index = (parent_index + 1) % noMembersNewLevel;
       		    	}while(childrenLeft); //while there are still children to assign
       		    	
       		    	//assert that the sum of all counters equals the total number of children
       		    	if(accumulate(childrenCounterPerParent.begin(), childrenCounterPerParent.end(), 0) != totalChildren)
       		    		throw GeneralError("AccessManager::EquiGrid_EquiChildren::createNewHierarchies ==>ASSERTION: wrong total of children");
       		    	
       		    	//start the actual assignement of ranges to parents
       		    	//init left end: 	left = 1st order code at grain level for this dimension
       		    	int left = oldRangeVect[dimi].leftEnd;
       		        //for each parent

               		for(int parent_code = Chunk::MIN_ORDER_CODE;
               		    parent_code <= int(noMembersNewLevel -1 + Chunk::MIN_ORDER_CODE); //***NOTE*** casting to int necessary if
               		                                                                // parent_code is initialized with a negative value!!!
               		    parent_code++) {

               			// create appropriate grain level range corresponding to its children
               			LevelRange grainrange;       			
                		// left end assignment
                		grainrange.leftEnd = left;
                		// right-end assignment = left + number of children assigned to this parent - 1;
                		grainrange.rightEnd = grainrange.leftEnd + childrenCounterPerParent[parent_code - Chunk::MIN_ORDER_CODE] - 1;
                		//Associate range to parent code
                		(newHierarchyVect[dimi])[parent_code] = grainrange;
                		//move left to the next free child
                		left = grainrange.rightEnd + 1;                		
			}//end for       		        	
        	}//end if
        	else { //this dimension corresponds to a pseudo level in the new chunk
       			// create appropriate grain level range covering ALL children (since no chunking takes place along this dim)
       			LevelRange grainrange;       			
        		// left end assignment
        		grainrange.leftEnd = oldRangeVect[dimi].leftEnd;
        		// right end assignment
        		grainrange.rightEnd = oldRangeVect[dimi].rightEnd;
        		//Associate range to parent code (pseudo code in this case)
        		(newHierarchyVect[dimi])[LevelMember::PSEUDO_CODE] = grainrange;        	
        	}//end else	
	} //end for
}// end AccessManager::EquiGrid_EquiChildren::createNewHierarchies
