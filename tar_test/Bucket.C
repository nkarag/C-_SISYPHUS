/***************************************************************************
                          Bucket.C  -  description
                             -------------------
    begin                : Fri Oct 13 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/

#include "Bucket.h"
#include "Chunk.h"
#include "definitions.h"

Bucket::Bucket(BucketHeader* const h)
	:hdrp(h), dirp(0), datap(0) {
 	// Check Size Constraint
/* 	try {
		checkSize();
	}
	catch(...) {
		throw;
	}
*/	
}

Bucket::Bucket(BucketHeader* const h, vector<DirChunk>* const dir, vector<DataChunk>* const data)
	:hdrp(h), dirp(dir), datap(data)
{
 	// Check Size Constraint
 /*	try {
		checkSize();
	}
	catch(...) {
		throw;
	}
*/	
}

Bucket::~Bucket(){
	delete hdrp;
	delete datap;
	delete dirp;	
}

Bucket const& Bucket::operator=(Bucket const& other)
{
	if(this != &other) {
		// deallocate current data
		delete hdrp;
		delete datap;
		delete dirp;
		// duplicate other's data
		hdrp = new BucketHeader(*(other.gethdrp()));		
		datap = new vector<DataChunk>(*(other.getdatap()));		
		dirp = new vector<DirChunk>(*(other.getdirp()));
	}
	return (*this);
}

void Bucket::checkSize()
{
       	// test Bucket Size constraints
       	if(hdrp->bodySz > PAGE_SIZE)
       		throw ExceptionBcktOverflow();
       	if(hdrp->bodySz < BCKT_THRESHOLD)
       		throw ExceptionBcktHdOverflow();
       	if(hdrp->hdrSz > PAGE_SIZE)
       		throw ExceptionBcktUnderflow();
}
