/***************************************************************************
                          FileManager.C  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/

#include <strstream>

#include "FileManager.h"
#include "SystemManager.h"
#include "DiskStructures.h"
#include "definitions.h"
#include "Exceptions.h"
#include "DataVector.h"
#include "Cube.h"
#include "Bucket.h"

FileManager::FileManager() {}

FileManager::~FileManager() {}

void FileManager::createCubeFile(FileID& fid) {

	// create shore file
	serial_t shore_fid;
	rc_t err = ss_m::create_file(SystemManager::getDevVolInfo()->volumeID, shore_fid, ss_m::t_regular);
	if(err) {
		ostrstream error;
		// Print Shore error message
		error <<"FileManager::createCubeFile ==> "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	// update FileID
	fid.set_shoreID(shore_fid);
} // end createCubeFile

void FileManager::destroyCubeFile(const FileID& fid) 
{
	rc_t err = ss_m::destroy_file(SystemManager::getDevVolInfo()->volumeID, fid.get_shoreID());
	if(err) {
		ostrstream error;
		// Print Shore error message
		error << "FileManager::destroyCubeFile ==> Exception while calling ss_m::destroy_rec "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

} // end destroyCubeFile

void FileManager::storeDiskBucketInCUBE_File(const DiskBucket* const dbuckp, const FileID& fid)
//precondition:
//	dbuckp points at a filled diskBucket structure in heap that is ready to be
//	stored in a SSM record. cinfo holds info about the cube, such as the SSM file where
//	this record must be created.
//postcondition:
//	the DiskBucket has been stored in a SSM record with id == dbuckp->hdr->id.rid and in
//	the SSM file assigned to the cube in question. In the SSM record header we do not store anything,
//	the whole DiskBucket is placed in the SSM record body.
{
	//ASSERTION1: dbuckp does not point to NULL
	if(!dbuckp)
		throw GeneralError(__FILE__, __LINE__, "FileManager::storeDiskBucketInCUBE_File ==> ASSERTION1: null pointer\n");
		
	//W_COERCE(ss_m::begin_xct());
	         	         		
	// create SSM record corresponding to a Bucket. Use the id created earlier.
        rc_t err = ss_m::create_rec_id(SystemManager::getDevVolInfo()->volumeID , fid.get_shoreID(),
                         vec_t(0, 0),       /* header  */
                         PAGESIZE,  /* length hint          */
                         vec_t(dbuckp, sizeof(DiskBucket)), /* body    */
                         dbuckp->hdr.id.rid);      /* rec id           */
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"FileManager::storeDiskBucketInCUBE_File ==>Error in ss_m::create_rec_id "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}
 	//W_COERCE(ss_m::commit_xct());         	         	
}//end of FileManager::storeDiskBucketInCUBE_File

void FileManager::storeDataVectorsInCUBE_FileBucket(const DataVector& hdr,
					const DataVector& body,
					const FileID& fid,
					const BucketID& bcktID,
					ssphSize_t szHint = 0)
//precondition:
//	A file with file id "fid" exists. "bcktID" is a valid bucket id corresponding
//	to a new bucket in "fid" that has NOT yet been created.
//processing:
//	invoke appropriate call to underlying SSM method for creating a record
//postcondition:					
//	A new bucket has been created in the file and "body" has been places in the bucket body and
//	"hdr" in the bucket header.
{
	// create SSM record corresponding to a Bucket. Use the id created earlier.
        rc_t err = ss_m::create_rec_id(SystemManager::getDevVolInfo()->volumeID , fid.get_shoreID(),
                         hdr.dataVector2vec_t(),       /* header  */
                         szHint,  /* length hint          */
                         body.dataVector2vec_t(), /* body    */
                         bcktID.rid);      /* rec id           */
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"FileManager::storeDataVectorsInCUBE_FileBucket ==>Error in ss_m::create_rec_id "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}
}//FileManager::storeDataVectorsInCUBE_FileBucket
										

