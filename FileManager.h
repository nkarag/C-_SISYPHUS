/***************************************************************************
                          FileManager.h  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

//#include "Cube.h"
#include "definitions.h"

struct DiskBucket; //forward declarations
class DataVector;
class FileID;
class BucketID;

/**
 * The FileManager class provides operations
 * related to the file system, e.g. open/close file,
 * read/write file etc. 
 *
 * @see 
 * @author: Nikos Karayannidis
 */
class FileManager {

public:
	/**
	 * Constructor
	 */
	FileManager();
	
	/**
	 * Destructor
	 */
	~FileManager();

	/**
	 * Creates a new Cube file and return its file id
	 * In case of a failure it throws a (char*) exception.
	 */
	static void createCubeFile(FileID& fid);

	/**
	 * Destroys a specified cube file.
	 */
	static void destroyCubeFile(const FileID& fid);
	

	/**
	 * This function receives a pointer to a DiskBucket structure and calls the appropriate
	 * SSM call in order to create a SSM record and store the DiskBucket into a fixed size
	 * bucket of a cube, in the bucket-oriented file system (i.e., into a CUBE_File). No order is
	 * defined on the buckets in a file: when a new bucket is created, the I/O subsystem may
	 * place the bucket anywhere in the CUBE File.
	 *
	 * @param dbuckp	the pointer to the DiskBucket structure
	 * @param fid		the file id of the CUBE_File in question
	 */		 		
	static void storeDiskBucketInCUBE_File(const DiskBucket* const dbuckp, const FileID& fid);	

	/**
	 * This routine receives to data vectors, in order to store them in a new CUBE File
	 * bucket with id "bcktID". The 1st one will be stored in the header of the bucket, while
	 * the 2nd one will be stored in the body of the bucket. In the case that the bucket size
	 * will grow in the future, the final bucket size is passed as a  hint in "szHint". This
	 * should ideally reflect the final length (in bytes) of the bucket and it will help the
	 * FileManager to place the bucket to alocation with enough contiguous space for this bucket.
	 * A value of 0 should be used if the final length is unknown (default value). No order is
	 * defined on the buckets in a file: when a new bucket is created, the I/O subsystem may
	 * place the bucket anywhere in the CUBE File. The length of the bucket is limited only by the
	 * size limts of the underlying SSM record  (apprx. 2GB)
	 *
	 * @param hdr	the header of the bucket
	 * @param body	the body of the bucket
	 * @param fid	id of the file with which a CUBE File is implemented
	 * @param bcktID the id of the bucket where the hdr and body will be stored.
	 *		***NOTE***: it is assumed that no bucket has been created yet
	 *			   only id generation!
	 * @param szHint byte length hint
	 */	
	static void storeDataVectorsInCUBE_FileBucket(const DataVector& hdr,
						const DataVector& body,
						const FileID& fid,
						const BucketID& bcktID,
						ssphSize_t szHint = 0);
};

#endif // FILE_MANAGER_H
