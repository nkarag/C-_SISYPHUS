/***************************************************************************
                          FileManager.C  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


#include "FileManager.h"
#include "SystemManager.h"
#include <strstream>

FileManager::FileManager() {}

FileManager::~FileManager() {}

void FileManager::createCubeFile(FileID& fid) {

	// create shore file
	serial_t shore_fid;
	rc_t err = ss_m::create_file(SystemManager::getDevVolInfo()->volumeID, shore_fid, ss_m::t_regular);
	if(err) {
		ostrstream error;
		// Print Shore error message
		error <<"FileManager::createCubeFile ==> "<< err <<endl;
		// throw an exeption
		throw error.str(); 
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
		error << "FileManager::destroyCubeFile ==> Exception while calling ss_m::destroy_rec "<< err <<endl;
		// throw an exeption
		throw error.str(); 
	}

} // end destroyCubeFile

