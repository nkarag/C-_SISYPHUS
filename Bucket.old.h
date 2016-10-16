/***************************************************************************
                          Bucket.h  -  description
                             -------------------
    begin                : Fri Oct 13 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


/***************************************************************************
                          Bucket.h  -  description
                             -------------------
    begin                : Fri Oct 13 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/

#ifndef BUCKET_H
#define BUCKET_H

#include <sm_vas.h>
#include <vector>
#include <string>

/**
 * Exception for the case of bucket overflow
 */
struct ExceptionBcktOverflow {
	string msg;
	
	ExceptionBcktOverflow() : msg("Bucket's body exceeds the PAGE_SIZE") {}
	ExceptionBcktOverflow(const string& s): msg(s) {}
	ExceptionBcktOverflow(const char* s) : msg(string(s)) {}
	~ExceptionBcktOverflow(){}
};

/**
 * Exception for the case of bucket header overflow
 */
struct ExceptionBcktHdOverflow {
	string msg;
	
	ExceptionBcktHdOverflow() : msg("Bucket's header exceeds the PAGE_SIZE") {}
	ExceptionBcktHdOverflow(const string& s): msg(s) {}
	ExceptionBcktHdOverflow(const char* s) : msg(string(s)) {}
	~ExceptionBcktHdOverflow(){}
};

/**
 * Exception for the case of bucket underflow
 */
struct ExceptionBcktUnderflow {
	string msg;
	
	ExceptionBcktUnderflow() : msg("Bucket's body is less than the minimum bucket occupancy threshold") {}
	ExceptionBcktUnderflow(const string& s): msg(s) {}
	ExceptionBcktUnderflow(const char* s) : msg(string(s)) {}
	~ExceptionBcktUnderflow(){}
};

/**
 * This struct plays the role of the physical id of a bucket. In the current
 * design this is equivalent with the logical id of a shore record.
 * @author: Nikos Karayannidis
 */
struct BucketID {
  	/**
	 * SSM Logical Volume id where this record's file resides
	 */
	//lvid_t vid; //you can retrieve this from the SystemManager
  	/**
	 * SSM logical record id
	 */
	serial_t rid;

	/**
	 * Default constructor
	 */
//	BucketID() : vid(), rid(serial_t::null) {}
//	BucketID(const lvid_t& v, const serial_t& r) : vid(v), rid(r) {}
	/**
	 * copy constructor
	 */
//	BucketID(const BucketID& bid) : vid(bid.vid),rid(bid.rid) {}

	/**
	 * Default constructor
	 */
	BucketID() : rid(serial_t::null) {}
	BucketID(const serial_t& r) : rid(r) {}
	/**
	 * copy constructor
	 */
	BucketID(const BucketID& bid) : rid(bid.rid) {}
	
	/**
	 * Check for a null bucket id
	 */
	bool isnull() {return (rid == serial_t::null);}
	
	friend bool operator==(const BucketID& b1, const BucketID& b2) {
	 	return (b1.rid == b2.rid);
	}

	friend bool operator!=(const BucketID& b1, const BucketID& b2) {
	 	return (b1.rid != b2.rid);
	}		
	
	friend bool operator<(const BucketID& b1, const BucketID& b2) {
	 	return (b1.rid < b2.rid);
	}	
	
	/**
	 * This function creates a new BucketID. This id corresponds to a new SSM record id.
	 * NOTE: that the record is not created yet, just the id.
	 */
	static BucketID createNewID();		
};

/**
 * This struct contains information about a bucket.
 * It can have a size up to a single disk page
 * and it is always pinned in the shore buffer pool when we pin the bucket.
 * @author: Nikos Karayannidis
 */
struct BucketHeader {
	/**
	 * SSM style id of the SSM record where the bucket resides
	 */
	BucketID id;
	
	/**
	 * size of the bucket header (size <= 1 disk page)
	 */
	size_t hdrSz;
	
	/**
	 * size of the DirChunk vector (based on cost- formulas)
	 */
	size_t dirSz;
	
	/**
	 * size of the DataChunk vector (based on cost- formulas)
	 */
	size_t dataSz;

	/**
	 * actual size of the bucket body (based on cost- formulas)
	 * (size <= 1 disk page) bodySz = dirSz+dataSz
	 */
	size_t bodySz;
	
	/**
	 * this is the byte offset of the DataChunk section within the body of the shore record.
	 * I.e. if p is a pointer pointing at the beginning of the body, then p + dataChkVectByteOffset
	 * should point at the vector<DataChunk>. This value also represents the actual (i.e. system
	 * related and not cost-formulae related) size of the body section corresponding to the directory chunks.
	 *
	 * @see	BucketHeader::getRealDirSz()
	 */
	size_t dataChkVectByteOffset;
	
	/**
	 *  This the actual (i.e. sytem related and not cost formulae related) size of the record body
	 */
	size_t recordBdySz;

	/**
	 * default constructor
	 */
	BucketHeader() : id(), hdrSz(0), dirSz(0), dataSz(0), bodySz(0), dataChkVectByteOffset(0), recordBdySz(0){}
	/**
	 * constructor
	 */
	BucketHeader(const BucketID& bid,  size_t hd,  size_t dir,  size_t data, size_t bd, size_t off, size_t rb)
		: id(bid),hdrSz(hd),dirSz(dir),dataSz(data),bodySz(bd), dataChkVectByteOffset(off), recordBdySz(rb){}
	/**
	 * copy constructor
	 */
	BucketHeader(const BucketHeader & bh)
		: id(bh.id),hdrSz(bh.hdrSz), dirSz(bh.dirSz), dataSz(bh.dataSz), bodySz(bh.bodySz), dataChkVectByteOffset(bh.dataChkVectByteOffset), recordBdySz(bh.recordBdySz){}
		
	/**
	 * Returns the actual (i.e. system related and not cost-formulae based) size of the directory chunks
	 * stored in the record body.
	 */
	 size_t getRealDirSz() {
	 	return dataChkVectByteOffset;
	 }		

	/**
	 * Returns the actual (i.e. system related and not cost-formulae based) size of the data chunks
	 * stored in the record body.
	 */
	 size_t getRealDataSz() {
	 	return (recordBdySz-dataChkVectByteOffset);
	 }		
	
	/**
	 * Returns the actual (i.e. system related and not cost-formulae based) size of the record body
	 */
	 size_t getRealBdySz() {
	 	return recordBdySz;
	 }			 	
};

class DataChunk; // fwd declarations
class DirChunk;

 /**
  * This class represents a "bucket", i.e. a fixed size storage unit equal to a disk page.
  * @author: Nikos Karayannidis
  */
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
	Bucket(BucketHeader* const h, vector<DirChunk>* const dir, vector<DataChunk>* const data);
	~Bucket();
	/**
	 * overloaded assignement operator
	 */
	Bucket const& operator=(Bucket const& other);

	/**
	 * this method checks that the constraint: size_of(body) <= PAGE_SIZE holds
	 */
	void checkSize();		

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
	vector<DataChunk>* datap;
	vector<DirChunk>* dirp;
};

#endif // BUCKET_H