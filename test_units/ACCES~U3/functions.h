#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <string>

#include "classes.h"
#include "DiskStructures.h"

void createDataChunksOfRegion(
                                unsigned int max_depth, //input
                                unsigned int numFacts,  //input
                                unsigned int numDims,   //input
                                vector<CostNode*>& dataChunksOfRegion //output
                                );

CostNode* createCostNode(unsigned int maxDepth, unsigned int numFacts, unsigned int numDims,
                        string& chunkid, int totcells, int rlcells, vector<LevelRange>& vrange);

void printResult(map<ChunkID, DirEntry>& resultMap);

void printDiskBucketContents_SingleTreeAndCluster(DiskBucket* const dbuckp, unsigned int maxDepth);

void printDiskDirChunk(ofstream& out, char* const startp);

void printDiskDataChunk(ofstream& out, char* const startp);

void updateDiskDirChunkPointerMembers(DiskDirChunk& chnk);

void updateDiskDataChunkPointerMembers(DiskDataChunk& chnk);			


void descendDepth1stCostTree(	 unsigned int maxDepth,
				 unsigned int numFacts,
				 const CostNode* const costRoot,
				 const string& factFile,
				 const BucketID& bcktID,
				 vector<DirChunk>* const dirVectp,
				 vector<DataChunk>* const dataVectp);
				
void placeSingleDataChunkInDiskBucketBody(
			unsigned int maxDepth,
			unsigned int numFacts,
			const DataChunk& datachunk,
			DiskBucket* const dbuckp,
			char* &nextFreeBytep);        							
			
DiskDataChunk* dataChunk2DiskDataChunk(const DataChunk& datachnk, unsigned int numFacts,
					unsigned int maxDepth);			

void placeDiskDataChunkInBcktBody(const DiskDataChunk* const chnkp, unsigned int maxDepth,
		char* &currentp,size_t& hdr_size, size_t& chnk_size);							
		
#endif //FUNCTIONS_H