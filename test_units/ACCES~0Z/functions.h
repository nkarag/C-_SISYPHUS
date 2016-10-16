#include "classes.h"
#include "DiskStructures.h"
//****************************************************************************************
//                              FUNCTION DECLARATIONS
//***************************************************************************************

void traverseSingleCostTreeCreateChunkVectors(
       			unsigned int maxDepth,
       			unsigned int numFacts,
       			const CostNode* const costRoot,
       			const string& factFile,
       			const BucketID& bcktID,
       			vector<DirChunk>* &dirVectp,
       			vector<DataChunk>* &dataVectp,
       			const TreeTraversal_t howToTraverse = breadthFirst);						

void placeChunksOfSingleTreeInDiskBucketBody(
		unsigned int maxDepth,
		unsigned int numFacts,
		const vector<DirChunk>* const dirVectp,
		const vector<DataChunk>* const dataVectp,
		DiskBucket* const dbuckp,
		char* &nextFreeBytep,
		const TreeTraversal_t howToTraverse = breadthFirst);
        		       			       			
void descendBreadth1stCostTree(unsigned int maxDepth, unsigned int numFacts,
                                const CostNode* const costRoot,
                                const string& factFile,
                                const BucketID& bcktID,
                                queue<CostNode*>& nextToVisit,
                                vector<DirChunk>* const dirVectp,
                                vector<DataChunk>* const dataVectp);


void descendDepth1stCostTree(unsigned int maxDepth, unsigned int numFacts,
                                const CostNode* const costRoot,
                                const string& factFile,
                                const BucketID& bcktID,
                                vector<DirChunk>* const dirVectp,
                                vector<DataChunk>* const dataVectp);

void _storeBreadth1stInDiskBucket(unsigned int maxDepth, unsigned int numFacts,
      	 const vector<DirChunk>* const dirVectp, const vector<DataChunk>* const dataVectp,
      	 DiskBucket* const dbuckp, char* &nextFreeBytep);

void _storeDepth1stInDiskBucket(unsigned int maxDepth, unsigned int numFacts,
		 const vector<DirChunk>* const dirVectp, const vector<DataChunk>* const dataVectp,
		 DiskBucket* const dbuckp, char* &nextFreeBytep);

DiskDirChunk* dirChunk2DiskDirChunk(const DirChunk& dirchnk, unsigned int maxDepth);

DiskDataChunk* dataChunk2DiskDataChunk(const DataChunk& datachnk, unsigned int numFacts,
         					unsigned int maxDepth);

void placeDiskDataChunkInBcktBody(const DiskDataChunk* const chnkp, unsigned int maxDepth,
       	char* &currentp,size_t& hdr_size, size_t& chnk_size);

void placeDiskDirChunkInBcktBody(const DiskDirChunk* const chnkp, unsigned int maxDepth,
		char* &currentp, size_t& hdr_size, size_t& chnk_size);


void createSingleTree(CostNode* &root, unsigned int maxDepth, unsigned int numFacts, unsigned int numDims);
void  createCaseBVect(const CostNode * const root, vector<CaseStruct>& caseBvect);
void printResults(const vector<DirEntry>& resultVect, const vector<CaseStruct>& caseBvect);

void printDiskBucketContents_SingleTreeAndCluster(DiskBucket* const dbuckp, unsigned int maxDepth);
void printDiskDirChunk(ofstream& out, char* const startp);
void printDiskDataChunk(ofstream& out, char* const startp);
void updateDiskDirChunkPointerMembers(DiskDirChunk& chnk);
void updateDiskDataChunkPointerMembers(DiskDataChunk& chnk);
