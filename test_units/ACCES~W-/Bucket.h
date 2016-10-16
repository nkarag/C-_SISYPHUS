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




typedef int serial_t;

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
	BucketID() : rid(0) {}
	BucketID(const serial_t& r) : rid(r) {}
	/**
	 * copy constructor
	 */
	BucketID(const BucketID& bid) : rid(bid.rid) {}
	
	/**
	 * Check for a null bucket id
	 */
	bool isnull() {return (rid == 0);}
	
	friend bool operator==(const BucketID& b1, const BucketID& b2) {
	 	return (b1.rid == b2.rid);
	}

	friend bool operator!=(const BucketID& b1, const BucketID& b2) {
	 	return (b1.rid != b2.rid);
	}		
	
	friend bool operator<(const BucketID& b1, const BucketID& b2) {
	 	return (b1.rid < b2.rid);
	}			
};


#endif // BUCKET_H