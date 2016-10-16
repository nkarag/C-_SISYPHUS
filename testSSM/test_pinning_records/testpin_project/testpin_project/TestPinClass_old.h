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

class TestPinClass {
	void test_pin();
public:

	TestPinClass();
	~TestPinClass() {cout<<"In ~TestPinClass()"<<endl;}
	
};

/**
 * This class represents a chunk id. i.e. the unique string identifier that is
 * derived from the interleaving of the member-codes of the members of the pivot-set levels
 * that define this chunk.
 * @author: Nikos Karayannidis
 */
class ChunkID {

private:
	string cid;

public:
	/**
	 * Default Constructor of the ChunkID
	 */
	ChunkID() : cid(""){ }

	/**
	 * Constructor of the ChunkID
	 */
	ChunkID(const string& s) : cid(s) {}
	/**
	 * copy constructor
	 */
	ChunkID(const ChunkID& id) : cid(id.getcid()) {}
	/**
	 * Destructor of the ChunkID
	 */
	~ChunkID() { }
	
	const string& getcid() const {return cid;}

	friend bool operator==(const ChunkID& c1, const ChunkID& c2){
	 	return (c1.cid == c2.cid);
	}
	
	
	friend bool operator<(const ChunkID& c1, const ChunkID& c2){
	 	return (c1.cid < c2.cid);
	}
};

struct BucketHeader {
	/**
	 * SSM style id of the SSM record where the bucket resides
	 */
	serial_t id;
		
	/**
	 * this is the byte offset of the DataChunk section within the body of the shore record.
	 * I.e. if p is a pointer pointing at the beginning of the body, then p + dataChkVectByteOffset
	 * should point at the vector<DataChunk>. This value also represents the actual (i.e. system
	 * related and not cost-formulae related) size of the body section corresponding to the directory chunks.
	 *
	 * @see	BucketHeader::getRealDirSz()
	 */
	size_t dataChkVectByteOffset;
	
	map<ChunkID, pair<size_t, size_t> > subtree2index;
	

	/**
	 * default constructor
	 */
	BucketHeader() : id(), dataChkVectByteOffset(0), subtree2index(){}
	/**
	 * constructor
	 */
	BucketHeader(const serial_t& bid,  size_t off, map<ChunkID, pair<size_t, size_t> >& m)
		: id(bid),dataChkVectByteOffset(off), subtree2index(m){}
	/**
	 * copy constructor
	 */
	BucketHeader(const BucketHeader & bh)
		: id(bh.id),dataChkVectByteOffset(bh.dataChkVectByteOffset), subtree2index(bh.subtree2index){}
		
	/**
	 * Returns the actual (i.e. system related and not cost-formulae based) size of the directory chunks
	 * stored in the record body.
	 */
	 size_t getRealDirSz() {
	 	return dataChkVectByteOffset;
	 }
	
	 size_t size_in_bytes() {
	 	size_t subtree2index_sz = 0;
	 	for(map<ChunkID, pair<size_t, size_t> >::const_iterator i = subtree2index.begin();
	 		i!=subtree2index.end(); i++){
		 	
		 	 subtree2index_sz +=  (i->first.getcid().size()*sizeof(char)) + (2*sizeof(size_t));
	 	 }
	 	 return (subtree2index_sz + sizeof(serial_t) + sizeof(size_t));
	 }
	
};

struct DirChunk {
	serial_t bucketid;
	unsigned int index;
	
	size_t size_in_bytes() {
		return (sizeof(serial_t)+sizeof(unsigned int));
	}
};

struct DataChunk {
	float measure;
	
	size_t size_in_bytes() {
		return (sizeof(float));
	}
	
};

class Bucket {
public:
	/**
	 * Default constructor
	 */
	 Bucket(): hdrp(0), dirp(0), datap(0) {}
	/**
	 * Constructor with no vectors
	 */
	Bucket(BucketHeader* const h);
	/**
	 * constructor with vectors
	 */
	Bucket(BucketHeader* const h, vector<DirChunk>* const dir, vector<DataChunk>* const data)
	        	:hdrp(h), dirp(dir), datap(data) {}
        ~Bucket(){
        cout<<"Beginning of ~Bucket"<<endl;
               delete hdrp;
        cout<<"hdrp passed"<<endl;
               delete dirp;
        cout<<"dirp passed"<<endl;               	
               delete datap;
        cout<<"datap passed"<<endl;
        cout<<"End of ~Bucket"<<endl;
        }
	/**
	 * overloaded assignement operator
	 */
        Bucket const& Bucket::operator=(Bucket const& other)
        {
               if(this != &other) {
               	// deallocate current data
               	delete hdrp;
               	delete dirp;
               	delete datap;               	
               	// duplicate other's data
               	hdrp = new BucketHeader(*(other.gethdrp()));		
               	datap = new vector<DataChunk>(*(other.getdatap()));		
               	dirp = new vector<DirChunk>(*(other.getdirp()));
               }
               return (*this);
        }
	
	// get/set
	const BucketHeader* gethdrp() const {return hdrp;}
	void sethdrp(BucketHeader* const h) {hdrp = h;}
	const vector<DirChunk>* getdirp() const {return dirp;}
	void setdirp(vector<DirChunk> * const dir) {dirp = dir;}
	const vector<DataChunk>* getdatap() const {return datap;}
	void setdatap(vector<DataChunk> * const data) {datap = data;}
private:
	/**
 	 * This is the bucket header pointer.
	 */
	BucketHeader* hdrp;
	/**
	 * This is the bucket body, containing chunks. In our current design the length
	 * of the body SHOULD NOT EXCEED THE SIZE OF A DISK PAGE.
	 * The body is split in two parts: one for directory chunks and one for data chunks.
         * @see PAGE_SIZE at defintions.h
	 */
	vector<DirChunk>* dirp;
	vector<DataChunk>* datap;	
};


#endif //TEST_PIN_H
