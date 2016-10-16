/***************************************************************************
                          TestPinClass.h  -  description
                             -------------------
    begin                : Wed Jan 31 2001
    copyright            : (C) 2001 by Nikos Karayannidis
    email                : 
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef TEST_PIN_H
#define TEST_PIN_H

#include <sm_vas.h>
#include <map>
#include <pair.h>
#include <vector>
#include <string>

// ****NOTE****
// SSM compiled with a 8192 disk page, can pin the whole record if it has size < 8116 bytes!
// for PAGESIZE = 8192 bucket reading does not work very well (segm. fault at the end) :-)
//	     use 8116 instead!
const unsigned int PAGESIZE = 16384;//8192;//32768;//8116; // 8K page //65536;
typedef string ChunkID;

/**
 * We want to store in a record (i.e. bucket)
 * 2 sub-trees. Each sub-tree corresponds to some DirChunks
 * and some DataChunks. A Bucket has 2 pointers to vectors:
 * vector<DirChunk>* dirp and vector<DataChunk>* datap
 * We store in dirp, first the DirChunks of the first subtree
 * and then the DirChunks of the 2nd. In the Bucket Header
 * we keep the appropriate index ranges in the vectors corresponding
 * to the chunks of each sub-tree. Moreover, we also keep the byte offsets
 * for each vector in the record body.
 * In this test we will create such a record and then try to read it from
 * disk.
 */

struct DirChunk;
struct DataChunk;
struct DiskBucket;
struct BucketID;

class TestPinClass {
	void test_pin();
	/**
	 * Create some chunk sub-trees
	 */
	void createTestChunks(vector<DirChunk>* dirp,
			vector<DataChunk>* datap,
			map<ChunkID, pair<size_t,size_t> >& mymap,
			serial_t& recordID);
	DiskBucket* cpBucketToHeap(BucketID bid);			
public:

	TestPinClass();
	~TestPinClass() {cout<<"In ~TestPinClass()"<<endl;}
	
};



//struct BucketHeader {
	/**
	 * SSM style id of the SSM record where the bucket resides
	 */
//	serial_t id;
		
	/**
	 * this is the byte offset of the DataChunk section within the body of the shore record.
	 * I.e. if p is a pointer pointing at the beginning of the body, then p + dataChkVectByteOffset
	 * should point at the vector<DataChunk>. This value also represents the actual (i.e. system
	 * related and not cost-formulae related) size of the body section corresponding to the directory chunks.
	 *
	 * @see	BucketHeader::getRealDirSz()
	 */
//	size_t dataChkVectByteOffset;
	
//	map<ChunkID, pair<size_t, size_t> > subtree2index;
	

	/**
	 * default constructor
	 */
//	BucketHeader() : id(), dataChkVectByteOffset(0), subtree2index(){}
	/**
	 * constructor
	 */
//	BucketHeader(const serial_t& bid,  size_t off, map<ChunkID, pair<size_t, size_t> >& m)
//		: id(bid),dataChkVectByteOffset(off), subtree2index(m){}
	/**
	 * copy constructor
	 */
//	BucketHeader(const BucketHeader & bh)
//		: id(bh.id),dataChkVectByteOffset(bh.dataChkVectByteOffset), subtree2index(bh.subtree2index){}
		
	/**
	 * Returns the actual (i.e. system related and not cost-formulae based) size of the directory chunks
	 * stored in the record body.
	 */
//	 size_t getRealDirSz() {
//	 	return dataChkVectByteOffset;
//	 }
	
//	 size_t size_in_bytes() {
//	 	size_t subtree2index_sz = 0;
//	 	for(map<ChunkID, pair<size_t, size_t> >::const_iterator i = subtree2index.begin();
//	 		i!=subtree2index.end(); i++){
		 	
//		 	 subtree2index_sz +=  (i->first.getcid().size()*sizeof(char)) + (2*sizeof(size_t));
//	 	 }
//	 	 return (subtree2index_sz + sizeof(serial_t) + sizeof(size_t));
//	 }
	
//};

struct DiskChunkHeader {
	//typedef chunkType enum{dir, data};
	
	//chunkType type;
	enum {idmaxsize = 10};
	char no_measures; // if (no_measures==0) then this
				  //  is a dir chunk
        short unsigned int no_entries; // # entries of this chunk					
	char id[idmaxsize];	
};

struct DirChunk {
        struct Entry {
        	serial_t bucketid;
        	short int index;};
        	
	DiskChunkHeader	hdr;
	vector<Entry> entries; // this chunk's entries        	        	
};

struct DataChunk {
	//enum {maxfacts = 2};
        struct Entry {
        	//float measures[maxfacts];};
        	vector<float> measures;};

	DiskChunkHeader	hdr;        		
	vector<Entry> entries; // this chunk's entries        	        			
};
/*
class ReadChunk {
protected:
       	const DiskChunkHeader* const hdrp; // read pointers, pointing to pinned bucket body
       	const char* const entryp;
public:
	ReadChunk(const DiskChunkHeader* h, const char* e): hdrp(h),entryp(e) {}
	virtual ~ReadChunk() {}
	virtual void* getEntry(int offset) = 0;		      	
};

class ReadDirChunk : public ReadChunk {
public:
	ReadDirChunk(const DiskChunkHeader* h, const char* e): ReadChunk(h,e){}
	virtual ~ReadDirChunk() {}
	virtual DirEntry* getEntry(int offset);		
};

class ReadDataChunk : public ReadChunk {
public:
	ReadDataChunk(const DiskChunkHeader* h, const char* e): ReadChunk(h,e){}
	virtual ~ReadDataChunk() {}
	virtual DataEntry* getEntry(int offset);		
};
*/
struct BucketID{
	serial_t id; //SSM record id
};

struct DiskBucketHeader{
	/**
	 * SSM style id of the SSM record where the bucket resides
	 */
	BucketID id;
	BucketID previous; // in order to exploit a "good" bucket order different than the
	BucketID next;     // one used by the SSM for the records of a file

	unsigned int no_chunks; // # of chunks stored. This gives the following info:
				// 	- number of entries in the bucket directory
				//	- next available index entry (i.e chunk slot in the bucket)
	size_t 	next_offset;	// the next available byte offset to store a chunk
	size_t freespace; // # of free bytes (contiguous space)
};

struct DiskBucket {
        typedef unsigned int dirent_t;
        enum {bodysize = PAGESIZE-sizeof(DiskBucketHeader)-sizeof(dirent_t*)};

	DiskBucketHeader hdr; // header
	
	char body[bodysize]; // the body of the bucket
	
	// initialize directory pointer to point one beyond last byte of body
	// the bucket directory (grows backwards!)
	dirent_t* offsetInBucket;	
};

class ReadBucket{
	/**
	 * pointer to pinned disk bucket.
	 * NOTE: this points always to the body of the very 1st page
	 * pinned from the bucket record (in case of pin_i::pin_all()
	 * returns 0)
	 *
	 */
	const DiskBucket* const dbuckp;
	
	/**
	 * Copy a fragmented chunk from two SSM pages to continuous space in heap
	 */
	const char* cpFragChunkToHeap(const char*startp, DiskBucket::dirent_t startoffs, DiskBucket::dirent_t endoffs, pin_i& curr_pg_hdl);
	
public:
	ReadBucket(const DiskBucket* b): dbuckp(b) {}
	~ReadBucket() {delete dbuckp;}
	
	//ReadChunk* getChunk(unsigned int slot) {}
	void readHeader();
	// this version of readBody tries to read the bucket
	// page by page in the SSM buffer pool
	// *NOTE* it does not WORK!! :-)
	void readBody(bool whole_buck_pinned, size_t fst_pg_length, pin_i& fst_pg_hdl);//pin_i& fstpage_hdl);
	void readBody();
};



#endif //TEST_PIN_H
