/***************************************************************************
                          DiskStructures.C  -  description
                             -------------------
    begin                : Mon Feb 11 2002
    copyright            : (C) 2002 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/
#include "DiskStructures.h"
#include "Chunk.h"
#include "Exceptions.h"

DiskChunkHeader::DiskChunkHeader(): local_depth(Chunk::NULL_DEPTH), next_local_depth(false), chunk_id(0), oc_range(0)
{
}//DiskChunkHeader::DiskChunkHeader()

int DiskChunkHeader::getNoOfDomainsFromDepth(short  int depth, short int local_depth)
{
        int noDomains;
        if(local_depth == Chunk::NULL_DEPTH)
        	noDomains = depth - Chunk::MIN_DEPTH;        	
        else {// local_depth >= MIN_DEPTH
        	//ASSERTION 1
        	if(!(local_depth >= Chunk::MIN_DEPTH))
        		throw GeneralError(__FILE__, __LINE__, "DiskChunkHeader::getNoOfDomainsFromDepth ==> ASSERTION 1: invalid local depth in chunk.\n");
        	noDomains = (depth - Chunk::MIN_DEPTH)+(local_depth - Chunk::MIN_DEPTH);		
        }//end else

        return noDomains;
}//DiskChunkHeader::getNoOfDomainsFromDepth


