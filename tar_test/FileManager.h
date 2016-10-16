/***************************************************************************
                          FileManager.h  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "Cube.h"

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
};

#endif // FILE_MANAGER_H
