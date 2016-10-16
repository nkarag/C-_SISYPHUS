/***************************************************************************
                          Bucket.C  -  description
                             -------------------
    begin                : Fri Oct 13 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/

#include "Bucket.h"
#include "SystemManager.h"
#include "Chunk.h"
#include "definitions.h"
#include "Exceptions.h"

BucketID BucketID::createNewID()
{
	// Create a BucketID for the root Bucket.
	// NOTE: no bucket allocation performed, just id generation!
	rc_t err;
	serial_t record_ID;
        err = ss_m::create_id(SystemManager::getDevVolInfo()->volumeID , 1, record_ID);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"BucketID::createNewID ==> Error in ss_m::create_id"<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}
	return BucketID(record_ID);	
}// end of BucketID::createNewID