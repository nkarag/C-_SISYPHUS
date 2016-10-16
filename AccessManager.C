/***************************************************************************
                          AccessManager.C  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/

#include "AccessManager.h"
#include "AccessManagerImpl.h"
/*#include "Cube.h"
#include "SystemManager.h"
#include "FileManager.h"
#include "CatalogManager.h"
#include "Chunk.h"
#include "DiskStructures.h"
#include "bitmap.h"
#include "Exceptions.h"

#include <strstream>
#include <fstream>
#include <sm_vas.h>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdio.h>
#include <new>
#include <map>
#include <climits>*/


//--------------------------------- class AccessManager -------------------------------------//

AccessManager::AccessManager(ostream& out = cout, ofstream& error = StdinThread::errorStream)	{			
	accMgrImpl = new AccessManagerImpl(out, error);
}

AccessManager::~AccessManager() { delete accMgrImpl; }

bool AccessManager::isDataChunk(int depth, int local_depth, bool next_flag, int max_depth){
      	return AccessManagerImpl::isDataChunk(depth, local_depth, next_flag, max_depth);
}//IsDataChunk()
	
bool AccessManager::isDirChunk(int depth, int local_depth, bool next_flag, int max_depth){
       return AccessManagerImpl::isDirChunk(depth, local_depth, next_flag, max_depth);
}//IsDirChunk()
	
bool AccessManager::isRootChunk(int depth, int local_depth, bool next_flag, int max_depth){
       return AccessManagerImpl::isRootChunk(depth, local_depth, next_flag, max_depth);
}//IsRootChunk()
	
bool AccessManager::isLargeChunk(size_t size){
       return AccessManagerImpl::isLargeChunk(size);
}//isLargeChunk
	
bool AccessManager::isArtificialChunk(int local_depth){
       return AccessManagerImpl::isArtificialChunk(local_depth);
}

void AccessManager::parseCommand(char* line, bool& quit)const
{
	accMgrImpl->parseCommand(line, quit);
} // end AccessManager::commandParse

cmd_err_t AccessManager::create_cube(string& name) const
{
	return accMgrImpl->create_cube(name);
}

cmd_err_t AccessManager::drop_cube(string& name) const
{
	return accMgrImpl->drop_cube(name);
}

cmd_err_t AccessManager::load_cube (const string& name, const string& dimFile, const string& factFile, const string& configFile)const
{
	return accMgrImpl->load_cube (name, dimFile, factFile, configFile);
}//AccessManager::load_cube

cmd_err_t AccessManager::print_cube(string& name)const
{
	return accMgrImpl->print_cube(name);
}//AccessManager::print_cube


