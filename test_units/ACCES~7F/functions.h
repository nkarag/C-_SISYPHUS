/***************************************************************************
                          functions.h  -  description
                             -------------------
    begin                : Wed Sep 26 2001
    copyright            : (C) 2001 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/
#ifndef FUNCTIONS_H
#define FUNCTIONS_H


#include "classes.h"
#include "DiskStructures.h"

const DataChunk* createDataChunk(unsigned int maxDepth, unsigned int numFacts);

void printDiskBucketContents_SingleTreeAndCluster(DiskBucket* const dbuckp, unsigned int maxDepth);

void placeDiskDataChunkInBcktBody(const DiskDataChunk* const chnkp, unsigned int maxDepth,
			char* &currentp,size_t& hdr_size, size_t& chnk_size);

DiskDataChunk* dataChunk2DiskDataChunk(const DataChunk& datachnk, unsigned int numFacts,
         					unsigned int maxDepth);


/**
* This procedure receives a byte pointer that points at the beginning of a DiskDirChunk stored
* in the body of a DiskBucket structure. Also it receives an output file stream that it uses in
* order to print the contents of the chunk. Also, prior to printing it updates the pointer members
* of the chunk in order to point at the appropriate location in the chunk.
*
* @param out	the output file stream - input parameter.
* @param startp	byte pointer, pointer at the beginning of a DiskDirChunk - input+output parameter
*/
void printDiskDirChunk(ofstream& out, char* const startp);

/**
* This procedure receives a byte pointer that points at the beginning of a DiskDataChunk stored
* in the body of a DiskBucket structure. Also it receives an output file stream that it uses in
* order to print the contents of the chunk. Also, prior to printing it updates the pointer members
* of the chunk in order to point at the appropriate location in the chunk.
*
* @param out	the output file stream - input parameter.
* @param startp	byte pointer, pointer at the beginning of a DiskDataChunk - input+output parameter
*/
void printDiskDataChunk(ofstream& out, char* const startp);

/**
* This procedure receives a reference to a DiskDirChunk. It updates its pointer members in
* order to point at the appropriate positions in the chunk and thus can used for accessing
* the contents of the chunk.
*
* @param chnk	reference to the DiskDirChunk - input+output parameter
*/
void updateDiskDirChunkPointerMembers(DiskDirChunk& chnk);

/**
* This procedure receives a reference to a DiskDataChunk. It updates its pointer members in
* order to point at the appropriate positions in the chunk and thus can used for accessing
* the contents of the chunk.
*
* @param chnk	reference to the DiskDataChunk - input+output parameter
*/
void updateDiskDataChunkPointerMembers(DiskDataChunk& chnk);

#endif //FUNCTIONS_H
