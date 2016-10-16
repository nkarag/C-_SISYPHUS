/***************************************************************************
                          AccessManagerImpl.C  -  description
                             -------------------
    begin                : Tue Feb 26 2002
    copyright            : (C) 2002 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/
#include "definitions.h"
#include "AccessManagerImpl.h"
#include "Cube.h"
#include "SystemManager.h"
#include "FileManager.h"
#include "CatalogManager.h"
#include "Chunk.h"
#include "DiskStructures.h"
#include "bitmap.h"
#include "Exceptions.h"
#include "DataVector.h"
#include "Misc.h"

#include <strstream>
#include <fstream>
#include <sm_vas.h>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdio.h>
#include <new>
#include <map>
#include <climits>

enum command_token_t {
    create_cmd,
    drop_cmd,
    load_cmd,
    print_cmd,
    quit_cmd,
    help_cmd
};

struct command_description_t {
    command_token_t     token;
    int                 param_cnt;      // number of parameters
    char*               name;           // string name of command
    char*               parameters;     // parameter list
    char*               description;    // command description
};

static command_description_t descriptions[] = {
    {create_cmd, 1, "create_cube", "name",       "create a cube with name <name>"},
    {drop_cmd, 1, "drop_cube", "name",       "delete cube with name <name>"},
    {load_cmd,  4, "load_cube",  "name dim_file data_file config_file", "load cube <name> with the data in <data_file> according to the construction parameters in <config_file>"},
    {print_cmd,  1, "print_cube",  "name",       "print the data of cube <name>"},
    {quit_cmd,   0, "quit",   "",       "quit and exit program"},
    {help_cmd,   0, "help",   "",       "prints this message"}
};

// number of commands
static unsigned int command_cnt = sizeof(descriptions)/sizeof(command_description_t);

static void
print_commands()
{
    cerr << "Valid commands are: \n"<< endl;
    const command_description_t* cmd;
    for (cmd = descriptions; cmd != descriptions+command_cnt; cmd++) {
        cerr << "    " << cmd->name << " " << cmd->parameters << endl;
        cerr << "        " << cmd->description << endl;
    }
    cerr << "\n    Comments begin with a '#' and continue until the end of the line." << endl;
}

static void
print_usage(const command_description_t* cmd)
{
    cerr << "Usage: "<< cmd->name << " " << cmd->parameters << endl;
}

//--------------------------------- class AccessManager -------------------------------------//

void AccessManagerImpl::parseCommand(char* line, bool& quit)
{
    istrstream  s(line);

    const int   max_params = 6;
    char*       params[max_params];
    int         param_cnt = 0;
    int         i;

    // find all parameters in the line (parameters begin
    // with non-white space) and end each parameter with \0
    bool in_param = false;      // not current in a parameter
    for (i = 0; line[i] != '\0'; i++) {
        if (in_param) {
            if (isspace(line[i])) {
                // end of parameter
                line[i] = '\0';
                in_param = false;
            }
        } else {
            if (line[i] == '#') {
                // rest of line is comment
                break;
            }

            if (!isspace(line[i])) {
                // beginning of parameter
                if (param_cnt == max_params) {
                    cerr << "Error: too many parameters." << endl;
                    return;
                }
                params[param_cnt] = line+i;
                param_cnt++;
                in_param = true;
            }
        }
    }

    if (param_cnt == 0) {
        // blank line
        return;
    }

    // Search for command in command list
    command_description_t* cmd;
    for (cmd = descriptions; cmd != descriptions+command_cnt; cmd++) {
        // command is recognized with just first 2 characters
        if (strncmp(params[0], cmd->name, 2) == 0) {
            break;
        }
    }
    if (cmd == descriptions+command_cnt) {
        // command not found
        cerr << "Error: unkown command " << params[0] << endl;
        print_commands();
    } else if (cmd->param_cnt != param_cnt-1) {
        // wrong number of parameters
        cerr << "Error: wrong number of parameters for " << cmd->name << endl;
        print_usage(cmd);
    } else {

        // call proper method for the command

        string   name;
	string	dimFile;
	string 	factFile;
	string configFile;
        quit = false;

        cmd_err_t err = 0;
        switch(cmd->token) {
        case create_cmd:
	    name = params[1];
            err = create_cube(name);
            if (!err) {
                cout << "Cube "<<name<<" succesfully created!" << endl;
            }
            break;
        case drop_cmd:
	    name = params[1];
            err = drop_cube(name);
            if (!err) {
                cout << "Cube "<<name<<" succesfully deleted!" << endl;
            }
            break;
        case load_cmd:
	    name = params[1];
	    dimFile = params[2];
	    factFile = params[3];
	    configFile = params[4];
	
	    try {
            	err = load_cube(name, dimFile, factFile, configFile);
            }
	    catch(GeneralError& error) {
       		GeneralError e("AccessManagerImpl::parseCommand() ==> ");
       		error += e;
            	errorLogStream<<error<<endl;
            	err =  (char*)error.getErrorMessage().c_str();       		
            }
            catch(std::bad_alloc&){
            	GeneralError error("AccessManagerImpl::parseCommand() ==> No more memory available! Some new operation failed (see error log)!");	
            	errorLogStream<<error<<endl;
            	err =  (char*)error.getErrorMessage().c_str();
            }//end catch
            catch(exception& e){
                ostrstream msg_stream;
                msg_stream<<"AccessManagerImpl::parseCommand() ==> Exception " << e.what() << " (derived from \"exception\") was thrown"<<endl;
                GeneralError error(msg_stream.str());            	
            	errorLogStream<<error<<endl;
            	err =  (char*)error.getErrorMessage().c_str();
            }
            catch(...){
            	GeneralError error("AccessManagerImpl::parseCommand() ==> Exception caught other than GeneralError, bad_alloc, or other derived from exception (in <exception>)! ");	
            	errorLogStream<<error<<endl;
            	err =  (char*)error.getErrorMessage().c_str();
            }//end catch

            if (!err) {
                cout << "Cube "<<name<<" succesfully loaded!" << endl;
            }
            break;
        case print_cmd:
	    name = params[1];
            err = print_cube(name);
            break;
        case quit_cmd:
            quit = true;
            break;
        case help_cmd:
            print_commands();
            break;
        default:
            cerr << "Internal Error at: " << __FILE__ << ":" << __LINE__ << endl;
            exit(1);
        } // end switch

        if (err) {
            //cerr << "Error: " << err << endl;
            cerr << err << endl;
            cerr << "Error: " << cmd->name << " command failed. See \"error.log\"." << endl;
        }
    } // end else

} // end AccessManagerImpl::commandParse

cmd_err_t AccessManagerImpl::create_cube(string& name)
{

	W_COERCE(ss_m::begin_xct());

	FileID file_id;
        // create the cube file
	try {
		FileManager::createCubeFile(file_id);
	}
	catch(GeneralError& error){
		GeneralError e("Exception while creating Cube file: ");
		error += e;
		errorLogStream<<error<<endl;
		cmd_err_t err =  (char*)error.getErrorMessage().c_str();
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());
		return err;
	}

	// update the catalog for the new cube

	// First, create a CubeInfo.
	CubeInfo cbinfo(name);
	cbinfo.set_fid(file_id);

	try {
		CatalogManager::registerNewCube(cbinfo);
	}
	catch(GeneralError& error){
		GeneralError e("Exception while registering new Cube in the catalog: ");
		error += e;
		errorLogStream<<error<<endl;
		cmd_err_t err =  (char*)error.getErrorMessage().c_str();
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());
		return err;
	}

 	W_COERCE(ss_m::commit_xct());  // commit the cube creation
	return 0;
}

cmd_err_t AccessManagerImpl::drop_cube(string& name)
{
	W_COERCE(ss_m::begin_xct());

	// first get information about the cube
	CubeInfo info;
	try{
		CatalogManager::getCubeInfo(name, info);

	}
	catch(GeneralError& error) {
		GeneralError e("Ex.from CatalogManager::getCubeInfo in AccessManagerImpl::drop_cube(): ");
		error += e;
		errorLogStream<<error<<endl;
		cmd_err_t err =  (char*)error.getErrorMessage().c_str();
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());
		return err;
	}

	// delete cube file
	try {
		FileManager::destroyCubeFile(info.get_fid());

	}
	catch(GeneralError& error) {
		GeneralError e("Ex. from FileManager::destroyFile in  AccessManagerImpl::drop_cube(): ");
		error += e;
		errorLogStream<<error<<endl;
		cmd_err_t err =  (char*)error.getErrorMessage().c_str();
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());
		return err;
	}

	// update catalog
	try{
		CatalogManager::unregisterCube(info);

	}
	catch(GeneralError& error) {
		GeneralError e("AccessManagerImpl::drop_cube() ==> ");
		error += e;
		errorLogStream<<error<<endl;
		cmd_err_t err =  (char*)error.getErrorMessage().c_str();
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());
		return err;
	}

 	W_COERCE(ss_m::commit_xct());  // commit the cube destroying

	return 0;
}

cmd_err_t AccessManagerImpl::load_cube (const string& name, const string& dimFile, const string& factFile, const string& configFile)
{
	// Execute the whole loading (i.e. CUBE File creation) process
	// as one big transaction (i.e. all or nothing).
	W_COERCE(ss_m::begin_xct());

	// first get information about the cube from the catalog
	CubeInfo info;
	try{
		CatalogManager::getCubeInfo(name, info);
	}
	catch(GeneralError& error) {
		GeneralError e("AccessManagerImpl::load_cube ==> ");
		error += e;
		errorLogStream<<error<<endl;
		cmd_err_t err =  (char*)error.getErrorMessage().c_str();
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());
		return err;
	}

	// get information about the dimensions from the dimFile
	try {
		info.Get_dimension_information(dimFile);
	}
	catch(GeneralError& error) {
		GeneralError e("AccessManagerImpl::load_cube ==> ");
		error += e;
		errorLogStream<<error<<endl;
		cmd_err_t err =  (char*)error.getErrorMessage().c_str();
		
		//***TODO*** clear dimension vector from cube info object
		
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());
		return err;
	}
     // The following comment section is from lsinos
    /*for (vector<Dimension>::iterator iter_dim = info.get_vectDim().begin();
    								 iter_dim != info.get_vectDim().end();
    								 ++iter_dim)
    {
		cout << "DIMENSION:" << (*iter_dim).get_name() << " - LEVELS: " << (*iter_dim).get_num_of_levels() << endl;
		for (vector<Dimension_Level>::iterator iter_lev = (*iter_dim).get_vectLevel().begin();
			 iter_lev != (*iter_dim).get_vectLevel().end();
			 ++iter_lev)
		{
			cout << "LEVEL:" << (*iter_lev).get_name() << " - LEVEL NUMBER: " << (*iter_lev).get_level_number();
			cout << " - MEMBERS: " << (*iter_lev).get_num_of_members() << endl;
			for (vector<LevelMember>::iterator iter_mem = (*iter_lev).get_vectMember().begin();
				 iter_mem != (*iter_lev).get_vectMember().end();
				 ++iter_mem)
			{
				cout << "MEMBER:" << (*iter_mem).get_name() << " - ORDER CODE: " << (*iter_mem).get_order_code();
				cout << " - MCODE:" << (*iter_mem).get_member_code() << " - PMCODE: " << (*iter_mem).get_parent_member_code();
				cout << " - FCOCODE:" << (*iter_mem).get_first_child_order_code();
				cout << " - LCOCODE: " << (*iter_mem).get_last_child_order_code() << endl;
			}
		}
	}*/
	// show dimension information on the screen
	info.Show_dimensions();
        #ifdef DEBUGGING
              cerr << "Reading the fact schema info..." <<endl;
        #endif
	// get fact information  from file
	try {
		info.getFactInfo(factFile);
	}
	catch(GeneralError& error) {
		GeneralError e("AccessManagerImpl::load_cube ==> ");
		error += e;
		errorLogStream<<error<<endl;
		cmd_err_t err =  (char*)error.getErrorMessage().c_str();
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());		
		return err;
	}
#ifdef DEBUGGING
      cerr << "Starting the Cube File construction algorithm ..." <<endl;
#endif	
	// Construct CUBE File
	try{
		constructCUBE_File(info, factFile, configFile);
	}
	catch(GeneralError& error) {
		GeneralError e("AccessManagerImpl::load_cube ==> ");//"Ex. from AccessManagerImpl::constructCubeFile in AccessManagerImpl::load_cube : ");
		error += e;
		errorLogStream<<error<<endl;
		cmd_err_t err =  (char*)error.getErrorMessage().c_str();
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());		
		return err;
	}
	catch(...){
		throw;
	}

	// store updated CubeInfo obj back on disk
	try{
		//CatalogManager::updateCubeInfo(name, info);
	}
	catch(GeneralError& error) {
		GeneralError e("AccessManagerImpl::load_cube ==> ");
		error += e;
		errorLogStream<<error<<endl;
		cmd_err_t err =  (char*)error.getErrorMessage().c_str();
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());
		return err;
	}

 	W_COERCE(ss_m::commit_xct()); // commit Cube loading

	return 0;
}//AccessManagerImpl::load_cube

cmd_err_t AccessManagerImpl::print_cube (string& name)
{
	return 0;
}

/*
Chunk_cell_data* AccessManagerImpl::Create_root_chunk(CubeInfo& info)
{
	//Calculate the size of the array that represents the root chunk
	int number_of_cells = 1; // counter of cells of the root chunk

	vector<Dimension_Level>::iterator iter_lev;
    for (vector<Dimension>::iterator iter_dim = info.get_vectDim().begin();
    								 iter_dim != info.get_vectDim().end();
    								 ++iter_dim)
    {
		iter_lev = (*iter_dim).get_vectLevel().begin(); //pointer to upper level
		number_of_cells *= (*iter_lev).get_num_of_members();
	}

	//Create the root chunk with empty cells
	Chunk_cell_data* root_chunk = new Chunk_cell_data[number_of_cells];
	for (int i=0; i<number_of_cells; i++)
	{
		root_chunk[i].bucketID = serial_t::null;
		root_chunk[i].chunk_offset = -1; // -1 means empty
	}

	//Allocate a bucket for the root chunk in the file with the name that is stored in the CubeInfo object
	serial_t record_ID;
    rc_t err = ss_m::create_rec(SystemManager::getDevVolInfo()->volumeID , info.get_fid().get_shoreID(),
                         vec_t(),      // empty header
                         PAGESIZE,    // size of record = 8KBytes
                         vec_t(),      // empty body
                         record_ID);   // bucket ID
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"Error in ss_m::create_rec in AccessManagerImpl::Create_root_chunk "<< err <<endl;
		// throw an exeption
		throw error;
	}

	//Store the ID of the root bucket in the CubeInfo object
	info.set_rootBucket(record_ID); //***I may store again the CubeInfo object***

	return root_chunk;
}
*/

void AccessManagerImpl::constructCUBE_File(CubeInfo& cinfo, const string& factFile, const string& configFile) const
//precondition:
//	cinfo contains the following valid information that will be used in this procedure:
//		- SSM file id, maxDepth, numFacts, num_of_dimensions, vectDim.
//	Also valid are the cube name and cube id.
//	factFile contains grain level fact table data,
// 	in the form of: chunkid	value1	value2...valueM
//	in each line, which corresponds to a single cell at the most detailed level.
//	Further, we assume that these lines are sorted in ascending order by their chunkid.
//postcondition:
//	A CUBE File organization has been created inside a single SSM file, loaded with the data
//	in factFile.
{
	// set CUBE FILE construction parameters
	AccessManager::CBFileConstructionParams constructionParams;  //default values initially
	// open input configuration file for reading
	ifstream config_input(configFile.c_str());
	if(!config_input){
		outputLogStream << "CUBE File construction config file could not be opened for reading, using default values...\n";
		errorLogStream << "CUBE File construction config file could not be opened for reading, using default values...\n";
	}//end if
	else {
		constructionParams.initParamsFromFile(config_input); //or init from config file
	}//end else
	
	//Update cinfo object with new AccessManager::CBFileConstructionParams
	cinfo.setconstructParams(constructionParams);

	// 1. Estimate the storage cost for the components of the chunk hierarchy tree

	// In this phase we will use only chunk headers.
	// 1.1 construct root chunk header.
	ChunkHeader* rootHdrp = new ChunkHeader;
	Chunk::createRootChunkHeader(rootHdrp, cinfo);

	// 1.2 create CostNode Tree
	CostNode* costRoot = 0;
	try {
		costRoot = Chunk::createCostTree(rootHdrp, cinfo, factFile);
	}
	catch(GeneralError& error) {
		GeneralError e("AccessManagerImpl::constructCubeFile ==>  ");
		error += e;
		delete rootHdrp;
		//if(costRoot) delete costRoot; normally on an exception thrown, the costRoot will still be 0!
		throw error; //error.getErrorMessage().c_str();
	}
	catch(...){
		delete rootHdrp;
		//if(costRoot) delete costRoot; normally on an exception thrown, the costRoot will still be 0!
		throw;
	}
	delete rootHdrp;// free up memory
	rootHdrp = 0;
	
        #ifdef DEBUGGING
       	try{
       	      test_construction_phaseI(costRoot, cinfo);
       	}
       	catch(GeneralError& error) {
       		GeneralError e("AccessManagerImpl::constructCubeFile ==>  ");//"Ex. from AccessManagerImpl::test_construction_phaseI in AccessManagerImpl::constructCubeFile : ");
       		error += e;
       		delete costRoot; // free up the whole tree space!
       		throw error;
       	}
       	catch(...){
       		delete costRoot; // free up the whole tree space!       	
		throw;       	
       	}
        #endif	
	
	// 2. General target: Use CostNode tree to allocate in-memory chunks, load them from Factfile,
	//    attach them to in-memory buckets, store buckets on disk.
	
	// 2.1. Create a BucketID for the root Bucket.
	//      NOTE: no bucket allocation performed yet, just id generation!
	BucketID rootBcktID;
	try{
		rootBcktID = BucketID::createNewID();				
	}
       	catch(GeneralError& error) {
       		GeneralError e("AccessManagerImpl::constructCUBE_File ==> ");
       		error += e;
       		delete costRoot; // free up the whole tree space!
       		throw error;
       	}	
       	catch(...){
       		delete costRoot; // free up the whole tree space!       	
		throw;       	
       	}
		
	// update CubeInfo root Bucket ID member.
	// NOTE: the root chunk offset (i.e.index) within this bucket is a constant (see class CubeInfo)
	cinfo.set_rootBucketID(rootBcktID);	
	
	// 2.2. Start the basic chunk-packing into buckets algorithm.
	
	// Creating the vector holder for the entries of the root Bucket.		
	// This will be filled by putChunksIntoBuckets
	vector<DirChunk>* rtBcktEntriesVectp = new vector<DirChunk>;
	rtBcktEntriesVectp->reserve(1); // reserve one position for storing the root chunk	
	DirEntry rootEntry; //will be updated by putChunksIntoBuckets
	//unsigned int lastIndxInRootBck = 0; // will be updated by putChunksIntoBuckets
	try {
		putChunksIntoBuckets(cinfo,
			   costRoot,
			   cinfo.get_rootChnkIndex(),
			   factFile,
			   rtBcktEntriesVectp,
			   rootEntry,
			   constructionParams);
			   //currIndxInRootBck);
	}
	catch(GeneralError& error) {
		GeneralError e("AccessManagerImpl::ConstructCubeFile  ==> ");
		error += e;
		delete rtBcktEntriesVectp;
		delete costRoot; // free up the whole tree space!
		throw error;
	}
	catch(...) {
		delete rtBcktEntriesVectp;
		delete costRoot; // free up the whole tree space!
		throw;
	}
	
	// assertion following:
	if((rootEntry.bcktId != cinfo.get_rootBucketID()) || (rootEntry.chnkIndex != cinfo.get_rootChnkIndex())) {
		delete rtBcktEntriesVectp;
		delete costRoot; // free up the whole tree space!
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::ConstructCubeFile  ==> Error in returned DirEntry value from AccessManagerImpl::putChunksIntoBuckets");
	}
	
      	// ***IMPORTANT***: Free up memory space by deleting this node.
      	// NOTE: this deletion might propagate down further to children nodes at arbitrary depth
      	// but these (the children) must already have been deleted by now (in putChunksIntoBuckets)and the corresponding
      	// entries removed from the vector of children pointers!
      	#ifdef DEBUGGING
      	// ASSERTION: indeed no children entries left
      	if(!costRoot->getchild().empty()){
		delete rtBcktEntriesVectp;
		delete costRoot; // free up the whole tree space!      	
      		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::constructCUBE_File ==> Before freeing cost node, not empty vector of children pointers found!");
      	}//end if
      	#endif
      	
      	//free up memory                		
	delete costRoot;
	costRoot = 0;
			
	// 2.3. Now, we are ready to create and store the root bucket.		
        try{
        	
		storeRootDirectoryInCUBE_File(	cinfo,
						*rtBcktEntriesVectp,
						cinfo.get_rootChnkIndex(),
						rootBcktID,
						constructionParams.rootDirectoryStorage,
						constructionParams.rootDirMemConstraint);
        }
       	catch(GeneralError& error) {
       		GeneralError e("AccessManagerImpl::constructCUBE_File==>");
       		error += e;
       		delete rtBcktEntriesVectp;
       		throw error;
       	}
       	catch(...){
       		delete rtBcktEntriesVectp;
		throw;       	
       	}
       	
       	delete rtBcktEntriesVectp;
       	rtBcktEntriesVectp = 0;       	      	      	      						
}//AccessManagerImpl::constructCUBE_File

void AccessManagerImpl::storeRootDirectoryInCUBE_File(
					const CubeInfo& cinfo,
					vector<DirChunk>& dirChunksRootDirVect,
					unsigned int rootIndex,
					const BucketID& rootBcktID,
					AccessManager::rootDirectoryStorage_t rootDirStg,
					memSize_t memory_constraint)const
//precondition:
//	The input vector dirChunksRootDirVect contains instances of all the dir chunks of the root directory,
//	stored in a depth first manner. The root chunk is stored in position rootIndex. The DirEntries in these
//	chunks should all (except the ones that point
//	to chunks that dont belong to the root directory) have a bucket id equal with rootBcktID and an index to
//	a child node that corresponds to their storage in the dirChunksRootDirVect vector.
//	rootBcktID is an id for the root bucket where the whole root directory will be stored, or only a part of
//	it that contains the root chunk (depends on the storage method in rootDirStg). No allocation for a bucket has taken place
//	yet; ONLY id generation. memory_constraint is an input parameter showing the available memory for holding the root directory
//	when accessing the CUBE File, since  some storage methods keep the whole root directory in memory during quering.
//processing:
//	switch among a predefined set of root directory storage methods, and for the chosen one, invoke the appropriate routines.		
//postcondition:
//	The dir chunks of the input vector have all been stored in the CUBE File. The input chunk is now empty
{
	//ASSERTIONS ...
	if(dirChunksRootDirVect.empty())
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeRootDirectoryInCUBE_File ==> empty input vector with dir chunks!\n");
		
	if(rootBcktID.isnull())
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeRootDirectoryInCUBE_File ==> Null input id for the root bucket!\n");	
		
	//Calculate the size of the whole root directory. Actually this will be a lower bound for this size since the
	//size consumed by the potential order-code mappings (for artificial chunks) are not counted.
	memSize_t dirRootSizeLowerBound = 0;
	for(vector<DirChunk>::const_iterator chunkIter = dirChunksRootDirVect.begin(); chunkIter != dirChunksRootDirVect.end(); chunkIter++){
		dirRootSizeLowerBound += chunkIter->gethdr().size;			
	}// end for
	
	cout<<"\n*** The ROOT DIRECTORY's size is >= "<<dirRootSizeLowerBound<<"(bytes) ***"<<endl;
	cout<<"*** The MEMORY CONSTRAINT (for the root dir) size is = "<<memory_constraint<<"(bytes) ***"<<endl;	
				
	//choose the appropriate method for storing the root directory.	
	switch(rootDirStg){
        	//CASE1: Use a single (large) bucket and store in a depth 1st manner
        	case AccessManager::singleBucketDepthFirst:
        		{
        			if(rootIndex != 0)
					throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeRootDirectoryInCUBE_File ==> rootIndex != 0 for singleBucketDepthFirst method!\n");
				// check directory size w.r.t. the memory constraint
				if(dirRootSizeLowerBound > memory_constraint)
					// Oops! That's a bit large! :)
					throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeRootDirectoryInCUBE_File ==> The memory constraint for storing the root directory is smaller than the lower bound of the root directory. Sorry we cannot use SingleBucketDepthFirst for storing the root directory. Either choose another method or increase the memory constraint in the construction configuration file\n");
        			//instantiate method
        			SingleBucketDepthFirst method(dirRootSizeLowerBound, memory_constraint, this);
        			try{
                			method(cinfo, dirChunksRootDirVect, rootBcktID);	
        			}
                               	catch(GeneralError& error) {
                               		GeneralError e("AccessManagerImpl::storeRootDirectoryInCUBE_File ==> ");
                               		error += e;
                               		throw error;
                               	}        			        			
                		break;
        		}
        	case AccessManager::singleBucketBreadthFirst:
        		{
                		break;
        		}        		
               	default:
               		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeRootDirectoryInCUBE_File ==> Unknown method for storing the rrot directory\n");
               		break;		
   	}//end switch					
}//AccessManagerImpl::storeRootDirectoryInCUBE_File()

void AccessManagerImpl::SingleBucketDepthFirst::operator()(
				const CubeInfo& cinfo,
				vector<DirChunk>& dirChunksRootDirVect,
				const BucketID& rootBcktID)
//precondition:
//	The input vector dirChunksRootDirVect contains instances of all the dir chunks of the root directory,
//	stored in a depth first manner. The root chunk is stored in position 0. The DirEntries in these
//	chunks should all (except the ones that point
//	to chunks that dont belong to the root directory) have a bucket id equal with rootBcktID and an index to
//	a child node that corresponds to their storage in the dirChunksRootDirVect vector.
//	rootBcktID is an id for the root bucket where the whole root directory will be stored. No allocation for a bucket has taken place
//	yet; ONLY id generation.
//processing:
//	From the input vector of DirChunks create the "root bucket". This consists of a byte record containing the diskDatachunks of the
//	root directory, a bucket directory vector containing a mapping of chunk slots in the root bucket to byte offsets in the byte vector.
//	and finally a root bucket header containing administrative info for the root bucket.
//	Build the byte vector progressively and gradually empty the input vector in order to avoid the consumption of large memory space.
//postcondition:
//	All the DirChunk instances of the input vector have been removed (therefore dirChunksRootDirVect is empty) and stored
//	in a CUBE File bucket as described above.
{
	//ASSERTIONS ...
	if(dirChunksRootDirVect.empty())
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::operator() ==> empty input vector with dir chunks!\n");
		
	if(rootBcktID.isnull())
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::operator() ==> Null input id for the root bucket!\n");	
	
	//create the root bucket body: byte vector and the root bucket directory
	vector<char>* rtBuckByteVectp = 0;
	vector<DiskRootBucketHeader::dirent_t>* rtBuckDirp = 0;
	try{
		createRootBucketVectorsInHeap(cinfo.getmaxDepth(), dirChunksRootDirVect, rtBuckByteVectp, rtBuckDirp);
	}
       	catch(GeneralError& error) {
       		//1st free up memory
		if (rtBuckByteVectp)       	
			delete rtBuckByteVectp;       		
		if (rtBuckDirp)       	
			delete rtBuckDirp;       		
       		GeneralError e("AccessManagerImpl::SingleBucketDepthFirst::operator() ==> ");
       		error += e;
       		throw error;
       	}
       	catch(...){
       		//1st free up memory
		if (rtBuckByteVectp)       	
			delete rtBuckByteVectp;       		
		if (rtBuckDirp)       	
			delete rtBuckDirp;       		
		throw;       	
       	}//catch
       	
       	//ASSERTIONS...
       	#ifdef DEBUGGING
       	if(!rtBuckDirp){
		if (rtBuckByteVectp)       	
			delete rtBuckByteVectp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::operator() ==> NULL pointer returned, root bucket directory not initialized\n");			
       	}
       	else if (!rtBuckByteVectp){
		if (rtBuckDirp)       	
			delete rtBuckDirp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::operator() ==> NULL pointer returned, root bucket byte vector not initialized\n");			       	
       	}
       	#endif       	
       	if (rtBuckDirp->empty() || rtBuckByteVectp->empty()){
		// first free memory
		delete rtBuckByteVectp;
		delete rtBuckDirp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::operator() ==> empty root bucket vector(s) encountered!\n");	
	}// end if
	
	if(!dirChunksRootDirVect.empty()){
		// first free memory
		delete rtBuckByteVectp;
		delete rtBuckDirp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::operator() ==> Non-empty vector: some root dir DirChunks hav not been processed!\n");	
	}// end if
	
	//create the root bucket header
	
	//1. Number of chunks (i.e., real directory entries)
	DiskRootBucketHeader::entriesnum_t numChunks = dirChunksRootDirVect.size();
	
	//2. total number of directory entries
	DiskRootBucketHeader::entriesnum_t totdirEnt = rtBuckDirp->size();
	
	//ASSERTION:
	if(totdirEnt < numChunks) {
		// first free memory
		delete rtBuckByteVectp;
		delete rtBuckDirp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::operator() ==> root bucket directory entries are less than number of chunks in the root directory!\n");	
	}// end if
	
	//3. total root bucket body size = byte vect + directory
	DiskRootBucketHeader::bytesize_t rootBuckBodySz = rtBuckByteVectp->size() + rtBuckDirp->size() * sizeof(DiskRootBucketHeader::dirent_t);
	
	//4. byte offset in the body where the byte vector begins
	DiskRootBucketHeader::dirent_t byteVectOffs = rtBuckDirp->size() * sizeof(DiskRootBucketHeader::dirent_t);

	// Instanciate header	
	DiskRootBucketHeader rootBhdr(numChunks, totdirEnt, rootBuckBodySz, byteVectOffs);	
	
	//create a data vector for the root bucket body: 1st goes the directory
	// Note: for passing STL vectors as C arrays, see Effective STL Item 16
	DataVector bodyVector( &(*rtBuckDirp)[0], rtBuckDirp->size() * sizeof(DiskRootBucketHeader::dirent_t) );
	
	//then goes the byte vector
	// Note: for passing STL vectors as C arrays, see Effective STL Item 16	
	bodyVector.put(&(*rtBuckByteVectp)[0], rtBuckByteVectp->size()*sizeof(char));
	
	//Finally create a data vector for the root bucket header
	DataVector hdrVector(&rootBhdr, sizeof(rootBhdr));
	
	//store the root bucket in the CUBE File in a single bucket (i.e., SSM record)
	// of length as much as it is needed.	
	try {
		//hint for the final length (in bytes) of the bucket
		ssphSize_t finalLengthHint = int((hdrVector.size() + rootBuckBodySz) * (1 + cinfo.getconstructParams().prcntExtraSpace));
		FileManager::storeDataVectorsInCUBE_FileBucket(hdrVector, bodyVector, cinfo.get_fid(), rootBcktID, finalLengthHint);
	}
      	catch(GeneralError& error) {
		// first free memory
		delete rtBuckByteVectp;
		delete rtBuckDirp;
      	
      		GeneralError e("AccessManagerImpl::SingleBucketDepthFirst::operator ==>");
      		error += e;
      		throw error;
      	}
      	catch(...){
		delete rtBuckByteVectp;
		delete rtBuckDirp;
		throw;      	
      	}
			
	//free up memory
      	delete rtBuckByteVectp;
      	rtBuckByteVectp = 0;
      	delete rtBuckDirp;					
      	rtBuckDirp = 0;
}//AccessManagerImpl::SingleBucketDepthFirst::operator

void AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap(
							unsigned int maxDepth,
                					vector<DirChunk>& inputChunkVect,
                					vector<char>* &byteVectp,
                					vector<DiskRootBucketHeader::dirent_t>* &dirVectp)
//precondition:
//	The inputChunkVect is a non-empty vector of DirChunks. byteVectp & dirVectp are two
//	pointers pointing to 0.
//processing:
//	gradually remove each DirChunk from the input vector and fill the byte vector
//	with the corresponding DiskDirChunk instances. Also update accordingly the directory vector
//	Try to conservatively allocate memory space and free up unused memory whenever possible.
//postcondition:
//	inputChunkVect is an empty vector. All previously stored DirChunks are now stored in a
//	DiskDirChunk form in the byte vector pointed to by byteVectp. A directory is also created
//	and dirVectp points at it.
{
	//ASSERTIONS...
	//not empty input vector
	if(inputChunkVect.empty())
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> Empty input vector!\n");			       		
	
	//vector pointers should point to NULL
	if(byteVectp || dirVectp)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> Output pointers are not NULL!\n");
	
	//try to reserve space for the two vectors according to the root directory lower bound
	//NOTE: an STL vector implementation will try to reserve at least the requested amount, maybe more will be allocated (unless
	//							^^^^^^^^^
	//      the memory request cannot be fulfilled and a bad_alloc exception is thrown)
	
	byteVectp = new vector<char>;
	dirVectp = new vector<DiskRootBucketHeader::dirent_t>;
	
	//mem allocation try for the byte vector
	memSize_t memReqForByteV = rootDirSzLowBound; //byte vector requires at least the sum of the sizes in the
						//ChunkHeaders of the input vector		
      	float reduction_factor = 2.0/3.0; //on failure, try to allocate the 2/3 of the requested memory
      	memSize_t minSize = sizeof(inputChunkVect.front().gethdr().size); //reserve at least the size of the first DirChunk!	
	try{		
		memReqForByteV =- ::persistentReserveForSTLVector(*byteVectp, memReqForByteV, reduction_factor, minSize);
	}
	catch(std::bad_alloc&){
		GeneralError error(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> Memory allocation request failed!\n");
		//log error to error log file
		accmgr->geterrorLogStream() << error << endl;
		//output to screen
	        accmgr->getoutputLogStream() << "(File: " << __FILE__ << ", line: " << __LINE__ << "): " << "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> Memory allocation request failed!\n" << endl;		
		
		delete byteVectp;
		byteVectp = 0; // leave pointer into a consistent state
		delete dirVectp;
		dirVectp = 0;
		throw; //throw bad_alloc, nothing we can do!
	}//catch

	//mem allocation try for the directory vector	
	int memReqForDirV = inputChunkVect.size(); //directory requires exactly the number of elements in the input vector	
      	reduction_factor = 2.0/3.0; //on failure, try to allocate the 2/3 of the requested memory
      	minSize = sizeof(DiskRootBucketHeader::dirent_t); //reserve at least the size of an entry
	try{		
		memReqForDirV =- ::persistentReserveForSTLVector(*dirVectp, memReqForDirV, reduction_factor, minSize);
		//**NOTE** memReqForDirV might become < 0 if more bytes are reserved!
	}
	catch(std::bad_alloc&){
		GeneralError error(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> Memory allocation request failed!\n");
		//log error to error log file
		accmgr->geterrorLogStream() << error << endl;
		//output to screen
	        accmgr->getoutputLogStream() << "(File: " << __FILE__ << ", line: " << __LINE__ << "): " << "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> Memory allocation request failed!\n" << endl;		
		
		delete byteVectp;
		byteVectp = 0; // leave pointer into a consistent state
		delete dirVectp;
		dirVectp = 0;
		throw; //throw bad_alloc, nothing we can do!
	}//catch
	
	DiskRootBucketHeader::dirent_t currByteOffset = 0; //byte offset in the byte vector
	//loop: while the input vector is not empty
	while(!inputChunkVect.empty()) {
	
		//create a DiskDirChunk instance from the leftmost DirChunk      		
      		DiskDirChunk* chnkp = 0;
      		try{        		
                        chnkp = accmgr->dirChunk2DiskDirChunk(inputChunkVect.front(), maxDepth);
	      	}
         	catch(GeneralError& error) {
         		GeneralError e("AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> ");
         		error += e;
         		if(chnkp) delete chnkp;
                	delete byteVectp;
                	byteVectp = 0; // leave pointer into a consistent state
                	delete dirVectp;
                	dirVectp = 0;
         		throw error;
         	}
         	catch(...){
         		if(chnkp) delete chnkp;
                	delete byteVectp;
                	byteVectp = 0; // leave pointer into a consistent state
                	delete dirVectp;
                	dirVectp = 0;
         		throw;         	
         	}		
				
		//remove the leftmost dirChunk from the input vector		
		//***NOTE:*** references/pointers/iterators to the input vector's elements are now INVALID!
		inputChunkVect.erase(inputChunkVect.begin());
				
		//resize the byte vector in order to place the new DiskDirChunk
		// NOTE: reservation is no good here because the DiskDirChunk's bytes will be memcopied to the vector
		//	data area and NOT inserted through a vector method
				
		// 1st calculate the size of the new chunk
		unsigned int chnkSz;
		
              	//If this is an artificially chunked dir chunk
              	if(AccessManagerImpl::isArtificialChunk(chnkp->hdr.local_depth)){
                	
              		//ASSERTION: not a null range_to_order-code mapping
              		if(!chnkp->rng2oc){
              			delete chnkp;
                 		delete byteVectp;
                 		byteVectp = 0; // leave pointer into a consistent state
                 		delete dirVectp;
                 		dirVectp = 0;
                 		              		
      				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> NULL range to order-code mapping for artificial dir chunk!\n");
      			}// end if
                		
              		//fill array with number of artificial members per dimension
                      	vector<unsigned int> noMembers;
              		noMembers.reserve(chnkp->hdr.no_dims);
              		for(int i = 0; i<chnkp->hdr.no_dims; i++){
      				noMembers.push_back(chnkp->rng2oc[i].noMembers);                		
              		}//end for
              		try{
              			//now, recalculate the size in order to include the range_to_order-code mapping
                      		chnkSz = DirChunk::calculateStgSizeInBytes(int(chnkp->hdr.depth),
                      							  maxDepth,
                      							  chnkp->hdr.no_dims,
                      							  chnkp->hdr.no_entries,
                      							  int(chnkp->hdr.local_depth),
                      							  chnkp->hdr.next_local_depth,
                      							  &noMembers[0] //argument for number of artificial members per dimension
                      							  );                		
               		}//try
              		catch(GeneralError& error){
              			GeneralError e("AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> ");
              			error += e;
              			delete chnkp;
                 		delete byteVectp;
                 		byteVectp = 0; // leave pointer into a consistent state
                 		delete dirVectp;
                 		dirVectp = 0;
                               	throw error;
              		}//catch
              		catch(...){
              			delete chnkp;
                 		delete byteVectp;
                 		byteVectp = 0; // leave pointer into a consistent state
                 		delete dirVectp;
                 		dirVectp = 0;
                               	throw;              		
              		}							
                 		
              	}//end if
              	else { //this is a regular dir chunk
              		#ifdef DEBUGGING
               		chnkSz = DirChunk::calculateStgSizeInBytes(int(chnkp->hdr.depth),
               							  maxDepth,
               							  chnkp->hdr.no_dims,
               							  chnkp->hdr.no_entries);
               		//ASSERTION: since this is not an artif. chunk, the first size calculation should still hold.
               		if(chnkSz != inputChunkVect.front().gethdr().size){
              			delete chnkp;
                 		delete byteVectp;
                 		byteVectp = 0; // leave pointer into a consistent state
                 		delete dirVectp;
                 		dirVectp = 0;
      				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> size mismatch for dir chunk!\n");
      			}//end if
      			#else
      				// no need to recalculate the size
      				chnkSz = inputChunkVect.front().gethdr().size;
      			#endif
              	}//else			
		
		// now try to resize
		#ifdef DEBUGGING
			if(currByteOffset != byteVectp->size()){
              			delete chnkp;
                 		delete byteVectp;
                 		byteVectp = 0; // leave pointer into a consistent state
                 		delete dirVectp;
                 		dirVectp = 0;
				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> vector size mismatch!\n");			
			}//end if
		#endif
		try{						
			byteVectp->resize(currByteOffset + chnkSz);
		}
		catch(std::bad_alloc&) { //watch for a bad_alloc
        		//if caught one then
       			//trim input vect's capacity to free up memory space
       			::trimSTLvectorsCapacity(inputChunkVect);
       			
       			//try again to resize
        		try{						
        			byteVectp->resize(currByteOffset + chnkSz);
        		}
       			catch(std::bad_alloc&) {
       				//if unable again then throw bad_alloc, nothing else we can do
                		GeneralError error(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> Memory allocation request failed!\n");
                		//log error to error log file
                		accmgr->geterrorLogStream() << error << endl;
                		//output to screen
                	        accmgr->getoutputLogStream() << "(File: " << __FILE__ << ", line: " << __LINE__ << "): " << "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> Memory allocation request failed!\n" << endl;		
                		
              			delete chnkp;
                 		delete byteVectp;
                 		byteVectp = 0; // leave pointer into a consistent state
                 		delete dirVectp;
                 		dirVectp = 0;
                		throw; //throw bad_alloc, nothing we can do!
       			}//catch
		}//catch
		
		//place the DiskDirChunk in the byte vector         	
      		size_t chnk_size = 0;
      		size_t hdr_size = 0;
      		char* nextFreeBytep = &(*byteVectp)[currByteOffset];
      		try{      		
                        accmgr->placeDiskDirChunkInBcktBody(chnkp, maxDepth, nextFreeBytep, hdr_size, chnk_size);			
		}
         	catch(GeneralError& error) {
			//free up memory
      			delete chnkp;
         		delete byteVectp;
         		byteVectp = 0; // leave pointer into a consistent state
         		delete dirVectp;
         		dirVectp = 0;
         		
         		GeneralError e("AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==>");
         		error += e;
         		throw error;
         	}//catch
         	catch(...){
			//free up memory
      			delete chnkp;
         		delete byteVectp;
         		byteVectp = 0; // leave pointer into a consistent state
         		delete dirVectp;
         		dirVectp = 0;

         		throw;         	
         	}
         	
         	//free up memory
         	delete chnkp;
         	chnkp = 0;
         	
         	#ifdef DEBUGGING         	
                        //ASSERTION  : no chunk size mismatch         	
                        if(chnkSz != chnk_size){
		         	//free up memory
                 		delete byteVectp;
                 		byteVectp = 0; // leave pointer into a consistent state
                 		delete dirVectp;
                 		dirVectp = 0;
                                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap ==> ASSERTION: DirChunk size mismatch!\n");
                        }//if
         	#endif
         					
		//update the directory vector with the appropriate byte offset
		try{
			dirVectp->push_back(currByteOffset);
		}
		//watch for a bad_alloc
		catch(std::bad_alloc&) { //watch for a bad_alloc
        		//if caught one then
       			//trim input vect's capacity to free up memory space
       			::trimSTLvectorsCapacity(inputChunkVect);
			
			//try again to update the directory vector
        		try{
        			dirVectp->push_back(currByteOffset);
        		}
        		catch(std::bad_alloc&) {
       				//if unable again then throw bad_alloc, nothing else we can do
                		GeneralError error(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> Memory allocation request failed!\n");
                		//log error to error log file
                		accmgr->geterrorLogStream() << error << endl;
                		//output to screen
                	        accmgr->getoutputLogStream() << "(File: " << __FILE__ << ", line: " << __LINE__ << "): " << "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> Memory allocation request failed!\n" << endl;		
                		
                 		delete byteVectp;
                 		byteVectp = 0; // leave pointer into a consistent state
                 		delete dirVectp;
                 		dirVectp = 0;
                		throw; //throw bad_alloc, nothing we can do!
			}//catch
		}//catch
		
		// get next "free" byte offset	
		currByteOffset += chnk_size;
			
		//if the next insertion in the byte vector will cause a reallocation of the data
		if(     		(!inputChunkVect.empty()) //provided there are still more chunks to store
						   &&
			(byteVectp->capacity() - byteVectp->size() < inputChunkVect.front().gethdr().size)							   		
		   ) {
       			//trim input vect's capacity to free up memory space
       			::trimSTLvectorsCapacity(inputChunkVect);

			//try to reserve more memory for the byte vect
                      	reduction_factor = 2.0/3.0; //on failure, try to allocate the 2/3 of the requested memory
                      	minSize = sizeof(inputChunkVect.front().gethdr().size); //reserve at least the size of the first DirChunk!
                      	//if the current memory Requirement has been decreased too much, updated it, else leave it as it is
                      	if(memReqForByteV < minSize)
                      		memReqForByteV = minSize * inputChunkVect.size(); //an estimation of the required space
                	try{		
                		memReqForByteV =- ::persistentReserveForSTLVector(*byteVectp, memReqForByteV, reduction_factor, minSize);                		
				//**NOTE** memReqForByteV might become < 0 if more bytes are reserved!				                		
                	}
                	catch(std::bad_alloc&){
                		GeneralError error(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> Memory allocation request failed!\n");
                		//log error to error log file
                		accmgr->geterrorLogStream() << error << endl;
                		//output to screen
                	        accmgr->getoutputLogStream() << "(File: " << __FILE__ << ", line: " << __LINE__ << "): " << "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> Memory allocation request failed!\n" << endl;		
                		
                 		delete byteVectp;
                 		byteVectp = 0; // leave pointer into a consistent state
                 		delete dirVectp;
                 		dirVectp = 0;
                		throw; //throw bad_alloc, nothing we can do!
                	}//catch			
        	}//end if			
        			
		//if the next insertion in the dir vector will cause a reallocation of the data
		if(     		(!inputChunkVect.empty()) //provided there are still more chunks to store
						   &&
			(dirVectp->capacity() - dirVectp->size() < sizeof(DiskRootBucketHeader::dirent_t))							   		
		   ) {
       			//trim input vect's capacity to free up memory space
       			::trimSTLvectorsCapacity(inputChunkVect);

			//try to reserve more memory for the byte vect
                      	reduction_factor = 2.0/3.0; //on failure, try to allocate the 2/3 of the requested memory
                      	minSize = sizeof(DiskRootBucketHeader::dirent_t); //reserve at least the size of the first DirChunk!
                      	//if the current memory Requirement has been decreased too much, updated it, else leave it as it is
                      	if(memReqForDirV < minSize)
                      		memReqForDirV = minSize * inputChunkVect.size();
                	try{		
                		memReqForDirV =- ::persistentReserveForSTLVector(*dirVectp, memReqForDirV, reduction_factor, minSize);
                		//**NOTE** memReqForDirV might become < 0 if more bytes are reserved!
                	}
                	catch(std::bad_alloc&){
                		GeneralError error(__FILE__, __LINE__, "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> Memory allocation request failed!\n");
                		//log error to error log file
                		accmgr->geterrorLogStream() << error << endl;
                		//output to screen
                	        accmgr->getoutputLogStream() << "(File: " << __FILE__ << ", line: " << __LINE__ << "): " << "AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap() ==> Memory allocation request failed!\n" << endl;		
                		
                 		delete byteVectp;
                 		byteVectp = 0; // leave pointer into a consistent state
                 		delete dirVectp;
                 		dirVectp = 0;
                		throw; //throw bad_alloc, nothing we can do!
                	}//catch			
        	}//end if			
        }//end while loop

        //trim byte vector if needed, to free up memory
        if(byteVectp->capacity() > byteVectp->size())
        	::trimSTLvectorsCapacity(*byteVectp);
        // trim dir vector  if needed
        if(dirVectp->capacity() > dirVectp->size())
        	::trimSTLvectorsCapacity(*dirVectp);
}//AccessManagerImpl::SingleBucketDepthFirst::createRootBucketVectorsInHeap					

void AccessManagerImpl::putChunksIntoBuckets(				
				const CubeInfo& cbinfo,
				CostNode* const costRoot,
				unsigned int where2store,
				const string& factFile,
				vector<DirChunk>* const dirChunksRootDirVectp,
				DirEntry& returnDirEntry,
				const AccessManager::CBFileConstructionParams& constructionParams
				) const
//precondition:
//	(costRoot points at a CostNode corresponding to a directory chunk R that will be stored
//	in the root bucket, while the subtrees hanging from there will be stored
//	to different buckets.)  ||
//	(costRoot points at a large data chunk that will be stored according to a large data chunk resolution method)
//postcondition:	
//	in the 1st case: the subtrees are stored in buckets and in dirChunksRootDirVectp has been
//	inserted an instance of the dirchunk R. Also the returnDirEntry contains the bucket id
//	of the root bucket and the chunk slot of the dirchunk R.
//	in the 2nd case: the data chunk is stored in some way according to the used method. In any case,
//	returnDirEntry is appropriately updated to show at the bucket and slot of a directory chunk that can
//	lead us to the original large data chunk (e.g., for methods that exploit further chunking) or directly to the stored
//	large data chunk (e.g. large_bucket method). dirChunksRootDirVectp contains instances of all the required directory chunks
//	in the artificial chunking that point of course to the stored data chunks (parts of the original large data chunk).
//
//	On return (for 1st case) all the children cost nodes of costRoot will be deleted and memory freed. All children entries
//	from the child vector of costRoot will be removed. (i.e., it will be empty)
{
	int maxDepth = cbinfo.getmaxDepth();
	// (I) if this is a directory chunk (i.e., not a leaf chunk)
	if(isDirChunk(costRoot->getchunkHdrp()->depth, costRoot->getchunkHdrp()->localDepth, costRoot->getchunkHdrp()->nextLocalDepth, maxDepth)){	
		// ASSERTION1: assert that the size of the tree under costRoot is > DiskBucket::bodysize
		unsigned int szBytes = 0;
		CostNode::calcTreeSize(costRoot, szBytes);
		if(szBytes <= DiskBucket::bodysize && costRoot->getchunkHdrp()->id.getcid() != "root")
			//then storeSingleTreeinCUBE_FileBucket should have been called instead!
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION1: wrong input tree for putChunksIntoBuckets\n");       							

		// 1. define a vector to hold the total cost of each sub-tree hanging from the chunk
		// ASSERTION :  children vector is not empty
		if(costRoot->getchild().empty())
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION: empty children vector in directory chunk!\n");       									
		vector<unsigned int> costVect(costRoot->getchild().size());
	  	// 2. init costVect	  	
	  	int i = 0;
		for(vector<CostNode*>::const_iterator iter = costRoot->getchild().begin();
		   iter != costRoot->getchild().end(); ++iter) {
			szBytes = 0;			
			CostNode::calcTreeSize((*iter), szBytes);
			costVect[i] = szBytes;
			i++;			
		}//end for
				
		// 3. The costRoot represents the current chunk. We want to store this chunk in the dirChunksRootDirVectp.In order
		//    to do that we need to create a DirChunk and fill appropriatelly its entries, then insert it in
	     	//    the vector at position where2store.
	     	
	     	// 3.1. Create the vector holding the chunk's entries.
	     	//      NOTE: the default DirEntry constructor should initialize the member BucketID
	     	//	with a BucketID::null constant.
	     	vector<DirEntry> entryVect(costRoot->getchunkHdrp()->totNumCells);
	     			
		// 4. We have 3 cases (A,B,C) that we have to deal with, arising from the contents of costVect
		//	case A: T<=costVect[i]<=B
		//	case B: costVect[i] < T
		//	case C: costVect[i] > B
		//	case D: EMPTY cell		
		//    NOTE: the "empty" case is handled by the default DirEntry constructor.
		vector<CaseStruct> caseAvect, caseBvect, caseCvect; // one vector for each case
		
		// 4.1. Init caseAvect, caseBvect, caseCvect
		vector<unsigned int>::const_iterator icost = costVect.begin();
		vector<ChunkID>::const_iterator icid = costRoot->getcMapp()->getchunkidVectp()->begin();
		while(icost != costVect.end() && icid != costRoot->getcMapp()->getchunkidVectp()->end()){
			if((*icost) >= DiskBucket::BCKT_THRESHOLD && (*icost) <= DiskBucket::bodysize){
				CaseStruct h = {(*icid),(*icost)};
				caseAvect.push_back(h);
			}
			else if((*icost) < DiskBucket::BCKT_THRESHOLD) {
				CaseStruct h = {(*icid),(*icost)};
				caseBvect.push_back(h);			
			}
			else {//(*icost) > DiskBucket::bodysize)
				assert((*icost) > DiskBucket::bodysize);
				CaseStruct h = {(*icid),(*icost)};
				caseCvect.push_back(h);						
			}			
			++icost;
			++icid;		
		}// end while
		
		//ASSERTION
		if(caseAvect.empty() && caseBvect.empty() && caseCvect.empty())
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION: did not find chunks to include in construction cases!\n");
		
		// 5. Process each list separately:
		if(!caseAvect.empty()){		
        		// 5.1 case A
        		for(vector<CaseStruct>::const_iterator i = caseAvect.begin(); i!=caseAvect.end(); ++i) {
        			const CostNode* childNodep = costRoot->getchildById((*i).id);
        			//ASSERTION
        			if(!childNodep)
        				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION: child node not found!\n");
        			if(isDataChunk(childNodep->getchunkHdrp()->depth, childNodep->getchunkHdrp()->localDepth, childNodep->getchunkHdrp()->nextLocalDepth, maxDepth)){
        				//then child node is a data chunk
                			DirEntry e;
                			try{
                        			storeDataChunkInCUBE_FileBucket(cbinfo,
                        						childNodep,
                        						factFile,
                        						e);			
                			}
                                       	catch(GeneralError& error) {
                                       		GeneralError e("AccessManagerImpl::putChunksIntoBuckets ==> ");
                                       		error += e;
                                       		throw error;
                                       	}
                                       	//ASSERTION2: the returned DirEntry is valid
                                       	if(e.bcktId.isnull() || e.chnkIndex != 0)
                                       		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION2: invalid returned dirchunk entry\n");
                                       		
                			// insert entry at entryVect in the right offset (calculated from the Chunk Id)			
                			Coordinates c;
                			(*i).id.extractCoords(c);
                			//ASSERTION3: check offset calculation
                			unsigned int offs = DirChunk::calcCellOffset(c, costRoot->getchunkHdrp()->vectRange);
                       			if(offs >= entryVect.size()){
                       				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION3: (Case A) entryVect out of range!\n");
                       			}			
                			entryVect[offs] = e;
        				
        			}//end if
        			else {// child node is a chunk tree
                			DirEntry e;
                			try{
                        			storeSingleTreeInCUBE_FileBucket(cbinfo,
                        						childNodep,
                        						factFile,
                        						e,
                        						constructionParams.how_to_traverse);			
                			}
                                       	catch(GeneralError& error) {
                                       		GeneralError e("AccessManagerImpl::putChunksIntoBuckets ==> ");
                                       		error += e;
                                       		throw error;
                                       	}
                                       	//ASSERTION2: the returned DirEntry is valid
                                       	if(e.bcktId.isnull() || e.chnkIndex != 0)
                                       		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION2: invalid returned dirchunk entry\n");
                                       		
                			// insert entry at entryVect in the right offset (calculated from the Chunk Id)			
                			Coordinates c;
                			(*i).id.extractCoords(c);
                			//ASSERTION3: check offset calculation
                			unsigned int offs = DirChunk::calcCellOffset(c, costRoot->getchunkHdrp()->vectRange);
                       			if(offs >= entryVect.size()){
                       				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION3: (Case A) entryVect out of range!\n");
                       			}	                                                                                                                                                                          		
                			entryVect[offs] = e;
                		}//end else
                		
                		// ***IMPORTANT***: Free up memory space, now that we have stored this subtree we dont need it any longer
                		// NOTE: in case of a tree, this deletion will propagate further down to the children, through ~CostNode()
                		costRoot->removeChildFromVector(childNodep->getchunkHdrp()->id);
                		childNodep = 0;
        		} // end for
		}//end if
		
		if(!caseBvect.empty()){
        		// 5.2 case B : do your clustering thing! ;-)
        		// this should return a vector of
        		// DirEntries with 1-1 correspondence with caseBVect. I use this to store
        	 	// in the appropriate positions in entryVect.
        		vector<DirEntry> resultEntryv;
        		resultEntryv.reserve(caseBvect.size());
        		
        		//select between two sub-cases: check if the children of current node are data chunks or dir chunks
        		vector<CostNode*>::const_iterator fst_child_iter = costRoot->getchild().begin();
			if(isDataChunk((*fst_child_iter)->getchunkHdrp()->depth, (*fst_child_iter)->getchunkHdrp()->localDepth, (*fst_child_iter)->getchunkHdrp()->nextLocalDepth, maxDepth)){        		
        			//then we have to put data chunks into clusters
                		try{
                			storeDataChunksInCUBE_FileClusters(
                							cbinfo,
                							caseBvect,
                							costRoot,
                							factFile,
                							resultEntryv,
                							constructionParams.clustering_algorithm);
                							
                		}
                               	catch(GeneralError& error) {
                               		GeneralError e("");
                               		error += e;
                               		throw error;
                               	}								
        		}//end if
        		else{
        			//ASSERTION 3.1: this is a directory chunk
				if(isDirChunk((*fst_child_iter)->getchunkHdrp()->depth, (*fst_child_iter)->getchunkHdrp()->localDepth, (*fst_child_iter)->getchunkHdrp()->nextLocalDepth, maxDepth))
        				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION 3.1: wrong chunk type: dir chunk expected!\n");
        				
        			//then we have to put trees into clusters
                		try{
                			storeTreesInCUBE_FileClusters(
                							cbinfo,
                							caseBvect,
                							costRoot,
                							factFile,
                							resultEntryv,
                							constructionParams.how_to_traverse,
                							constructionParams.clustering_algorithm);
                		}
                               	catch(GeneralError& error) {
                               		GeneralError e("AccessManagerImpl::putChunksIntoBuckets ==> ");
                               		error += e;
                               		throw error;
                               	}			
        		}//end else		
        		
                       	//ASSERTION4:valid returned DirEntries vector
                       	if(resultEntryv.size() != caseBvect.size())
                       		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION4: wrong size of returned vector\n");
        		//update entryVect
        		vector<CaseStruct>::const_iterator caseb_iter = caseBvect.begin();
        		vector<DirEntry>::const_iterator bent_iter = resultEntryv.begin();
        		while(caseb_iter != caseBvect.end() && bent_iter != resultEntryv.end()) {
        			//ASSERTION5: valid bucket id in Direntries
        			if((*bent_iter).bcktId.isnull())
        				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION5: null bucket id\n");				
        			// insert entry at entryVect in the right offset (calculated from the Chunk Id)			
        			Coordinates c;
        			(*caseb_iter).id.extractCoords(c);
        			//ASSERTION6: check offset calculation
        			unsigned int offs = DirChunk::calcCellOffset(c, costRoot->getchunkHdrp()->vectRange);
               			if(offs >= entryVect.size()){
               				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION6: (Case B) entryVect out of range!\n");
               			}			
        			entryVect[offs] = (*bent_iter);											
        					
        			caseb_iter++;
        			bent_iter++;		
        		}//end while
        		
               		// ***IMPORTANT***: Free up memory space, now that we have stored the subtrees of the cluster,
               		//		    we dont need them any longer. In case of trees, this deletion will propagate
               		//		    further down to the children.
			for(vector<CaseStruct>::const_iterator i = caseBvect.begin(); i!=caseBvect.end(); ++i) {
               			costRoot->removeChildFromVector((*i).id);               			
               		}//end for
        		
        	}//end if		
	
		if(!caseCvect.empty()){        						
        		// 5.3 case C
        		for(vector<CaseStruct>::const_iterator i = caseCvect.begin(); i!=caseCvect.end(); ++i) {
                        	DirEntry entry; //will be updated by putChunksIntoBuckets
                       		dirChunksRootDirVectp->reserve(1); // reserve one position for storing this child chunk in the root bucket	
                        	CostNode* childNodep = costRoot->getchildById((*i).id);
        			//ASSERTION
        			if(!childNodep)
        				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION: child node not found!\n");
                        	
                        	try {
                               		putChunksIntoBuckets(cbinfo,
                               			   childNodep,
                               			   //dirChunksRootDirVectp->capacity(), //where2store in the root bucket vector: at the end
                               			   //NOTE: vector capacity might be increased MORE THAN ONE, after the call reserve(1)!!!
                               			   (where2store + 1), //store the next root one position after this one
                               			   factFile,
                               			   dirChunksRootDirVectp,
                               			   entry,
                               			   constructionParams);
                               			   //currIndxInRootBck);
                               	}
                               	catch(GeneralError& error) {
                               		GeneralError err("AccessManagerImpl::putChunksIntoBuckets ==> ");
                               		error += err;
                               		throw error;
                               	}
                               	//ASSERTION 7 & 8: the returned DirEntry is valid
                               	//  invariant: if the chunk that was passed as input to putChunksIntoBuckets() was a directory chunk
                               	//		then it will be stored in the root bucket, also its index in the root chunk vector
                               	// 		will be greater than its parent chunk. The same also holds for data chunks but only
                               	//		when an artificial chunking large-chunk resolution method is employed.
                               	if(isDirChunk(childNodep->getchunkHdrp()->depth, childNodep->getchunkHdrp()->localDepth, childNodep->getchunkHdrp()->nextLocalDepth, maxDepth)
                               					||
				   constructionParams.large_chunk_resolution == AccessManager::equigrid_equichildren){                               	
                                       	if(entry.bcktId != cbinfo.get_rootBucketID())
                                       		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION7: invalid returned dirchunk entry\n");
                			if(entry.chnkIndex < where2store)
                				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION8: invalid returned dirchunk entry\n");
        			}//end if
        			                       		                       	
        			// insert entry at entryVect in the right offset (calculated from the Chunk Id)			
        			Coordinates c;
        			(*i).id.extractCoords(c);
        			//ASSERTION 10: offset calculation
        			unsigned int offs = DirChunk::calcCellOffset(c,costRoot->getchunkHdrp()->vectRange);
               			if(offs >= entryVect.size()){
               				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==>ASSERTION 10: (Case C) entryVect out of range!\n");
               			}			
        			entryVect[offs] = entry;

                		// ***IMPORTANT***: Free up memory space by deleting this child node.
                        	// NOTE: this deletion might propagate down further to children nodes at arbitrary depth
                        	// but these (the children) must already have been deleted by now and the corresponding entries removed from
                        	// the vector of children pointers!
                        	#ifdef DEBUGGING
                        	// ASSERTION: indeed no children entries left
                        	if(!childNodep->getchild().empty())   	
                        		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets() ==> Before freeing cost node, not empty vector of children pointers found!");
                        	#endif                		
                		costRoot->removeChildFromVector(childNodep->getchunkHdrp()->id);
                		childNodep = 0;        			        			
        		}//end for
        	}//end if
		
		// 6. Now that entryVect is filled, create the chunk
		// 6.1
		DirChunk newChunk((*costRoot->getchunkHdrp()), entryVect);
		
		// 6.2 insert newChunk in the dirChunksRootDirVectp at position where2store.
		dirChunksRootDirVectp->insert(dirChunksRootDirVectp->begin()+where2store, newChunk);
		
		// 7. update the DirEntry corresponding to the newChunk	and
		//    return it to the caller. Actually, the caller can find this
		//    information but this way is easier and cleaner.
		returnDirEntry.bcktId = cbinfo.get_rootBucketID();
		returnDirEntry.chnkIndex = where2store;		
		
         	// NOTE:  We CANNOT delete costRoot here, in order to free up memory. This deletion will be done by the
         	// caller.  If we deleted in this place,
         	// then a dangling pointer would appear in the parent node, in the vector of children pointers.
         	//        ^^^^^^^^^^^^^^^^
		
		
	} //end if
	else { // II. this is a large data (leaf) chunk
		//ASSERTION 11: assert that this is a data chunk
		if(!isDataChunk(costRoot->getchunkHdrp()->depth, costRoot->getchunkHdrp()->localDepth, costRoot->getchunkHdrp()->nextLocalDepth, maxDepth))			
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION 11: Chunk type error: Data Chunk expected!\n");	

		// ASSERTION :  children vector should be empty
		if(!costRoot->getchild().empty())
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION: non-empty children vector in data chunk!\n");       															
			
		// ASSERTION 12: assert that this is a large data chunk
		if(!isLargeChunk(costRoot->getchunkHdrp()->size))
			//then storeSingleDataChunkInCUBE_FileBucket should have been called instead!
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putChunksIntoBuckets ==> ASSERTION 12: chunk size error: large data chunk expected!\n");       										
		
		// We will have to use large data chunk storage resolution to store this leaf chunk
		// this routine will eventually call putChunksIntoBuckets (at least for the methods that
		// exploit artificial chunking, it will. In particular, it will replace costRoot with
		// a new costNode tree, i.e. extending the chunking further. then it will call again
		// putchunksIntoBuckets to do the rest.)
		try{
			storeLargeDataChunk( 	cbinfo,
         					costRoot,
         					where2store,
         					factFile,
         					dirChunksRootDirVectp,
         					returnDirEntry,
						constructionParams);
		}
               	catch(GeneralError& error) {
               		GeneralError e("AccessManagerImpl::putChunksIntoBuckets ==> ");
               		error += e;
               		throw error;
               	}
               	
         	// NOTE:  We CANNOT delete costRoot here, in order to free up memory. This deletion will be done by the
         	// caller.  If we deleted in this place,
         	// then a dangling pointer would appear in the parent node, in the vector of children pointers.
         	//        ^^^^^^^^^^^^^^^^
               				
	}//end else	
} // end of AccessManagerImpl::putChunksIntoBuckets

void AccessManagerImpl::storeLargeDataChunk(
				const CubeInfo& cbinfo,
				const CostNode* const costRoot,
				unsigned int where2store,
				const string& factFile,
				vector<DirChunk>* const dirChunksRootDirVectp,
				DirEntry& returnDirEntry,
				const AccessManager::CBFileConstructionParams& constructionParams //input
        				)const
//precondition:
//	costRoot points at a CostNode corresponding to a large data chunk. 	
//postcondition:
//	the large data chunk corresponding to costRoot has been created (i.e., filled with values from the input file)
//	and stored to CUBE file according to the method described in method_token         				
{
	//ASSERTION 1: assert that this is a data chunk
	if(!isDataChunk(costRoot->getchunkHdrp()->depth, costRoot->getchunkHdrp()->localDepth, costRoot->getchunkHdrp()->nextLocalDepth, cbinfo.getmaxDepth()))	
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeLargeDataChunk ==> ASSERTION1: input chunk is NOT a data chunk");
			
	//ASSERTION 2: assert that the data chunk's size is greater than the free space in the bucket
	if( !(AccessManagerImpl::isLargeChunk(costRoot->getchunkHdrp()->size)) )
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeLargeDataChunk ==> ASSERTION2: wrong size for large data chunk");
	
       	/***NOTE***
       		For methods that employ artificial chunking,
       		we will need a look up table to record the chunk ids that have been extended.
       		However, this table SHOULD BE USED ONLY during the chunk expression creation time (i.e.,
       		during the dimension data restriction evaluation phase) and not
       		when accessing the CUBE File for retrieving cube data because it will stall the evaluation time due to the
       		searching in this table. Of course, the same holds also for the dimension data restriction evaluation. However,
       		we don't want to burden the CUBE file structure with such an overhead and so we defer the solution to an
       		efficient dimension data storage structure, which is something open for now. :)
       		Therefore the actual implementation of this table
       		relates strongly to the implementation of the DIMENSION File.
       		The table created now will be used only if the dimension data are stored within Sisyphus and thus
       		the chunk expression creation is not done in an external DBMS.
       		
       		Another solution could be to store the extended hierarchies for each dimension inside each large data chunk, e.g.,
       		in the shore record header. For each dimension we would have to store a set of order-code ranges corresponding to the
       		grain level and the new chunk-id domain that each such set maps to. For example,
       		DIMENSIONA([9,11]=>0, [12,14]=>1, [15,16]=>2), etc. This way we will not restrict the body space of the bucket and we
       		will not make dimension evaluation too complicated and the processing overhead during evaluation is minimal, since the
       		shore record header is always pinned anyway when pinning a record.
       	**********/       	       		       	       	
		
	//choose method for resolving large data chunk storage
	switch(constructionParams.large_chunk_resolution){
        	//CASE1: equi-grid_equi-children method
        	case AccessManager::equigrid_equichildren:
        		{
                		// find the maximum number of directory entries (E) that fit in a bucket
                		// Since this bucket will store only a single DirChunk,it will occupy one entry in the bucket internal directory
                		// (DiskBucket::direntry_t)
                		unsigned int maxDirEntries = ( DiskBucket::bodysize - sizeof(DiskBucketHeader::dirent_t) )/sizeof(DiskDirChunk::DirEntry_t);
                		unsigned int noDimensions = cbinfo.get_num_of_dimensions();        		
                   		//call equi-grid-equi-children method
               			EquiGrid_EquiChildren method(maxDirEntries, noDimensions, costRoot->getchunkHdrp()->vectRange);

                   		try{
                   			method(this,cbinfo,costRoot,where2store,factFile,dirChunksRootDirVectp,returnDirEntry,constructionParams);
                   		}
                               	catch(GeneralError& error) {
                               		GeneralError e("AccessManagerImpl::storeLargeDataChunk ==> ");
                               		error += e;
                               		throw error;
                               	}			
                		break;
        		}
               	default:
               		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeLargeDataChunk ==> Unknown method for storing large data chunks\n");
               		break;		
   	}//end switch					
}//AccessManagerImpl::storeLargeDataChunk

AccessManagerImpl::EquiGrid_EquiChildren::EquiGrid_EquiChildren(int e, int d, const vector<LevelRange>& rangeVect): maxDirEntries(e), noDims(d)
//precondition:
//      It is assumed that this routine is called on behalf of a *large data chunk*. Therefore, for example
//      it does not allow pseudo levels at all for input chunk (since it is a data chunk).
//processing:
//      calculates the number of members (equal for all dimensions) of the newly inserted levels. Decides
//      which of these new levels will be pseudo levels. Also calculates the maximum number of children
//      (equal for all members of the same dimension) of a member of a new level.
//postcondition:
//      All data members have valid values.
{
       		
	//find the number of artificial partitions (i.e., No of members of new level) along each dimension (m = floor(E**1/N))
	// this will be the same for all dimensions (equi-grid)       		
       	noMembersNewLevel = int( ::floor( ::pow(double(maxDirEntries),1.0/double(noDims)) ) );
       			
       	//find the maximum number of children under each new member (c = N/m), equal for all members of a dimension (equi-children).
       	// One entry per dimension
       	maxNoChildrenPerDim.reserve(noDims);//the order of the dimensions in this vector are the same with the interleaving order
       	noPseudoLevels = 0; //init pseudo level counter
       	vector<unsigned int> noMembersGrainVect(noDims); //init vector to hold no of grain members per dim                       	
       	for(int i=0; i<noDims; i++){
       		//if this not a pseudo level
       		if(rangeVect[i].lvlName.find("Pseudo-level") == string::npos){
               		// calc no of members on each dimension at the grain level of the original chunk                       		
        		noMembersGrainVect[i] = rangeVect[i].rightEnd - rangeVect[i].leftEnd + 1;
                        		
        		// calc the max number of children at the grain level that a parent member can have
        		// if the new members are fewer than the existing members at the grain level
        		if(noMembersGrainVect[i] >= noMembersNewLevel){
        			//then the hierarchy is meaningful
                		maxNoChildrenPerDim.push_back(int(::ceil(double(noMembersGrainVect[i])/double(noMembersNewLevel))));
                	}//end if
                	else {
                		//there is no meaning in creating a hierarchy along this dimension; instead create a pseudo level
                		maxNoChildrenPerDim.push_back(0);
                		noPseudoLevels++; //one more pseudo level
                	}//end else
        	}//end if
        	else {  //this is a Pseudo level
        		//since we apply (for now) this method only to large DATA chunks (and not dir chunks)
        		//there shouldn't be any pseudo level at the data level
        		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::EquiGrid_EquiChildren() ==> Pseudo level encountered in data chunk\n");
        	}//end else
       	}//for
                       	
       	//If all new levels will be pseudo levels
       	if(noPseudoLevels == noDims){                       	
                //then we have to reduce the number of members of the new levels
                // we will make it equal with the minimum number of grain level members for each dimension
                //noMembersNewLevel = minimum(noMembersGrain)  such that minimum(noMembersGrain) != 1

                // find the minimum number of grain members in the original chunk.
                vector<unsigned int>::const_iterator minimumMembersGrainIter = min_element(noMembersGrainVect.begin(), noMembersGrainVect.end());
                //if the min number is not equal with 1
                if(*minimumMembersGrainIter != 1) {
                        // The *new* number of new members inserted will be:
                        noMembersNewLevel = *minimumMembersGrainIter;
                        //Now, the maximum number of children per dim becomes:
                        for(int i=0; i<noDims; i++){
                                maxNoChildrenPerDim[i] = int(::ceil(double(noMembersGrainVect[i])/double(noMembersNewLevel)));
                        }//for
                        // No pseudo levels now
                        noPseudoLevels = 0;
                }//end if
                else {//else if min number equals 1
                        //find all dimensions with only 1 member and make them pseudo levels
                        noPseudoLevels = 0;
                        for(int i=0; i<noDims; i++){
                                if(noMembersGrainVect[i] == 1){
                                        //then this must be a
                                        maxNoChildrenPerDim[i] = 0; //pseudo level
                                        noPseudoLevels++;
                                        //also put maxint in then num of members vect in order to get it out of the way
                                        noMembersGrainVect[i] = UINT_MAX; //numeric_limits<unsigned int>::max();
                                }//end if
                        }//for
                        if(noPseudoLevels == noDims)
                                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::EquiGrid_EquiChildren() ==> Found large data chunk consisting of only ONE(!!!) cell. Sorry, cant handle this case, I think you should consider a re-design of your cube!!! :-)");
                        //Now that the 1's are out of the way,
                        //find the minimum No of grain members in the original chunk that is != 1
                        minimumMembersGrainIter = min_element(noMembersGrainVect.begin(), noMembersGrainVect.end());
                        // The *new* number of new members inserted will be:
                        noMembersNewLevel = *minimumMembersGrainIter;
                        //Now, the maximum number of children per dim becomes:
                        for(int i=0; i<noDims; i++){
                                //if not a pseudo level
                                if(maxNoChildrenPerDim[i] != 0)
                                        maxNoChildrenPerDim[i] = int(::ceil(double(noMembersGrainVect[i])/double(noMembersNewLevel)));
                        }//for
                }//end else
       	} //end if                       	
}//end AccessManagerImpl::EquiGrid_EquiChildren::EquiGrid_EquiChildren()


void AccessManagerImpl::EquiGrid_EquiChildren::operator()(
					const AccessManagerImpl* const accmgr,
					const CubeInfo& cbinfo, //input
        				const CostNode* const costRoot,  //input
        				unsigned int where2store,      //input
        				const string& factFile,        //input
        				vector<DirChunk>* const dirChunksRootDirVectp,     //output
        				DirEntry& returnDirEntry,      //output
        				const AccessManager::CBFileConstructionParams& constructionParams //input
					)
//precondition:
//	The input CostNode corresponds to a large data chunk. The maximum number of dir entries that fit in a DiskBucket
//	has been calculated and stored as a member in this class function. Also in the constructor of the class function
//	the number of new members inserted (equi-grid) has been calculated and the number of children of these members in
//	the grain level (equi-children) per dimension has also been calculated. Both are kept as data members of the
//	function class EquiGrid_EquiChildren.
//processing:
//	This routine will create a two level costNode tree, which results from the application of the method on the
//	input large data chunk. Then it will call AccessManagerImpl::putchunksIntoBuckets in order to take care the storage
//	of the newly created tree into the CUBE File. The AccessManagerImpl::putchunksIntoBuckets might recursively call again
//	this routine, if it encounters again a large data chunk.
//postcondition:
//	The chunk corresponding to the CostNode pointed to by costRoot has been instantiated in the form of a DirChunk and
//	is stored in the vector that are stored the root bucket's dir chunks. All the children chunks pointed to by this root are
//	stored in separate buckets. Note that in the case that a child is still a large data chunk this routine will be called
//	recursively (by putChunksIntoBuckets()) and therefore the corresponding DirChunk will be also inserted in the root bucket
//	vector of dir chunks. Finally, a valid directory entry is returned that points to the dirChunk corresponding to costRoot.
{
	unsigned int maxDepth = cbinfo.getmaxDepth();
	unsigned int numFacts = cbinfo.getnumFacts();

	//Assert that this is a large data chunk

	//ASSERTION 1: assert that this is a data chunk
	if(!isDataChunk(costRoot->getchunkHdrp()->depth, costRoot->getchunkHdrp()->localDepth, costRoot->getchunkHdrp()->nextLocalDepth, maxDepth))
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::EquiGrid_EquiChildren::operator() ==> ASSERTION1: input chunk is NOT a data chunk");

	//ASSERTION 2: assert that the data chunk's size is greater than the free space in the bucket
	if( !AccessManagerImpl::isLargeChunk(costRoot->getchunkHdrp()->size) )
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::EquiGrid_EquiChildren::operator() ==> ASSERTION2: wrong size for large data chunk");

	// create a new chunk header for the input chunk by copying the old one
	// the following members must be updated to reflect the new dir chunk that will be created
	// in the place of the original large data chunk:
	//		localDepth, nextLocalDepth, rlNumCells, totNumCells, vectRange, size.
	ChunkHeader* newHeaderForRootp = new ChunkHeader( *(costRoot->getchunkHdrp()) );

	// Update the local depth and the next field
	// if this is a classic data chunk
	if(newHeaderForRootp->localDepth == Chunk::NULL_DEPTH) {
		//the local depth should take the minimum value
		newHeaderForRootp->localDepth = Chunk::MIN_DEPTH;
		newHeaderForRootp->nextLocalDepth = true; //indicates that another level will follow
	}//end if
	//else if this is an artificially formed data chunk then
	else if(newHeaderForRootp->localDepth > Chunk::MIN_DEPTH && newHeaderForRootp->nextLocalDepth == false){
		// the local depth should remain the same, just the next flag must be set on
		newHeaderForRootp->nextLocalDepth = true;
	}//end else if
	else{ //Something is wrong here!!
		delete newHeaderForRootp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::EquiGrid_EquiChildren::operator() ==> wrong local depth or next flag value");
	}//end else

	//Update the number of total cells
	//*NOTE* The number of newly inserted  members is the same for all dimensions (equi-grid). Therefore the total num of cells
	//equals with this number raised in the number of dimensions.However we have to exclude the dimensions whose new level
	//will be a pseudo level.
	newHeaderForRootp->totNumCells = int(::pow(double(noMembersNewLevel), double(noDims-noPseudoLevels)));
	
	//Update the new header with the new ranges on each dimension (equal for all dims) for the 1st level
	for(int dimi = 0; dimi < noDims; dimi++){
		//if this does not correspond to a pseudo level
		if(maxNoChildrenPerDim[dimi] != 0){
			newHeaderForRootp->vectRange[dimi].leftEnd = Chunk::MIN_ORDER_CODE;
			newHeaderForRootp->vectRange[dimi].rightEnd = noMembersNewLevel-1;
		}//end if
		else { //then this will be a pseudo level
			newHeaderForRootp->vectRange[dimi].lvlName = string("Pseudo-level");
			newHeaderForRootp->vectRange[dimi].leftEnd = LevelRange::NULL_RANGE;
			newHeaderForRootp->vectRange[dimi].rightEnd = LevelRange::NULL_RANGE;
		}//end else
	}//end for

	//if a cell map does not exist for the input chunk then create one, we will need it next
	if(!costRoot->getcMapp()){
	       	//Scan input file for prefix matches with the chunk id of the original (large) chunk and create
	       	//corresponding cell map
		const_cast<CostNode*>(costRoot)->setcMapp(Chunk::scanFileForPrefix(factFile, newHeaderForRootp->id.getcid(), true));
	}//end if

	// create an empty cell map for the new root
	CellMap* newmapp = new CellMap;

	//create the corresponding CostNode without the children attached yet and an empty cell map
	CostNode* newCostRoot = new CostNode(newHeaderForRootp, newmapp);

	//no more needed, free up space
        delete newHeaderForRootp;
        newHeaderForRootp = 0;
        delete newmapp;
        newmapp = 0;

       	//now, create and store the new hierarchies per dimension
       	//use a vector of maps (one map per dimension)
       	vector<map<int, LevelRange> >* newHierarchyVectp = new vector<map<int, LevelRange> >(noDims);
       	try{
       		createNewHierarchies(costRoot->getchunkHdrp()->vectRange, *newHierarchyVectp);
       	}
      	catch(GeneralError& error) {
      		GeneralError e("AccessManagerImpl::EquiGrid_EquiChildren::operator ==> ");
      		error += e;
      		delete newCostRoot;
      		delete newHierarchyVectp;
      		throw error;
      	}
      	catch(...){
      		delete newCostRoot;
      		delete newHierarchyVectp;
      		throw;      	
      	}
      	
      	#ifdef DEBUGGING
      	//print new hierarchy
      	cerr << "The artificial hierarchy will be the following: " << endl;
        for(int dimi = 0; dimi < noDims; dimi++) {
                cout << "____ Dim: " << dimi << " ____" << endl;     	
                for(map<int, LevelRange>::const_iterator mapiter = (*newHierarchyVectp)[dimi].begin(); mapiter != (*newHierarchyVectp)[dimi].end(); mapiter++){
                        cout << mapiter->first << " ==> " << "[" << mapiter->second.leftEnd << ", " << mapiter->second.rightEnd << "]" << endl;
                }//for
                cout << endl;      	
        }//for      	
      	#endif

       	// initialize the number of real cells in the parent node
       	const_cast<ChunkHeader*>(newCostRoot->getchunkHdrp())->rlNumCells = 0;

       	//For each cell of the (new) parent cost node (form it from the new ranges):
       	//first initialize cell-coords with the lower-left cell (***NOTE*** pseudo codes included)
       	Coordinates currCellCoords(noDims, vector<DiskChunkHeader::ordercode_t>(noDims));
	for(int dimi = 0; dimi < noDims; dimi++){
		//if this does not correspond to a pseudo level
		if(maxNoChildrenPerDim[dimi] != 0)
			currCellCoords.cVect[dimi] = Chunk::MIN_ORDER_CODE;
		else { //then this will be a pseudo level
			currCellCoords.cVect[dimi] = LevelMember::PSEUDO_CODE;
		}//end else
	}//end for
	//Create a cell with these coordinates, in the data space defined by the parent chunk
	Cell currentCell(currCellCoords, newCostRoot->getchunkHdrp()->vectRange);
       	#ifdef DEBUGGING
       	int numCellsVisited = 0;
       	#endif	
       	do{
       		//loop invariant: in each iteration a cost node corresponding to a child chunk must be created
       		//		  and attached to the parent ONLY if this child is NOT EMPTY, i.e., it contains at least
       		//		  one data point.
       		
		#ifdef DEBUGGING
		cerr << "Current cell is: " << currentCell << ". Is  it the first cell? "<< currentCell.isFirstCell() <<endl;
		numCellsVisited++;		
		#endif
       		
       		// create a cost node for the corresponding child chunk
       		// We will need a chunk header and a cell map:
       		ChunkHeader* childHeaderp = new ChunkHeader;

       		//chunk id
       		//create chunk id domain from the coordinates
       		string newdomain;
       		ChunkID::coords2domain(currentCell.getcoords(), newdomain);
       		//add new domain as a suffix to the parent chunk id
       		 //take the parent id
       		ChunkID childid(newCostRoot->getchunkHdrp()->id.getcid());
       		//and add the new domain as suffix
       		try{
       			childid.addSuffixDomain(newdomain);
       		}
		catch(GeneralError& error){
			GeneralError e("AccessManagerImpl::EquiGrid_EquiChildren::operator() ==> ");
			error += e;
	      		delete newCostRoot;
	      		delete newHierarchyVectp;
                        delete childHeaderp;
                 	throw error;
		}
		catch(...){
	      		delete newCostRoot;
	      		delete newHierarchyVectp;
                        delete childHeaderp;
                 	throw;		
		}							       		       		       		       		
       			
       		childHeaderp->id.setcid(childid.getcid());
      		
       		#ifdef DEBUGGING
       		cerr << "New chunk id is: "<< childHeaderp->id.getcid() <<endl;
       		#endif
       		
       		//global depth remains the same with the parent
       		childHeaderp->depth = newCostRoot->getchunkHdrp()->depth;
       		
       		//local depth and next flag
       		childHeaderp->localDepth = newCostRoot->getchunkHdrp()->localDepth + 1; //one more than the parent
       		childHeaderp->nextLocalDepth = 	false; // this is a leaf chunk (i.e., data chunk)
       		
       		//number of dimensions
       		childHeaderp->numDim = noDims;
       		       		
       		//range vector for the child chunk:
       		childHeaderp->vectRange.reserve(noDims);
       		//for each coordinate value of the parent node
       		for(int dimindex = 0; dimindex < noDims; dimindex++){
       			// get the corresponding range from the newly created hierarchy
       			childHeaderp->vectRange.push_back( (*newHierarchyVectp)[dimindex][currentCell.getcoords().cVect[dimindex]] );
       		}//end for
       		
       		#ifdef DEBUGGING
       		cerr << "The order code ranges for chunk: " << childHeaderp->id.getcid() << " is:" << endl;
       		for(vector<LevelRange>::const_iterator rngiter = childHeaderp->vectRange.begin(); rngiter != childHeaderp->vectRange.end(); rngiter++, cerr<<" x "){
                        cerr << "[" << rngiter->leftEnd << ", " <<rngiter->rightEnd << "]";       		
       		}//end for
       		cerr << endl;
       		#endif       		
       		
       		// calculate total number of cells as the product of the number of members on each dimension for this chunk
       		int mbproduct = 1; //init product
       		//for each dimension
       		for(int dimindex = 0; dimindex < noDims; dimindex++){
			int nomembers = childHeaderp->vectRange[dimindex].rightEnd - childHeaderp->vectRange[dimindex].leftEnd +1 ;
       			mbproduct *= nomembers;
       		}//end for
       		childHeaderp->totNumCells = mbproduct;

		//create empty cell map for child
       		CellMap* childMapp = 0;
       		
      		// Search original (input) large data chunk's cell map for cells in the range of the child chunk
      		// and create the childs cell map
      		try{
      			childMapp = costRoot->getcMapp()->searchMapForDataPoints(childHeaderp->vectRange, childHeaderp->id);
      		}
		catch(GeneralError& error){
			GeneralError e("AccessManagerImpl::EquiGrid_EquiChildren::operator() ==> ");
			error += e;
	      		delete newCostRoot;
	      		delete newHierarchyVectp;
                        delete childHeaderp;
			//if(childMapp) delete childMapp there is no way for childMapp to be != 0
                 	throw error;
		}
		catch(...){
	      		delete newCostRoot;
	      		delete newHierarchyVectp;
                        delete childHeaderp;
			//if(childMapp) delete childMapp there is no way for childMapp to be != 0
                 	throw;		
		}
		
		//#ifdef DEBUGGING
		//cerr << "Printing the data points found in the parent's CellMap\n";
                //if(childMapp) {
                        //print new map
                //        for(vector<ChunkID>::const_iterator citer = childMapp->getchunkidVectp()->begin(); citer != childMapp->getchunkidVectp()->end(); citer++){
                //                cout << citer->getcid() << endl;
                //        }//end for
                //}//if
                //#endif
									       		       		       		       		      		
      		//if no data point was found in the range of this child chunk
      		if(!childMapp){
      			//DO NOT CREATE any cost node for the child
      			delete childHeaderp;
      			childHeaderp = 0;
               		//get next cell
               		try{
        	       		currentCell.becomeNextCell();
        	       	}
        		catch(GeneralError& error){
        			GeneralError e("AccessManagerImpl::EquiGrid_EquiChildren::operator() ==> ");
        			error += e;
        	      		delete newCostRoot;
        	      		delete newHierarchyVectp;        			
                         	throw error;
        		}
        		catch(...){
        	      		delete newCostRoot;
        	      		delete newHierarchyVectp;        			
                         	throw;        		
        		}//catch      			
      			continue; //next iteration
      		}//end if
      		
      		//else, at least one data point was found
       		       		       		       		       		       		
       		//update number of real cells for child chunk
		childHeaderp->rlNumCells = childMapp->getchunkidVectp()->size();
		
		// calculate size of child chunk
		try{
        		childHeaderp->size = DataChunk::calculateStgSizeInBytes(childHeaderp->depth,
        							  maxDepth,
        							  childHeaderp->numDim,
        							  childHeaderp->totNumCells,
        							  childHeaderp->rlNumCells,
        							  numFacts,
        							  childHeaderp->localDepth);
		}
		catch(GeneralError& error){
			GeneralError e("AccessManagerImpl::EquiGrid_EquiChildren::operator() ==> ");
			error += e;
	      		delete newCostRoot;
	      		delete newHierarchyVectp;
                        delete childHeaderp;
			delete childMapp;						
                 	throw error;
		}
		catch(...){
	      		delete newCostRoot;
	      		delete newHierarchyVectp;
                        delete childHeaderp;
			delete childMapp;						
                 	throw;		
		}							       		       		       		       		
       		       			       		
       		//Also, update parent's members:
   		//insert chunk id into parent node cell map
		if(!const_cast<CellMap*>(newCostRoot->getcMapp())->insert(childHeaderp->id.getcid())){
	      		delete newCostRoot;
	      		delete newHierarchyVectp;
                        delete childHeaderp;
			delete childMapp;						
		
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::EquiGrid_EquiChildren::operator() ==> double entry in cell map");
		}//end if
		        				
       		//update parent number of real cells in the new header by adding one cell, since this parent cell
       		// points to a non-empty child
		const_cast<ChunkHeader*>(newCostRoot->getchunkHdrp())->rlNumCells++; //add one more cell       		
       			
		// keep the cell map for data chunks derived from artificial chunking. This is necessary, because we wont be
       		// able to created it in a possible future call of scanFileForPrefix (if this is again a large data chunk)
       		// since this would mean that we would have to change the chunk ids in the fact load file to include the extra domains.
       		// Also, in order to retrieve the actual values of the cells from the load file we will need it for artificially chunked
       		// data chunks.Therefore,       				
       		// create cost node for child with cell map
       		CostNode* costNodeChild = new CostNode(childHeaderp, childMapp);
       		
       		//free up memory, no more needed
                delete childHeaderp;
                childHeaderp = 0;
		delete childMapp;
		childMapp = 0;						
       		
       		//attach cost node to parent
       		const_cast<vector<CostNode*>&>(newCostRoot->getchild()).push_back(costNodeChild);
       		costNodeChild = 0; //no need to point at the CostNode any more
       		
       		//get next cell
       		try{
	       		currentCell.becomeNextCell();
	       	}
		catch(GeneralError& error){
			GeneralError e("AccessManagerImpl::EquiGrid_EquiChildren::operator() ==> ");
			error += e;
	      		delete newCostRoot; //the children will be also deleted
	      		delete newHierarchyVectp;
                 	throw error;
		}
		catch(...){
	      		delete newCostRoot; //the children will be also deleted
	      		delete newHierarchyVectp;
                 	throw;		
		}
       	}while(!currentCell.isFirstCell()); //continue while there are still more cells to visit
       	
       	#ifdef DEBUGGING
       	cerr << "Number of cells visited: " << numCellsVisited << endl;		
       	#endif
       	       		
       	//Update the size of the parent node, now that you have the real number of cells.
       	// ***NOTE*** As long as we dont compress dirchunks (current convention) this computation
       	// could also be done before computing the real number of cells, since we dont use it  anyway.
       	// However, in case this convention changes the calculation should take place here       		       	
       	try{
       		const_cast<ChunkHeader*>(newCostRoot->getchunkHdrp())->size = DirChunk::calculateStgSizeInBytes(newCostRoot->getchunkHdrp()->depth,
        							  maxDepth,
        							  newCostRoot->getchunkHdrp()->numDim,
        							  newCostRoot->getchunkHdrp()->totNumCells,
        							  newCostRoot->getchunkHdrp()->localDepth);
       	}
       	catch(GeneralError& error){
       		GeneralError e("AccessManagerImpl::EquiGrid_EquiChildren::operator() ==> ");
       		error += e;
      		delete newCostRoot; //the children will be also deleted
      		delete newHierarchyVectp;       		
                throw error;
       	}
       	catch(...){
      		delete newCostRoot; //the children will be also deleted
      		delete newHierarchyVectp;       		
                throw;       	
       	}							       		       		       		       		
       	

/*       	update the look-up table which records data chunk cells (i.e., their chunk-ids) that have been moved to a
   	larger depth due to artificial chunking
   	IF such a table does not already exist
   		call appropriate method to create it
   	For each cell of the original large chunk (empty cells included)
   		create new chunk id by inserting the new domain
		insert into B+tree, old chunk id of parent cell as a key and the new one as the associated value  	
*/   	
        //Instead of using a look-up table, the parent-child relationship of the new members and the grain level order codes
        //will be stored in the chunk header of the parent node and then finally in an appropriate form will be stored along the
        // corresponding DiskDirChunk
        const_cast<ChunkHeader*>(newCostRoot->getchunkHdrp())->artificialHierarchyp = newHierarchyVectp;
	newHierarchyVectp = 0; //no need to point at the hierarchy anymore
	
   	//call putChunkIntoBuckets for the new cost-tree
	try {
       		accmgr->putChunksIntoBuckets(
			cbinfo,newCostRoot,where2store,factFile,dirChunksRootDirVectp,returnDirEntry,constructionParams );       			
       	}
       	catch(GeneralError& error) {
       		GeneralError e("AccessManagerImpl::EquiGrid_EquiChildren::operator() ==> ");
     		error += e;
      		delete newCostRoot; //the children will be also deleted     		
       		throw error;
       	}
       	catch(...){
      		delete newCostRoot; //the children will be also deleted     		
       		throw;       	
       	}   	
       	
   	// NOTE: From the arguments updated by the previous call, will be returned the vector with root bucket's chunks,
   	// containing the new root that we have created and the
   	// directory entry pointing at this root.
   	
   	//delete costRoot; //free up space, discard input chunk.

       	// NOTE:  We CANNOT delete costRoot here, in order to free up memory. This deletion will be done by the
       	// caller (not necessarily the immediate caller but constructCUBE_File). This will
       	// delete all cost nodes corresponding to chunks of the root directory. If we deleted in this place,
       	// then a dangling pointer would appear in the parent node, in the vector of children pointers.
       	//        ^^^^^^^^^^^^^^^^

       	// On the contrary (see previous comment), we must delete the newly created node here.
   	// NOTE: this deletion might propagate down further to children nodes at arbitrary depth (i.e.,local depth)
   	// but these (the children) must already have been deleted by now and the corresponding entries removed from
   	// the vector of children pointers!
   	#ifdef DEBUGGING
   	// ASSERTION: indeed no children entries left
   	if(!newCostRoot->getchild().empty()){  	
      		delete newCostRoot; //the children will be also deleted   	
   		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::EquiGrid_EquiChildren::operator() ==> Before freeing cost node, not empty vector of children pointers found!");
   	}
   	#endif   	   	
   	delete newCostRoot;
   	newCostRoot = 0;
   	
}//AccessManagerImpl::EquiGrid_EquiChildren::operator()

void AccessManagerImpl::EquiGrid_EquiChildren::createNewHierarchies(
			const vector<LevelRange>& oldRangeVect, //input
			vector<map<int, LevelRange> >& newHierarchyVect //output
			)
//precondition:
//	The EquiGrid_EquiChildren() has calculated the number of artificial partitions (i.e., number of members of new level - equal for all dims))
//	along each dimension, stored in the EquiGrid_EquiChildren::noMembersNewLevel data member and the maximum number of children under each new member,
//      equal for all members of a dimension (equi-children), stored in the EquiGrid_EquiChildren::maxNoChildrenPerDim data member.
//      (NOTE: The latter is only needed for discriminating between pseudo and non-pseudo levels.)
//	The oldRangeVect contains the ranges of the original large (data) chunk. The output parameter
//      newHierarchyVect will store the result hierarchy. When the routine is called it is assumed that the newHierarchyVect has been
//      initialized like this: vector<map<int, LevelRange> > newHierarchyVect(noDims), where nodims is the number of dimensions of the cube.
//processing:
//	compute the parent child relationships between the newly inserted members and the grain-level members for each dimension (oldRangeVect);
//      then,store them in a vector of maps (each map corresponds to dimension). NOTE: if for a dimension the new level inserted is a
//	pseudo level, then in the map we store the original range (i.e., no chunking takes place along this dimension) associated with
//	the PSEUDO_CODE constant. The method used for uniformly distibuting the children among the parents is: iterate parents in a
//      cyclic way and each time assign a child to a parent, until no more children are left.
//postcondition:
//	the vector of maps contains for each dimension, the association of each parent order code to a range of grain level members
{
       	//for each dimension
       	for(int dimi = 0; dimi < noDims; dimi++) {
       		//if this is not a pseudo level in the new chunk
       		if(maxNoChildrenPerDim[dimi] != 0){
       			//init vector which counts the number of children of each parent along this dimension
       			vector<unsigned int> childrenCounterPerParent(noMembersNewLevel, 0);
       			
       			//the total number of children is
       			int totalChildren = oldRangeVect[dimi].rightEnd - oldRangeVect[dimi].leftEnd + 1;
       		
       		    	//loop
       		    	int childrenLeft = totalChildren;
       		    	int parent_index = 0;
       		    	do{
       		    		//assign one child to each parent
       		    		childrenCounterPerParent[parent_index]++;
       		    		//one more child has been assigned
       		    		childrenLeft--;
       		    		//cycle to the next parent
       		    		parent_index = (parent_index + 1) % noMembersNewLevel;
       		    	}while(childrenLeft); //while there are still children to assign
       		    	
       		    	//assert that the sum of all counters equals the total number of children
       		    	if(accumulate(childrenCounterPerParent.begin(), childrenCounterPerParent.end(), 0) != totalChildren)
       		    		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::EquiGrid_EquiChildren::createNewHierarchies ==>ASSERTION: wrong total of children");
       		    	
       		    	//start the actual assignement of ranges to parents
       		    	//init left end: 	left = 1st order code at grain level for this dimension
       		    	int left = oldRangeVect[dimi].leftEnd;
       		        //for each parent

               		for(int parent_code = Chunk::MIN_ORDER_CODE;
               		    parent_code <= int(noMembersNewLevel -1 + Chunk::MIN_ORDER_CODE); //***NOTE*** casting to int necessary if
               		                                                                // parent_code is initialized with a negative value!!!
               		    parent_code++) {

               			// create appropriate grain level range corresponding to its children
               			LevelRange grainrange;       			
                		// left end assignment
                		grainrange.leftEnd = left;
                		// right-end assignment = left + number of children assigned to this parent - 1;
                		grainrange.rightEnd = grainrange.leftEnd + childrenCounterPerParent[parent_code - Chunk::MIN_ORDER_CODE] - 1;
                		//Associate range to parent code
                		(newHierarchyVect[dimi])[parent_code] = grainrange;
                		//move left to the next free child
                		left = grainrange.rightEnd + 1;                		
			}//end for       		        	
        	}//end if
        	else { //this dimension corresponds to a pseudo level in the new chunk
       			// create appropriate grain level range covering ALL children (since no chunking takes place along this dim)
       			LevelRange grainrange;       			
        		// left end assignment
        		grainrange.leftEnd = oldRangeVect[dimi].leftEnd;
        		// right end assignment
        		grainrange.rightEnd = oldRangeVect[dimi].rightEnd;
        		//Associate range to parent code (pseudo code in this case)
        		(newHierarchyVect[dimi])[LevelMember::PSEUDO_CODE] = grainrange;        	
        	}//end else	
	} //end for
}// end AccessManagerImpl::EquiGrid_EquiChildren::createNewHierarchies

/*
void AccessManagerImpl::constructCubeFile(CubeInfo& cinfo, string& factFile)
//precondition:
//	cinfo contains the following valid information:
//		- maxDepth, numFacts, num_of_dimensions, vectDim
//	that will be used in this procedure.
{
	// 1. Estimate the storage cost for the components of the chunk hierarchy tree

	// In this phase we will use only chunk headers.
	// 1.1 construct root chunk header.
	ChunkHeader* rootHdrp = new ChunkHeader;
	Chunk::createRootChunkHeader(rootHdrp, cinfo);

	// 1.2 create CostNode Tree
	CostNode* costRoot = 0;
	try {
		costRoot = Chunk::createCostTree(rootHdrp, cinfo, factFile);
	}
	catch(GeneralError& error) {
		//GeneralError e("");//"Ex. from Chunk::createCostTree in AccessManagerImpl::constructCubeFile : ");
		//error += e;
		throw error; //error.getErrorMessage().c_str();
	}
	
        #ifdef DEBUGGING
        	try{
        	      test_construction_phaseI(costRoot, cinfo );
        	      delete costRoot;
        	      return;
        	}
        	catch(GeneralError& error) {
        		GeneralError e("");//"Ex. from AccessManagerImpl::test_construction_phaseI in AccessManagerImpl::constructCubeFile : ");
        		error += e;
        		throw error;
        	}
        #endif	
	
	// 2. General target: Use CostNode tree to allocate in-memory chunks, load them from Factfile,
	//    attach them to in-memory buckets, store buckets on disk.
	
	// 2.1. Create a BucketID for the root Bucket.
	//      NOTE: no bucket allocation performed yet, just id generation!
	rc_t err;
	serial_t record_ID;
        err = ss_m::create_id(SystemManager::getDevVolInfo()->volumeID , 1, record_ID);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"AccessManagerImpl::ConstructCubeFile  ==> Error in ss_m::create_id"<< err <<endl;
		// throw an exeption
		throw error;
	}
	BucketID rootBcktID(record_ID);
	// update CubeInfo root Bucket ID member.
	// NOTE: the root chunk offset (i.e.index) within this bucket is a constant (see class CubeInfo)
	cinfo.set_rootBucketID(rootBcktID);	
	
	// 2.2. Start the basic chunk-packing into buckets algorithm.
	
	// Creating the vector holder for the entries of the root Bucket.		
	// This will be filled by putIntoBuckets
	vector<DirChunk>* rtBcktEntriesVectp = new vector<DirChunk>;
	rtBcktEntriesVectp->reserve(1); // reserve one position for storing the root chunk	
	DirEntry rootEntry; //will be updated by putIntoBuckets
	//unsigned int lastIndxInRootBck = 0; // will be updated by putIntoBuckets
	try {
		putIntoBuckets(cinfo,
			   costRoot,
			   cinfo.get_rootChnkIndex(),
			   factFile,
			   rtBcktEntriesVectp,
			   rootEntry);
			   //currIndxInRootBck);
	}
	catch(GeneralError& error) {
		GeneralError e("AccessManagerImpl::ConstructCubeFile  ==> ");
		error += e;
		throw error;
	}
	// assertion following:
	if((rootEntry.bcktId != cinfo.get_rootBucketID()) || (rootEntry.chnkIndex != cinfo.get_rootChnkIndex())) {
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::ConstructCubeFile  ==> Error in returned DirEntry value from AccessManagerImpl::putIntoBuckets");
	}
	
	// 2.3. Now, we are ready to create and store the root bucket.
	
	// ****ALSO CHECK THE CASE OF OVERFLOW CHAIN FOR THE ROOT BUCKET****
	
	// 2.3.1. First create the root bucket header
	size_t hdrSz = sizeof(BucketHeader); // Size (bytes) of the root-bucket's header.
	
	size_t dirSz = 0; // Calculate size (bytes) of the Dirchunk vector of the root bucket
	for (	vector<DirChunk>::const_iterator iter = rtBcktEntriesVectp->begin();
		iter != rtBcktEntriesVectp->end();
		++iter	){
		
		// use the chunk size info stored inside the chunk header.		
		dirSz += (*iter).gethdr().size;		
	}
		
	size_t dataSz = 0; //size of the DataChunk vector which is empty for the root bucket
	size_t bodySz = dirSz + dataSz;
	
	// create a byte vector containing the DirChunk vector of the root bucket
	vec_t body_data(rtBcktEntriesVectp, sizeof(*rtBcktEntriesVectp));
	size_t datachunks_offset_in_body = body_data.size(); // i.e. if char* p = record.body(),
							     // then (p + offset) point to the data chunk section
	size_t recordBdySz = datachunks_offset_in_body + 0; //no byte vector for DataChunks
		
	BucketHeader* rtBcktHdrp = new BucketHeader(rootBcktID, hdrSz, dirSz, dataSz, bodySz, datachunks_offset_in_body, recordBdySz);
	
	//2.3.2. Now, create the bucket instance
	Bucket rootBucket(rtBcktHdrp, rtBcktEntriesVectp, 0);	
	try {
		rootBucket.checkSize();		
	}
	catch(ExceptionBcktOverflow& e1) {
		string m("AccessManagerImpl::ConstructCubeFile  ==> ");//"Ex. from Bucket constructor in AccessManagerImpl::constructCubeFile : ");
		m += e1.msg;
		throw error;
	}
	catch(ExceptionBcktHdOverflow& e2) {
		string m("AccessManagerImpl::ConstructCubeFile  ==> ");//"Ex. from Bucket constructor in AccessManagerImpl::constructCubeFile : ");
		m += e2.msg;
		throw error;
	}
	catch(ExceptionBcktUnderflow& e3) {
		// ignore this case for the root bucket.
		;
	}
		
	// 2.3.3. Store the root bucket
	// prepare header and body, byte vectors
	vec_t hdr(rootBucket.gethdrp(), sizeof(*rootBucket.gethdrp()));
	// dont forget the DataChunk vector! In the the root bucket case it is empty.
	//vec_t datachunk_data(rootBucket.getdatap(),sizeof(*rootBucket.getdatap()));
	//body_data.put(datachunk_data, datachunks_offset_in_body, datachunk_data.size());
	
	// create record with the previously generated id
        err = ss_m::create_rec_id(SystemManager::getDevVolInfo()->volumeID , cinfo.get_fid().get_shoreID(),
                         hdr,
                         PAGESIZE,
                         body_data,
                         record_ID);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"AccessManagerImpl::ConstructCubeFile  ==> Error in ss_m::create_rec_id"<< err <<endl;
		// throw an exeption
		throw error;
	}
	
	delete costRoot; // free up the whole tree space!
}//AccessManagerImpl::constructCubeFile
*/
/*
void AccessManagerImpl::putIntoBuckets(CubeInfo& cbinfo,
				CostNode* costRoot,
				unsigned int where2store,
				string& factFile,
				vector<DirChunk>* dirChunksRootDirVectp,
				DirEntry& returnDirEntry)

//precondition:
//	(costRoot points at a CostNode corresponding to a directory chunk R that will be stored
//	in the root bucket, while the subtrees hanging from there will be stored
//	to different buckets.)  ||
//	(costRoot points at a data chunk that will be stored in a chain of overflow buckets)
//postcondition:	
//	in the 1st case: the subtrees are stored in buckets and in dirChunksRootDirVectp has been
//	inserted an instance of the dirchunk R. Also the returnDirEntry contains the bucket id
//	of the root bucket and the chunk slot of the dirchunk R.
//	in the 2nd case: the data chunk is stored in a bucket and the dirChunksRootDirVectp and returnDirEntry
//	remain unchanged.
				
{
	// (I) if this is NOT a leaf chunk   	
	if(costRoot->getchunkHdrp()->depth < cbinfo.getmaxDepth()){
		// ASSERTION1: assert that the size of the tree under costRoot is > DiskBucket::bodysize
		unsigned int szBytes = 0;
		CostNode::calcTreeSize(costRoot, szBytes);
		if(szBytes <= DiskBucket::bodysize)
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putIntoBuckets ==> ASSERTION1 failed\n");       							

		// 1. define a vector to hold the total cost of each sub-tree hanging from the chunk
		vector<unsigned int> costVect(costRoot->getchild().size());
	  	// 2. init costVect	  	
	  	int i = 0;
		for(vector<CostNode*>::const_iterator iter = costRoot->getchild().begin();
		   iter != costRoot->getchild().end(); ++iter) {
			CostNode::calcTreeSize((*iter), szBytes);
			costVect[i] = szBytes;
			i++;			
		}//end for
				
		// 3. The costRoot represents the current chunk. We want to store this chunk in the dirChunksRootDirVectp.In order
		//    to do that we need to create a DirChunk and fill appropriatelly its entries, then insert it in
	     	//    the vector at position where2store.
	     	
	     	// 3.1. Create the vector holding the chunk's entries.
	     	//      NOTE: the default DirEntry constructor should initialize the member BucketID
	     	//	with a BucketID::null constant.
	     	vector<DirEntry> entryVect(costRoot->getchunkHdrp()->totNumCells);
	     			
		// 4. We have 3 cases (A,B,C) that we have to deal with, arising from the contents of costVect
		//	case A: T<=costVect[i]<=B
		//	case B: costVect[i] < T
		//	case C: costVect[i] > B
		//	case D: EMPTY cell		
		//    NOTE: the "empty" case is handled by the default DirEntry constructor.
		vector<CaseStruct> caseAvect, caseBvect, caseCvect; // one vector for each case
		
		// 4.1. Init caseAvect, caseBvect, caseCvect
		vector<unsigned int>::const_iterator icost = costVect.begin();
		vector<ChunkID>::const_iterator icid = costRoot->getcMapp()->begin();
		while(icost != costVect.end() && icid != costRoot->getcMapp()->end()){
			if((*icost) >= DiskBucket::BCKT_THRESHOLD && (*icost) <= DiskBucket::bodysize){
				CaseStruct h = {(*icid),(*icost)};
				caseAvect.push_back(h);
			}
			else if((*icost) < DiskBucket::BCKT_THRESHOLD) {
				CaseStruct h = {(*icid),(*icost)};
				caseBvect.push_back(h);			
			}
			else {//(*icost) > DiskBucket::bodysize)
				assert((*icost) > DiskBucket::bodysize)
				CaseStruct h = {(*icid),(*icost)};
				caseCvect.push_back(h);						
			}			
			++icost;
			++icid;		
		}// end while
		
		// 5. Process each list separately:		
		// 5.1 case A
		for(vector<CaseStruct>::const_iterator i = caseAvect.begin(); i!=caseAvect.end();++i;) {
			DirEntry e;
			try{
				storeTreeInBucket(cinfo, costRoot->getchildById((*i).id), factFile, e);
			}
                       	catch(GeneralError& error) {
                       		GeneralError e("");
                       		error += e;
                       		throw error;
                       	}
                       	//ASSERTION2: the returned DirEntry is valid
                       	if(e.bcktId.isnull() || e.chnkIndex != 0)
                       		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putIntoBuckets ==> ASSERTION2: invalid returned dirchunk entry\n");
                       		
			// insert entry at entryVect in the right offset (calculated from the Chunk Id)			
			Coordinates c;
			(*i).id.extractCoords(c);
			//ASSERTION3: check offset calculation
			unsigned int offs = DirChunk::calcCellOffset(c);
       			if(offs >= entryVect.size()){
       				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putIntoBuckets ==> ASSERTION3: (Case A) entryVect out of range!\n");
       			}			
			entryVect[offs] = e;							
		} // end for
		
		// 5.2 case B : do your clustering thing! ;-)
		// this should return a vector of
		// DirEntries with 1-1 correspondence with caseBVect. I use this to store
	 	// in the appropriate positions in entryVect.
		vector<DirEntry> ev;
		ev.reserve(caseBvect.size());
		try{
			findBestClusters(cinfo, caseBvect, costRoot, factFile, ev);
		}
               	catch(GeneralError& error) {
               		GeneralError e("");
               		error += e;
               		throw error;
               	}
               	//ASSERTION4:valid returned DirEntries vector
               	if(ev.size() != caseBvect.size())
               		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putIntoBuckets ==> ASSERTION4: wrong size of returned vector\n");
		//update entryVect
		vector<CaseStruct>::const_iterator caseb_iter = caseBvect.begin();
		vector<DirEntry>::const_iterator bent_iter = ev.begin();
		while(caseb_iter != caseBvect.end() && bent_iter != ev.end()) {
			//ASSERTION5: valid bucket id in Direntries
			if((*bent_iter).bcktId.isnull())
				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putIntoBuckets ==> ASSERTION5: null bucket id\n");				
			// insert entry at entryVect in the right offset (calculated from the Chunk Id)			
			Coordinates c;
			(*caseb_iter).id.extractCoords(c);
			//ASSERTION6: check offset calculation
			unsigned int offs = DirChunk::calcCellOffset(c);
       			if(offs >= entryVect.size()){
       				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putIntoBuckets ==> ASSERTION6: (Case B) entryVect out of range!\n");
       			}			
			entryVect[offs] = (*bent_iter);											
					
			caseb_iter++;
			bent_iter++;		
		}//end while		
						
		// 5.3 case C
		for(vector<CaseStruct>::const_iterator i = caseCvect.begin(); i!=caseCvect.end();++i;) {
                	DirEntry entry; //will be updated by putIntoBuckets
               		rtBcktEntriesVectp->reserve(1); // reserve one position for storing this child chunk in the root bucket	
                	//unsigned int lastIndxInRootBck = 0; // will be updated by putIntoBuckets
                	try {
                       		putIntoBuckets(cinfo,
                       			   costRoot->getchildById((*i).id),
                       			   dirChunksRootDirVectp->capacity(), //where2store in the root bucket vector: at the end
                       			   factFile,
                       			   rtBcktEntriesVectp,
                       			   entry);
                       			   //currIndxInRootBck);
                       	}
                       	catch(GeneralError& error) {
                       		GeneralError e("");
                       		error += e;
                       		throw error;
                       	}
                       	//ASSERTION7 & 8 & 9: the returned DirEntry is valid
                       	// invariant1: the chunk that was passed as input to putIntoBuckets() will be stored
                       	//	      in the root bucket at a chunk slot, one greater than its parent chunk.
                       	// OR invariant2: the chunk that was passed as input to putIntoBuckets() is a data chunk
                       	//                and has been stored in a different bucket than the root bucket.
                       	if(costRoot->getchildById(*i).getchunkHdrp()->depth < cbinfo.getmaxDepth()){
                               	if(e.bcktId != cinfo.get_rootBucketID())
                               		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putIntoBuckets ==> ASSERTION7: invalid returned dirchunk entry\n");
        			if((e.chnkIndex - where2store) != 1)
        				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putIntoBuckets ==> ASSERTION8: invalid returned dirchunk entry\n");
			}//end if
			else{ //data chunk
                               	if(e.bcktId == cinfo.get_rootBucketID() || e.bcktId.isnull() || e.chnkIndex != 0)
        				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putIntoBuckets ==> ASSERTION9: invalid returned dirchunk entry\n");					
			}//end else
			                       		                       	
			// insert entry at entryVect in the right offset (calculated from the Chunk Id)			
			Coordinates c;
			(*i).id.extractCoords(c);
			//ASSERTION10: offset calculation
			unsigned int offs = DirChunk::calcCellOffset(c)
       			if(offs >= entryVect.size()){
       				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putIntoBuckets ==>ASSERTION10: (Case C) entryVect out of range!\n");
       			}			
			entryVect[offs] = entry;									
		}//end for
		
		// 6. Now that entryVect is filled, create the chunk
		// 6.1
		DirChunk newChunk((*costRoot->getchunkHdrp()), entryVect);
		
		// 6.2 insert newChunk in the dirChunksRootDirVectp at position where2store.
		dirChunksRootDirVectp->insert(dirChunksRootDirVectp->begin()+where2store, newChunk);
		
		// 7. update the DirEntry corresponding to the newChunk	and
		//    return it to the caller. Actually, the caller can find this
		//    information but this way is easier and cleaner.
		returnDirEntry.bcktId = cbinfo.get_rootBucketID();
		returnDirEntry.chnkIndex = where2store;
		
	} //end if
	else { // II. this is a leaf chunk
		//ASSERTION11
		if(costRoot->getchunkHdrp()->depth != cbinfo.getmaxDepth())
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::putIntoBuckets ==> ASSERTION11: Wrong depth in leaf chunk!\n");	
		
		// We (maybe) have to use bucket overflow chains to store this leaf chunk
		try{
			storeDataChunkIntoBucket(costRoot, cbinfo, factFile, returnDirEntry);
		}
               	catch(GeneralError& error) {
               		GeneralError e("AccessManagerImpl::putIntoBuckets ==> ");
               		error += e;
               		throw error;
               	}			
	}//end else	
} // end of AccessManagerImpl::putIntoBuckets
*/

/*
 * The following function object is defined in order to
 * use multimap container in AccessManagerImpl::findBestClusters
 */
/*class less_than_BucketID {
public:
	bool operator()(const BucketID& b1, const BucketID& b2) {
		return (b1.rid < b2.rid);
	}
};*/

void AccessManagerImpl::storeDataChunksInCUBE_FileClusters(
			const CubeInfo& cinfo,
			const vector<CaseStruct>& caseBvect,
			const CostNode* const costRoot,
			const string& factFile,
			vector<DirEntry>& resultVect,
        		const AccessManager::clustAlgToken_t clustering_algorithm) const
// precondition:
//	each entry caseBvect corresponds to a data chunk. All data chunks are hanging from the same parent
//	and have a size-cost less than the bucket threshold. costRoot is a pointer
//	to the common parent CostNode. The resultVect is empty but we have already reserved
//	space for it, because we know that its size is equal with the size of caseBvect.
//
// postcondition:
//	The data chunks have been stored in CUBE_File buckets, some of them in the same bucket,
//	forming a "cluster" (or "bucket region").
//	The resultVect is filled with DirEntries that correspond 1-1 to the entries in caseBVect.
//	Each DirEntry denotes the bucket id as well as the chunk slot that each data chunk resides.
{
        if(!costRoot)
                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeDataChunksInCUBE_FileClusters ==> null tree pointer\n");

        if(factFile.empty())
                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeDataChunksInCUBE_FileClusters ==> no load file name\n");

	//formulate the regions
	multimap<BucketID, ChunkID> bucketRegion;
	vector<BucketID> buckIDs;
	try{
		formulateBucketRegions(caseBvect, bucketRegion, buckIDs, clustering_algorithm);
	}
       	catch(GeneralError& error) {
       		GeneralError e("AccessManagerImpl::storeDataChunksInCUBE_FileClusters ==> "); //error_out<<msg<<endl;
       		error += e;
       		throw error;
       	}
       	
       	map<ChunkID, DirEntry> resultMap; // map, to associate each data chunk with its
       					  // corresponding DirEntry in its parent chunk.
       	
	//for each region...
	typedef multimap<BucketID, ChunkID>::const_iterator MMAP_ITER;
        unsigned int maxDepth = cinfo.getmaxDepth();
        unsigned int numFacts = cinfo.getnumFacts();
	// for each bucket id corresponding to a region
	for(vector<BucketID>::const_iterator buck_i = buckIDs.begin();
	    buck_i != buckIDs.end(); ++buck_i) {

		//find the range of "items" in a specific region corresponding to a specific BucketID
        	pair<MMAP_ITER,MMAP_ITER> c = bucketRegion.equal_range(*buck_i);
        	vector<CostNode*> dataChunksOfregion; // vector to hold the corresponding cost nodes pointers (data chunk pointers)
        	// iterate through all chunk ids of a region
        	for(MMAP_ITER iter = c.first; iter!=c.second; ++iter) {
			CostNode* childNodep = costRoot->getchildById(iter->second);
			//ASSERTION
			if(!childNodep)
				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeDataChunksInCUBE_FileClusters ==> ASSERTION: child node not found!");
        		 dataChunksOfregion.push_back(childNodep);
      		} //end for

		//create a DiskBucket instance in heap containing this region and update resultMap
	        DiskBucket* dbuckp = 0;
		try{
			createDataChunkRegionDiskBucketInHeap(	maxDepth,
								numFacts,
								dataChunksOfregion,
								*buck_i,
								factFile,
								dbuckp,
								resultMap);
		}
         	catch(GeneralError& error) {
         		GeneralError e("AccessManagerImpl::storeDataChunksInCUBE_FileClusters ==> ");
         		error += e;
         		if(dbuckp) delete dbuckp;
         		throw error;
         	}
         	catch(...){
         		if(dbuckp) delete dbuckp;
         		throw;         	
         	}

                #ifdef DEBUGGING
         	//print the contents of the created bucket in a separate file
                 try{
                         printDiskBucketContents_SingleTreeAndCluster(dbuckp, maxDepth);
                 }
               	catch(GeneralError& error) {
               		GeneralError e("AccessManagerImpl::storeDataChunksInCUBE_FileClusters ==> ");
               		error += e;
               		delete dbuckp;
               		throw error;
               	}
               	catch(...){
               		delete dbuckp;
               		throw;               	
               	}
                #endif

        	// Store the DiskBucket in a fixed size Bucket in the CUBE File
        	try {
        		FileManager::storeDiskBucketInCUBE_File(dbuckp, cinfo.get_fid());
        	}
         	catch(GeneralError& error) {
         		GeneralError e("AccessManagerImpl::storeDataChunksInCUBE_FileClusters ==> ");
         		error += e;
         		delete dbuckp;
         		throw error;
         	}
         	catch(...){
         		delete dbuckp;
         		throw;         	
         	}
         	
         	delete dbuckp; //free up memory
         	dbuckp = 0;
	}//end for

	//update result vector with the appropriate DirEntries
	// NOTE: resultVect has 1-1 correspondence with caseBvect
	for(vector<CaseStruct>::const_iterator i = caseBvect.begin(); i!=caseBvect.end(); ++i) {
		resultVect.push_back(resultMap[(*i).id]);
	} //end for
	
}//AccessManagerImpl::storeDataChunksInCUBE_FileClusters

void AccessManagerImpl::storeTreesInCUBE_FileClusters(
			const CubeInfo& cinfo,
			const vector<CaseStruct>& caseBvect,
			const CostNode* const costRoot,
			const string& factFile,
			vector<DirEntry>& resultVect,
			const AccessManager::treeTraversal_t howToTraverse,
        		const AccessManager::clustAlgToken_t clustering_algorithm) const
// precondition:
//	each entry caseBvect corresponds to a chunk-tree. All trees are hanging from the same parent
//	and have a size-cost less than the bucket threshold. costRoot is a pointer
//	to the common parent CostNode. The resultVect is empty but we have already reserved
//	space for it, because we know that its size is equal with the size of caseBvect.
//
// postcondition:
//	The trees have been stored in CUBE_File buckets, some of them in the same bucket, forming a "cluster" (or "bucket region").
//	The resultVect is filled with DirEntries that correspond 1-1 to the entries in caseBVect. Each DirEntry
//	denotes the bucket id as well as the chunk slot that each tree resides.
{
        if(!costRoot)
                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeTreesInCUBE_FileClusters ==> null tree pointer");

        if(factFile.empty())
                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeTreesInCUBE_FileClusters ==> no load file name");

	//formulate the regions
	multimap<BucketID, ChunkID> bucketRegion;
	vector<BucketID> buckIDs;
	try{
		formulateBucketRegions(caseBvect, bucketRegion, buckIDs, clustering_algorithm);
	}
       	catch(GeneralError& error) {
       		GeneralError e("AccessManagerImpl::storeTreesInCUBE_FileClusters ==> "); //error_out<<msg<<endl;
       		error += e;
       		throw error;
       	}

       	map<ChunkID, DirEntry> resultMap; // map, to associate each root of the trees with its
       					  // corresponding DirEntry in its parent chunk.

	//for each region...
	typedef multimap<BucketID, ChunkID>::const_iterator MMAP_ITER;
        unsigned int maxDepth = cinfo.getmaxDepth();
        unsigned int numFacts = cinfo.getnumFacts();
	// for each bucket id corresponding to a region
	for(vector<BucketID>::const_iterator buck_i = buckIDs.begin();
	    buck_i != buckIDs.end(); ++buck_i) {

		//find the range of "items" in a specific region corresponding to a specific BucketID
        	pair<MMAP_ITER,MMAP_ITER> c = bucketRegion.equal_range(*buck_i);
        	vector<CostNode*> treesOfregion; // vector to hold the corresponding cost nodes pointers (tree pointers)
        	// iterate through all chunk ids of a region
        	for(MMAP_ITER iter = c.first; iter!=c.second; ++iter) {
			CostNode* childNodep = costRoot->getchildById(iter->second);
			//ASSERTION
			if(!childNodep)
				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeTreesInCUBE_FileClusters ==> ASSERTION: child node not found!");
        		 treesOfregion.push_back(childNodep);
      		} //end for

		//create a DiskBucket instance in heap containing this region and update resultMap
	        DiskBucket* dbuckp = 0;
		try{
			createTreeRegionDiskBucketInHeap(maxDepth, numFacts, treesOfregion, *buck_i,
							factFile, dbuckp, resultMap, howToTraverse);
		}
         	catch(GeneralError& error) {
         		GeneralError e("AccessManagerImpl::storeTreesInCUBE_FileClusters ==> ");
         		error += e;
         		if(dbuckp) delete dbuckp;
         		throw error;
         	}
         	catch(...){
         		if(dbuckp) delete dbuckp;
         		throw;         	
         	}

                #ifdef DEBUGGING
         	//print the contents of the created bucket in a separate file
                 try{
                         printDiskBucketContents_SingleTreeAndCluster(dbuckp, maxDepth);
                 }
               	catch(GeneralError& error) {
               		GeneralError e("AccessManagerImpl::storeTreesInCUBE_FileClusters ==> ");
               		error += e;
               		delete dbuckp;
               		throw error;
               	}
               	catch(...){
               		delete dbuckp;
               		throw;               	
               	}
                #endif

        	// Store the DiskBucket in a fixed size Bucket in the CUBE File
        	try {
        		FileManager::storeDiskBucketInCUBE_File(dbuckp, cinfo.get_fid());
        	}
         	catch(GeneralError& error) {
         		GeneralError e("AccessManagerImpl::storeTreesInCUBE_FileClusters ==> ");
         		error += e;
         		delete dbuckp;
         		throw error;
         	}
         	catch(...){
         		delete dbuckp;
         		throw;         	
         	}
         	
         	delete dbuckp; //free up memory
         	dbuckp = 0;
	}//end for

	//update result vector with the appropriate DirEntries
	// NOTE: resultVect has 1-1 correspondence with caseBvect
	for(vector<CaseStruct>::const_iterator i = caseBvect.begin();
		i!=caseBvect.end(); ++i) {

		resultVect.push_back(resultMap[(*i).id]);
	} //end for
}// end of AccessManagerImpl::storeTreesInCUBE_FileClusters

void AccessManagerImpl::createDataChunkRegionDiskBucketInHeap(
			unsigned int maxDepth,
			unsigned int numFacts,
			const vector<CostNode*>& dataChunksOfregion,
			const BucketID& bcktID,
			const string& factFile,
			DiskBucket* &dbuckp,
			map<ChunkID, DirEntry>& resultMap)const
//precondition:
//	the cost-nodes in dataChunksOfregion belong to the same region (cluster) and each
//	has a size-cost less than the bucket threshold. dbuckp is a NULL pointer
//	and resultMap is empty.
//postcondition:
//	dbuckp points at a DiskBucket that contains the corresponding data chunks that have been
//	loaded from the input factFile and stored in a bucket as different subtrees.
//	The resultMap contains the DirEntries corresponding to the data chunks.
{
	//ASSERTION1: dbuckp points to NULL
	if(dbuckp)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::createDataChunkRegionDiskBucketInHeap ==> ASSERTION1: input pointer to DiskBucket should be null\n");
        //ASSERTION2: non empty input vector
        if(dataChunksOfregion.empty())
                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::createDataChunkRegionDiskBucketInHeap ==> ASSERTION2: input vector is empty\n");
        //ASSERTION3: NOT too many subtrees in a single cluster
        if(dataChunksOfregion.size() > DiskBucketHeader::subtreemaxno)
                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::createDataChunkRegionDiskBucketInHeap ==> ASSERTION3: too many subtrees to store in a cluster\n");

	// allocate DiskBucket in heap
	try{
		dbuckp = new DiskBucket;
	}
	catch(std::bad_alloc&){
		GeneralError error(__FILE__, __LINE__, "AccessManagerImpl::createDataChunkRegionDiskBucketInHeap ==> cant allocate space for new DiskBucket!\n");
		//log error to error log file
		errorLogStream << error << endl;
		//output to screen
	        outputLogStream << "(File: " << __FILE__ << ", line: " << __LINE__ << "): " << "AccessManagerImpl::createDataChunkRegionDiskBucketInHeap ==> cant allocate space for new DiskBucket!\n" << endl;		
	
		throw; // re-throw bad_alloc
	}	

	// initialize bucket directory pointer to point one beyond last byte of body
	dbuckp->offsetInBucket = reinterpret_cast<DiskBucketHeader::dirent_t*>(&(dbuckp->body[DiskBucket::bodysize]));

	// initialize the DiskBucketHeader
	dbuckp->hdr.id.rid = bcktID.rid; // store the bucket id
	// Init the links to other buckets with null ids
	dbuckp->hdr.next.rid = serial_t::null;
	dbuckp->hdr.previous.rid = serial_t::null;
	
	dbuckp->hdr.no_chunks = 0;  	//init chunk counter
	dbuckp->hdr.next_offset = 0; //next free byte offset in the body
	dbuckp->hdr.freespace = DiskBucket::bodysize;//init free space counter
	dbuckp->hdr.no_ovrfl_next = 0; // no overflow-bucket  chain used

	dbuckp->hdr.no_subtrees = 0; // init subtree counter
	char* nextFreeBytep = dbuckp->body; //init current byte pointer
	//for each cost-node of the region...
	for(vector<CostNode*>::const_iterator chunk_iter = dataChunksOfregion.begin();
					chunk_iter != dataChunksOfregion.end(); chunk_iter++) {

		//create the corresponding data chunk from the input file
        	//read the input file and create a DataChunk instance
        	vector<DataChunk>* dataVectp = new vector<DataChunk>;
                vector<DirChunk>* const dirVectp = 0; // no pointer to DirChunk vector passed in this case
        	dataVectp->reserve(1);
                try {
        		descendDepth1stCostTree( maxDepth,
                                                 numFacts,
                                                 *chunk_iter,
                                                 factFile,
                                                 bcktID,
                                                 dirVectp,
                                                 dataVectp);
                }
                catch(GeneralError& error) {
                       GeneralError e("AccessManagerImpl::createDataChunkRegionDiskBucketInHeap ==> ");
                       error += e;
                       delete dbuckp;
                       dbuckp = 0;
                       delete dataVectp;
                       throw error;
                }
                catch(...){
                       delete dbuckp;
                       dbuckp = 0;
                       delete dataVectp;
                       throw;
                }

              	//ASSERTION 3: valid returned vectors
               	if(dataVectp->size() != 1){
               		delete dbuckp;
               		dbuckp = 0;
               		delete dataVectp;
        		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::createDataChunkRegionDiskBucketInHeap ==> ASSERTION 3: error in creating the DataChunk instance from file\n");
        	}//if

		//update subtree counter in bucket header
		dbuckp->hdr.no_subtrees++; //one more tree will be added
		//update subtree-directory with the chunk-directory entry (i.e. chunk slot) corresponding
		//to this tree.
		dbuckp->hdr.subtree_dir_entry[dbuckp->hdr.no_subtrees-1] = dbuckp->hdr.no_chunks;
		
        	// place the data chunk in the bucket's body		
                try{
        	        placeSingleDataChunkInDiskBucketBody(maxDepth, numFacts,
        		                        dataVectp->front(), dbuckp, nextFreeBytep);
                }
               	catch(GeneralError& error) {
               		GeneralError e("AccessManagerImpl::createDataChunkRegionDiskBucketInHeap ==> ");
               		error += e;
               		delete dbuckp;
               		dbuckp = 0;
               		delete dataVectp;
               		throw error;
               	}
               	catch(...){
               		delete dbuckp;
               		dbuckp = 0;
               		delete dataVectp;
               		throw;               	
               	}
               	// free up memory        	
        	delete dataVectp;
		dataVectp = 0;
		
                //insert new entry in result map that holds chunk-id to DirEntry associations. The chunk id
                // corresponds to the data chunk.
		//ASSERTION 4: first assert that this is the first time we insert this chunk id
		ChunkID chnkid = (*chunk_iter)->getchunkHdrp()->id;
		if(resultMap.find(chnkid) != resultMap.end()){
			delete dbuckp;
			dbuckp = 0;
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::createDataChunkRegionDiskBucketInHeap ==>ASSERTION 4: Duplicate chunk id for data chunk in cluster\n");
		}//end if
		DirEntry dirent(bcktID, dbuckp->hdr.subtree_dir_entry[dbuckp->hdr.no_subtrees-1]); // create the DirEntry consisting of the bucket
												   // id and the chunk slot corresponding												   // to this data chunk
		resultMap[chnkid] = dirent;
	}//end for	
}//AccessManagerImpl::createDataChunkRegionDiskBucketInHeap	

void AccessManagerImpl::createTreeRegionDiskBucketInHeap(
		unsigned int maxDepth,
		unsigned int numFacts,
		const vector<CostNode*>& treesOfregion,
		const BucketID& bcktID,
		const string& factFile,
		DiskBucket* &dbuckp,
		map<ChunkID, DirEntry>& resultMap,
		const AccessManager::treeTraversal_t howToTraverse)const
			
//precondition:
//	the cost-trees in treesOfregion belong to the same region (cluster) and each
//	has a size-cost less than the bucket threshold. dbuckp is a NULL pointer
//	and resultMap is empty.
//postcondition:
//	dbuckp points at a DiskBucket that contains the corresponding chunk trees that have been
//	loaded from the input factFile. The resultMap contains the DirEntries corresponding to the trees.
{
	//ASSERTION1: dbuckp points to NULL
	if(dbuckp)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::createTreeRegionDiskBucketInHeap ==> ASSERTION1: input pointer to DiskBucket should be null\n");
        //ASSERTION2: non empty input vector
        if(treesOfregion.empty())
                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::createTreeRegionDiskBucketInHeap ==> ASSERTION2: input vector is empty\n");
        //ASSERTION3: NOT too many subtrees in a single cluster
        if(treesOfregion.size() > DiskBucketHeader::subtreemaxno)
                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::createTreeRegionDiskBucketInHeap ==> ASSERTION3: too many subtrees to store in a cluster\n");

	// allocate DiskBucket in heap
	try{
		dbuckp = new DiskBucket;
	}
	catch(std::bad_alloc&){
		GeneralError error(__FILE__, __LINE__, "AccessManagerImpl::createTreeRegionDiskBucketInHeap ==> cant allocate space for new DiskBucket!\n");		
		//log error to error log file
		errorLogStream << error << endl;
		//output to screen
	        outputLogStream << "(File: " << __FILE__ << ", line: " << __LINE__ << "): " << "AccessManagerImpl::createTreeRegionDiskBucketInHeap ==> cant allocate space for new DiskBucket!\n" << endl;		
	
		throw; // re-throw bad_alloc
	}	

	// initialize bucket directory pointer to point one beyond last byte of body
	dbuckp->offsetInBucket = reinterpret_cast<DiskBucketHeader::dirent_t*>(&(dbuckp->body[DiskBucket::bodysize]));

	// initialize the DiskBucketHeader
	dbuckp->hdr.id.rid = bcktID.rid; // store the bucket id
	// Init the links to other buckets with null ids
	dbuckp->hdr.next.rid = serial_t::null;
	dbuckp->hdr.previous.rid = serial_t::null;
	dbuckp->hdr.no_chunks = 0;  	//init chunk counter
	dbuckp->hdr.next_offset = 0; //next free byte offset in the body
	dbuckp->hdr.freespace = DiskBucket::bodysize;//init free space counter
	dbuckp->hdr.no_ovrfl_next = 0; // no overflow-bucket  chain used

	dbuckp->hdr.no_subtrees = 0; // init subtree counter
	char* nextFreeBytep = dbuckp->body; //init current byte pointer
	//for each cost-tree of the region...
	for(vector<CostNode*>::const_iterator tree_iter = treesOfregion.begin();
					tree_iter != treesOfregion.end(); tree_iter++) {

		//create the corresponding dir/data chunks from the input file
		//according to the traversal method
        	//  In order to create a DiskBucket we will need a vector of DirChunks, a vector of DataChunks
        	vector<DirChunk>* dirVectp = 0;
        	vector<DataChunk>* dataVectp = 0;
        	// traverse the cost-tree and fill these vectors with chunks, loaded from the fact input file
        	try{
                        traverseSingleCostTreeCreateChunkVectors(
                        				maxDepth,
							numFacts,
                        				*tree_iter,
                        				factFile,
                        				bcktID,
                        				dirVectp,
                        				dataVectp,
                        				howToTraverse);
        	}
               	catch(GeneralError& error) {
               		GeneralError e("AccessManagerImpl::createTreeRegionDiskBucketInHeap ==> ");
               		error += e;
               		delete dbuckp;
               		dbuckp = 0;
               		if(dirVectp) delete dirVectp;
               		if(dataVectp) delete dataVectp;
               		throw error;
               	}
		catch(...){
               		delete dbuckp;
               		dbuckp = 0;
               		if(dirVectp) delete dirVectp;
               		if(dataVectp) delete dataVectp;
               		throw;		
		}               	

		//update subtree counter in bucket header
		dbuckp->hdr.no_subtrees++; //one more tree will be added
		//update subtree-directory with the chunk-directory entry (i.e. chunk slot) corresponding
		//to this tree.
		dbuckp->hdr.subtree_dir_entry[dbuckp->hdr.no_subtrees-1] = dbuckp->hdr.no_chunks;

                //According to the desired traversal method store the chunks in the body of the bucket
                try{
        	        placeChunksOfSingleTreeInDiskBucketBody(maxDepth, numFacts,
        		                        dirVectp, dataVectp, dbuckp, nextFreeBytep,howToTraverse);
                }
               	catch(GeneralError& error) {
               		GeneralError e("AccessManagerImpl::createTreeRegionDiskBucketInHeap ==> ");
               		error += e;
               		delete dbuckp;
               		dbuckp = 0;
               		delete dirVectp;
               		delete dataVectp;               		               		
               		throw error;
               	}
               	catch(...){
               		delete dbuckp;
               		dbuckp = 0;
               		delete dirVectp;
               		delete dataVectp;               		               		
               		throw;               	
               	}
               	// free up memory
        	delete dirVectp;
        	dirVectp = 0;
        	delete dataVectp;
        	dataVectp = 0;

                //insert new entry in result map that holds chunk-id to DirEntry associations. The chunk id
                // corresponds to the root of the tree.
		//ASSERTION 4: first assert that this is the first time we insert this chunk id
		ChunkID chnkid = (*tree_iter)->getchunkHdrp()->id;
		if(resultMap.find(chnkid) != resultMap.end()){
               		delete dbuckp;
               		dbuckp = 0;		
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::createTreeRegionDiskBucketInHeap ==>ASSERTION 4: Duplicate chunk id for root of tree in cluster\n");
		}
		DirEntry dirent(bcktID, dbuckp->hdr.subtree_dir_entry[dbuckp->hdr.no_subtrees-1]); // create the DirEntry consisting of the bucket
												   // id and the chunk slot corresponding
												   // to this tree
		resultMap[chnkid] = dirent;

	}//end for
}//end of AccessManagerImpl::createTreeRegionDiskBucketInHeap

void AccessManagerImpl::printDiskBucketContents_SingleTreeAndCluster(
							DiskBucket* const dbuckp,
							unsigned int maxDepth)const
// precondition:
//	the dbuckp points at a DiskBucket allocated in heap, which contains a single tree or
//	a cluster of trees. The pointer members of DiskBucket DON NOT contain valid values, therefore
//	they ought to be updated prior to reading the contents of the body.
//postcondition:
//	a file with unique name (for each bucket) has been created with the contents of *dbuckp and
//	the *dbuckp pointer members have been updated to point at the right places in the bucket's body.
{
	// the following piece of codes creates a unique file name for
	// each invocation of this function, based on a static variable called "prefix"

        //static int prefix = 0; // prefix of the output file name
        //ostrstream file_name;
        //file_name<<"OutputDiskBucket_"<<prefix<<".dbug"<<ends;
        //prefix++; //increase for next call


        // Alternatively, we can create a unique file based on the bucket id
        ostrstream file_name;
        file_name<<"OutputDiskBucket_"<<dbuckp->hdr.id.rid<<".dbug"<<ends;

	ofstream out(file_name.str()); //create a file stream for output

        if (!out)
        {
		string s("AccessManagerImpl::printDiskBucketContents_SingleTreeAndCluster ==> ");	
		string msg = s + string("creating file") + string(file_name.str()) +  string(" failed\n");
		GeneralError error(msg);
		throw error;
        }

        out<<"****************************************************************"<<endl;
        out<<"* Contents of a DiskBucket created by                          *"<<endl;
        out<<"*         AccessManagerImpl::createXXXDiskBucketInHeap             *"<<endl;
        out<<"****************************************************************"<<endl;

        out<<"\nBUCKET HEADER"<<endl;
        out<<"-----------------"<<endl;
        out<<"id: "<<dbuckp->hdr.id.rid<<endl;
        out<<"previous: "<<dbuckp->hdr.previous.rid<<endl;
        out<<"next: "<<dbuckp->hdr.next.rid<<endl;
        out<<"no_chunks: "<<dbuckp->hdr.no_chunks<<endl;
        out<<"next_offset: "<<dbuckp->hdr.next_offset<<endl;
        out<<"freespace: "<<dbuckp->hdr.freespace<<endl;
        out<<"no_subtrees: "<<int(dbuckp->hdr.no_subtrees)<<endl;
        out<<"subtree directory entries:\n";
        for(int i=0; i<dbuckp->hdr.no_subtrees; i++)
                out<<"subtree_dir_entry["<<i<<"]: "<<dbuckp->hdr.subtree_dir_entry[i]<<endl;
        out<<"no_ovrfl_next: "<<int(dbuckp->hdr.no_ovrfl_next)<<endl;

        // for each subtree in the bucket
        for(int i=0; i<dbuckp->hdr.no_subtrees; i++){

                out<<"\n---- SUBTREE "<<i<<" ----\n";
                //get the corresponding chunk slot range
                unsigned int treeBegin = dbuckp->hdr.subtree_dir_entry[i];
                unsigned int treeEnd;
                if(i+1==dbuckp->hdr.no_subtrees) //if this is the last subtree
                        treeEnd  = dbuckp->hdr.no_chunks; //get slot one-passed the last chunk slot of the tree
                else
                        treeEnd  = dbuckp->hdr.subtree_dir_entry[i+1]; //get the next tree's 1st slot

                //for each chunk in the range of slots that correspond to this subtree
                for(int chnk_slot = treeBegin; chnk_slot < treeEnd; chnk_slot++) {
                        //access each chunk:
                        //get a byte pointer at the beginning of the chunk
                        char* beginChunkp = dbuckp->body + dbuckp->offsetInBucket[-chnk_slot-1];

                        //first read the chunk header in order to find whether it is a DiskDirChunk, or
                        //a DiskDataChunk
                        DiskChunkHeader* chnk_hdrp = reinterpret_cast<DiskChunkHeader*>(beginChunkp);
                        //if(chnk_hdrp->depth == maxDepth)//then this is a DiskDataChunk
                       	if(AccessManagerImpl::isDirChunk(chnk_hdrp->depth, chnk_hdrp->local_depth, chnk_hdrp->next_local_depth, maxDepth))
                                printDiskDataChunk(out, beginChunkp, maxDepth);
                        //else if (chnk_hdrp->depth < maxDepth && chnk_hdrp->depth > Chunk::MIN_DEPTH)//it is a DiskDirchunk
                        else if(AccessManagerImpl::isDataChunk(chnk_hdrp->depth, chnk_hdrp->local_depth, chnk_hdrp->next_local_depth, maxDepth))
                                printDiskDirChunk(out, beginChunkp, maxDepth);
                        else {// Invalid depth!
                                //if(chnk_hdrp->depth == Chunk::MIN_DEPTH)
                                //        throw GeneralError(__FILE__, __LINE__, "printDiskBucketContents_SingleTreeAndCluster ==> depth corresponding to root chunk!\n");
                                ostrstream msg_stream;
                                msg_stream<<"printDiskBucketContents_SingleTreeAndCluster ==> chunk depth at slot "<<chnk_slot<<
                                                " inside DiskBucket "<<dbuckp->hdr.id.rid<<" is invalid"<<endl;
                                throw GeneralError(__FILE__, __LINE__, msg_stream.str());
                        }//end else
                }//end for
        }//end for
}//AccessManagerImpl::printDiskBucketContents_SingleTreeAndCluster

void AccessManagerImpl::printDiskDirChunk(ofstream& out, char* const startp, unsigned int maxDepth)const
// precondition:
//   startp is a byte pointer that points at the beginning of the byte stream where a DiskDirChunk
//   has been stored.
// postcondition:
//   The pointer members of the DiskDirChunk are updated to point at the corresponding arrays.
//   The contents of the DiskDirChunk are printed.
{

        //get a pointer to the dir chunk
        DiskDirChunk* const chnkp = reinterpret_cast<DiskDirChunk*>(startp);

	//ASSERTION: this is a dir chunk
	if(!AccessManagerImpl::isDirChunk(chnkp->hdr.depth, chnkp->hdr.local_depth, chnkp->hdr.next_local_depth, maxDepth))
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::printDiskDirChunk ==> wrong chunk type!\n");			
			
	//Is this the root chunk?
	bool isRootChunk = AccessManagerImpl::isRootChunk(chnkp->hdr.depth, chnkp->hdr.local_depth, chnkp->hdr.next_local_depth, maxDepth);

	//Is this an artificially chunked dir chunk?
	bool isArtifChunk = AccessManagerImpl::isArtificialChunk(chnkp->hdr.local_depth);	

        //update pointer members
        updateDiskDirChunkPointerMembers(maxDepth, *chnkp);

        //print header
        //use chnkp->hdrp to print the content of the header
        out<<"**************************************"<<endl;
        out<<"Depth: "<<int(chnkp->hdr.depth)<<endl;
        out<<"No_dims: "<<int(chnkp->hdr.no_dims)<<endl;
        out<<"No_measures: "<<int(chnkp->hdr.no_measures)<<endl;
        out<<"No_entries: "<<chnkp->hdr.no_entries<<endl;

        //if this is not the root chunk
        if(!isRootChunk){
                //print chunk id
                for(int i=0; i<chnkp->hdr.depth; i++){
                        for(int j=0; j<chnkp->hdr.no_dims; j++){
                                out<<(chnkp->hdr.chunk_id)[i].ordercodes[j];
                                (j==int(chnkp->hdr.no_dims)-1) ? out<<"." : out<<"|";
                        }//end for
                }//end for
                out<<endl;
	}//end if
	else{
		out << "ROOT CHUNK" << endl;
	}

        //print the order code ranges per dimension level
        for(int i=0; i<chnkp->hdr.no_dims; i++)
                out<<"Dim "<<i<<" range: left = "<<(chnkp->hdr.oc_range)[i].left<<", right = "<<(chnkp->hdr.oc_range)[i].right<<endl;

	//if this is an artificially chunked dir chunk
	if(isArtifChunk){
		//print the range to order code mappings
		for(int i=0; i < chnkp->hdr.no_dims; i++){
			for(int j =0; j < chnkp->rng2oc[i].noMembers; j++){
				//watch for last iteration
				if(j == chnkp->rng2oc[i].noMembers - 1)
        				out << "{" << chnkp->rng2oc[i].rngElemp[j].rngLeftBoundary << ", "
        					<< (chnkp->hdr.oc_range)[i].right << "} ==> " << j <<endl;				
				else				
        				out << "{" << chnkp->rng2oc[i].rngElemp[j].rngLeftBoundary << ", "
        					<< chnkp->rng2oc[i].rngElemp[j+1].rngLeftBoundary - 1<< "} ==> " << j <<endl;
			}//for
		}//for
	}//end if

        //print the dir entries
        out<<"\nDiskDirChunk entries:\n";
        out<<"---------------------\n";
        for(int i=0; i<chnkp->hdr.no_entries; i++){
                out<<"Dir entry "<<i<<": ";
                out<<chnkp->entry[i].bucketid.rid<<", "<<chnkp->entry[i].chunk_slot<<endl;;
        }//end for
}//AccessManagerImpl::printDiskDirChunk

void AccessManagerImpl::printDiskDataChunk(ofstream& out, char* const startp, unsigned int maxDepth)const
// precondition:
//   startp is a byte pointer that points at the beginning of the byte stream where a DiskDataChunk
//   has been stored.
// postcondition:
//   The pointer members of the DiskDataChunk are updated to point at the corresponding arrays.
//   The contents of the DiskDataChunk are printed.
{
        //get a pointer to the data chunk
        DiskDataChunk* const chnkp = reinterpret_cast<DiskDataChunk*>(startp);

	//ASSERTION: this is a data chunk
	if(!AccessManagerImpl::isDataChunk(chnkp->hdr.depth, chnkp->hdr.local_depth, chnkp->hdr.next_local_depth, maxDepth))
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::printDiskDataChunk ==> wrong chunk type!\n");			

        //update pointer members
        updateDiskDataChunkPointerMembers(maxDepth, *chnkp);

        //print header
        //use chnkp->hdrp to print the content of the header
        out<<"**************************************"<<endl;
        out<<"Depth: "<<int(chnkp->hdr.depth)<<endl;
        out<<"No_dims: "<<int(chnkp->hdr.no_dims)<<endl;
        out<<"No_measures: "<<int(chnkp->hdr.no_measures)<<endl;
        out<<"No_entries: "<<chnkp->hdr.no_entries<<endl;
        //print chunk id
        for(int i=0; i<chnkp->hdr.depth; i++){
                for(int j=0; j<chnkp->hdr.no_dims; j++){
                        out<<(chnkp->hdr.chunk_id)[i].ordercodes[j];
                        (j==int(chnkp->hdr.no_dims)-1) ? out<<"." : out<<"|";
                }//end for
        }//end for
        out<<endl;
        //print the order code ranges per dimension level
        for(int i=0; i<chnkp->hdr.no_dims; i++)
                out<<"Dim "<<i<<" range: left = "<<(chnkp->hdr.oc_range)[i].left<<", right = "<<(chnkp->hdr.oc_range)[i].right<<endl;

        //print no of ace
        out<<"No of ace: "<<chnkp->no_ace<<endl;

        //print the bitmap
        out<<"\nBITMAP:\n\t";
        for(int b=0; b<chnkp->hdr.no_entries; b++){
                  (!chnkp->test_bit(b)) ? out<<"0" : out<<"1";
        }//end for

        //print the data entries
        out<<"\nDiskDataChunk entries:\n";
        out<<"---------------------\n";
        for(int i=0; i<chnkp->no_ace; i++){
                out<<"Data entry "<<i<<": ";
                for(int j=0; j<chnkp->hdr.no_measures; j++){
                        out<<chnkp->entry[i].measures[j]<<", ";
                }//end for
                out<<endl;
        }//end for
}//end AccessManagerImpl::printDiskDataChunk

void AccessManagerImpl::updateDiskDirChunkPointerMembers(unsigned int maxDepth, DiskDirChunk& chnk)const
//precondition:
//      chnk is a DiskDirChunk structure but it contains uninitialised pointer members
//postcondition:
// the following pointer members have been initialized to point at the corresponding arrays:
// chnk.hdr.chunk_id, chnk.hdr.chunk_id[i].ordercodes (0<=i<chnk.hdr.depth), chnk.hdr.oc_range,
// chnk.entry.
{
	//ASSERTION: this is a dir chunk
	if(!AccessManagerImpl::isDirChunk(chnk.hdr.depth, chnk.hdr.local_depth, chnk.hdr.next_local_depth, maxDepth))
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::updateDiskDirChunkPointerMembers ==> wrong chunk type!\n");			
			
	//Is this the root chunk?
	bool isRootChunk = AccessManagerImpl::isRootChunk(chnk.hdr.depth, chnk.hdr.local_depth, chnk.hdr.next_local_depth, maxDepth);

	//Is this an artificially chunked dir chunk?
	bool isArtifChunk = AccessManagerImpl::isArtificialChunk(chnk.hdr.local_depth);	


        //get  a byte pointer
        char* bytep = reinterpret_cast<char*>(&chnk);


        bytep += sizeof(DiskDirChunk); // move at the end of the static part

        //if this not the root chunk
        if(!isRootChunk){
                //update chunk_id pointer
                chnk.hdr.chunk_id = reinterpret_cast<DiskChunkHeader::Domain_t*>(bytep);

                //update each domain pointer

                //fist find the number of domains in the chunk id
                int noDomains;
                try{
                	noDomains = DiskChunkHeader::getNoOfDomainsFromDepth(int(chnk.hdr.depth), int(chnk.hdr.local_depth));
                }
        	catch(GeneralError& error) {
                	GeneralError e("AccessManagerImpl::updateDiskDirChunkPointerMembers ==> ");
                	error += e;
                	throw error;
                }

        //        if(chnk.hdr.local_depth == Chunk::NULL_DEPTH)
        //        	noDomains = chnk.hdr.depth - Chunk::MIN_DEPTH;        	
        //        else {// local_depth >= MIN_DEPTH
                	//ASSERTION 1
        //        	if(!(chnk.hdr.local_depth >= Chunk::MIN_DEPTH)
        //        		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::updateDiskDirChunkPointerMembers ==> ASSERTION 1: invalid local depth in chunk.\n");
        //        	noDomains = (chnk.hdr.depth - Chunk::MIN_DEPTH)+(chnk.hdr.local_depth - Chunk::MIN_DEPTH);		
        //        }//end else

                 //We need to initialize the "ordercodes" pointer inside each Domain_t:
                //place byte pointer at the 1st ordercode of the first Domain_t
        	bytep += sizeof(DiskChunkHeader::Domain_t) * noDomains;

                //for each Domain_t
                for(int i=0; i<noDomains; i++){
                        //place byte pointer at the 1st ordercode of the current Domain_t
                        (chnk.hdr.chunk_id)[i].ordercodes = reinterpret_cast<DiskChunkHeader::ordercode_t*>(bytep);
                        bytep += sizeof(DiskChunkHeader::ordercode_t) * chnk.hdr.no_dims;
                }//end for
        }//end if
        else {//else for the root chunk the chunk id points to null
               chnk.hdr.chunk_id = 0;
        }//else

        //update oc_range pointer
        // in the header to point at the first OrderCodeRng_t
        // the currp must already point  at the first OrderCodeRng_t
        chnk.hdr.oc_range = reinterpret_cast<DiskChunkHeader::OrderCodeRng_t*>(bytep);

        //if this is an artificially chunked dir. chunk
        if(isArtifChunk){
		//update rng2oc pointer
		bytep += sizeof(DiskChunkHeader::OrderCodeRng_t)*chnk.hdr.no_dims;
		chnk.rng2oc = reinterpret_cast<DiskDirChunk::Rng2oc_t*>(bytep);
		
                 //We need to initialize the "rngElemp" pointer inside each Rng2oc_t:
                //place byte pointer at the 1st Rng2ocElem_t of the first Rng2oc_t
        	bytep += sizeof(DiskDirChunk::Rng2oc_t) * chnk.hdr.no_dims;

                //for each Rng2oc_t
                for(int i=0; i<chnk.hdr.no_dims; i++){
                        //place byte pointer at the 1st Rng2ocElem_t of the current Rng2oc_t
                        (chnk.rng2oc)[i].rngElemp = reinterpret_cast<DiskDirChunk::Rng2ocElem_t*>(bytep);
                        bytep += sizeof(DiskDirChunk::Rng2ocElem_t) * (chnk.rng2oc)[i].noMembers;
                }//end for		
        }//end if
        else {  // Not an artificial dir chunk
        	//move current pointer to the first DirEntry_t
	        bytep += sizeof(DiskChunkHeader::OrderCodeRng_t)*chnk.hdr.no_dims;
        }//end else

        //update entry pointer ( the current pointer must already point at the 1st DirEntry_t
        chnk.entry = reinterpret_cast<DiskDirChunk::DirEntry_t*>(bytep);

}//AccessManagerImpl::updateDiskDirChunkPointerMembers


void AccessManagerImpl::updateDiskDataChunkPointerMembers(unsigned int maxDepth, DiskDataChunk& chnk)const
//precondition:
//      chnk is a DiskDataChunk structure but it contains uninitialised pointer members
//postcondition:
// the following pointer members have been initialized to point at the corresponding arrays:
// chnk.hdr.chunk_id, chnk.hdr.chunk_id[i].ordercodes (0<=i<chnk.hdr.depth), chnk.hdr.oc_range,
// chnk.bitmap, chnk.entry, chnk.entry[i].measures (0<= i <chnk.no_ace).
{
	//ASSERTION: this is a data chunk
	if(!AccessManagerImpl::isDataChunk(chnk.hdr.depth, chnk.hdr.local_depth, chnk.hdr.next_local_depth, maxDepth))
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::updateDiskDataChunkPointerMembers ==> wrong chunk type!\n");			

        //get  a byte pointer
        char* bytep = reinterpret_cast<char*>(&chnk);

        //update chunk_id pointer
        bytep += sizeof(DiskDataChunk); // move at the end of the static part
        chnk.hdr.chunk_id = reinterpret_cast<DiskChunkHeader::Domain_t*>(bytep);

        //update each domain pointer

        //fist find the number of domains in the chunk id
        int noDomains;
        try{
        	noDomains = DiskChunkHeader::getNoOfDomainsFromDepth(int(chnk.hdr.depth), int(chnk.hdr.local_depth));
        }
	catch(GeneralError& error) {
        	GeneralError e("AccessManagerImpl::updateDiskDataChunkPointerMembers ==> ");
        	error += e;
        	throw error;
        }

//        if(chnk.hdr.local_depth == Chunk::NULL_DEPTH)
//        	noDomains = chnk.hdr.depth - Chunk::MIN_DEPTH;        	
//        else {// local_depth >= MIN_DEPTH
        	//ASSERTION 1
//        	if(!(chnk.hdr.local_depth >= Chunk::MIN_DEPTH)
//        		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::updateDiskDataChunkPointerMembers ==> ASSERTION 1: invalid local depth in chunk.\n");
//        	noDomains = (chnk.hdr.depth - Chunk::MIN_DEPTH)+(chnk.hdr.local_depth - Chunk::MIN_DEPTH);		
//        }//end else

         //We need to initialize the "ordercodes" pointer inside each Domain_t:
        //place byte pointer at the 1st ordercode of the first Domain_t
	bytep += sizeof(DiskChunkHeader::Domain_t) * noDomains;

        //for each Domain_t
        for(int i=0; i<noDomains; i++){
                //place byte pointer at the 1st ordercode of the current Domain_t
                (chnk.hdr.chunk_id)[i].ordercodes = reinterpret_cast<DiskChunkHeader::ordercode_t*>(bytep);
                bytep += sizeof(DiskChunkHeader::ordercode_t) * chnk.hdr.no_dims;
        }//end for

        //update oc_range pointer
        // in the header to point at the first OrderCodeRng_t
        // the currp must already point  at the first OrderCodeRng_t
        chnk.hdr.oc_range = reinterpret_cast<DiskChunkHeader::OrderCodeRng_t*>(bytep);

        //update bitmap pointer
        bytep += sizeof(DiskChunkHeader::OrderCodeRng_t)*chnk.hdr.no_dims;
        chnk.bitmap = reinterpret_cast<bmp::WORD*>(bytep);

        //update entry pointer
        bytep += sizeof(bmp::WORD)* bmp::numOfWords(chnk.hdr.no_entries); // move to the 1st data entry
        chnk.entry = reinterpret_cast<DiskDataChunk::DataEntry_t*>(bytep);

        //move byte pointer at the first measure value
        bytep += sizeof(DiskDataChunk::DataEntry_t)*chnk.no_ace;
        //for each data entry update measures pointer
        for(int e=0; e<chnk.no_ace; e++){
                //place measures pointer at the first measure
                chnk.entry[e].measures = reinterpret_cast<measure_t*>(bytep);
                bytep += sizeof(measure_t)*chnk.hdr.no_measures; //move on to next set of measures
        }//end for
}//AccessManagerImpl::updateDiskDataChunkPointerMembers

void AccessManagerImpl::formulateBucketRegions(
			const vector<CaseStruct>& caseBvect,
			multimap<BucketID, ChunkID>& resultRegions,
			vector<BucketID>& resultBucketIDs,
			const AccessManager::clustAlgToken_t clustering_algorithm)const
// precondition:
//	each entry of caseBvect corresponds to a chunk-tree. All trees are hanging from the same parent
//	and have a size-cost less than the bucket threshold. clustering_algorithm denotes the algorithm
//	with which the bucket regions (i.e. clusters) will be created.
//	
// postcondition:
//	resultBucketIDs contains the set of bucket ids that correspond to the buckets that will store
//	the formulated regions. Each region is stored in a single bucket. resultRegions contains the
// 	mapping of each bucket id to the chunk ids of the roots of the trees of the corresponding region.
{
	// Call the clustering algorithm to form the Bucket Regions
	//multimap<BucketID, ChunkID, less_than_BucketID> bucketRegion;
	switch(clustering_algorithm) {
		case AccessManager::simple:
			SimpleClusteringAlg algorithm; //init function object
			try {			
				algorithm(caseBvect, resultRegions, resultBucketIDs);
			}
                       	catch(GeneralError& error) {
                       		GeneralError e("AccessManagerImpl::formulateBucketRegions ==> ");
                       		error += e;
                       		throw error;
                       	}			
			break;
		default:
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::formulateBucketRegions ==> Unknown clustering algorithm\n");
			break;
	}// end switch
	
} // end of AccessManagerImpl::formulateBucketRegions

void AccessManagerImpl::SimpleClusteringAlg::operator() (
							const vector<CaseStruct>& caseBvect,
							multimap<BucketID,ChunkID>& resultRegions,
							vector<BucketID>& resultBucketIDs
				                    )
// precondition:
//	each entry of caseBvect corresponds to a chunk-tree. All trees are hanging from the same parent
//	and have a size-cost less than the bucket threshold.
// postcondition:
//	resultBucketIDs contains the set of bucket ids that correspond to the buckets that will store
//	the formulated regions. Each region is stored in a single bucket. resultRegions contains the
// 	mapping of each bucket id to the chunk ids of the roots of the trees of the corresponding region.
{
        //ASSERTION: no empty input
        if(caseBvect.empty())
                throw GeneralError(__FILE__, __LINE__, "SimpleClusteringAlg::operator() ==> empty input vector\n");

	size_t CLUSTER_SIZE_LIMIT = DiskBucket::bodysize; //a cluster's size must not exceed this limit

	//cost of current cluster
	size_t curr_clst_cost = 0;

        unsigned int noSubtreesInCurrClst = 0; //number of subtrees in current cluster

	//create a new bucket id
	BucketID curr_id;
	try{
		curr_id = BucketID::createNewID();
	}
       	catch(GeneralError& error) {
       		GeneralError e("AccessManagerImpl::SimpleClusteringAlg::operator() ==> ");
       		error += e;
       		throw error;
       	}

	//insert bucket id into bucket id output vector
	resultBucketIDs.push_back(curr_id);

	//loop from the  1st item to the last of the input vector
	for(vector<CaseStruct>::const_iterator iter = caseBvect.begin(); iter != caseBvect.end(); iter++) {
                //ASSERTION: cost must be below Threshold
                if(iter->cost >= DiskBucket::BCKT_THRESHOLD)
                        throw GeneralError(__FILE__, __LINE__, "SimpleClusteringAlg::operator() ==> cost of tree exceeds bucket threshold!\n");
		//add the cost of current item to the cost of current cluster
		//if the cost of current cluster is still below the limit
		if(curr_clst_cost + (*iter).cost <= CLUSTER_SIZE_LIMIT
                                        &&
                   noSubtreesInCurrClst+1 <= DiskBucketHeader::subtreemaxno) {
			//then insert current item in the current cluster
                        resultRegions.insert(make_pair(curr_id, iter->id));
			curr_clst_cost += (*iter).cost; // update cluster cost
                        noSubtreesInCurrClst++; //update cluster counter
		}
		else {
			//create a new bucket id
			try{
				curr_id = BucketID::createNewID();
			}
                	catch(GeneralError& error) {
                		GeneralError e("AccessManagerImpl::SimpleClusteringAlg::operator() ==> ");
                		error += e;
                		throw error;
                	}
			//insert bucket id into bucket id output vector
                	resultBucketIDs.push_back(curr_id);
                	// re-initialize cluster cost
                	curr_clst_cost = 0;
                        noSubtreesInCurrClst = 0; //reset subtree counter
			//then insert current item in the current cluster
			resultRegions.insert(make_pair(curr_id, iter->id));
			curr_clst_cost += (*iter).cost; // update cluster cost
		}// end else
	}//end for
}// end of AccessManagerImpl::SimpleClusteringAlg::operator()

void AccessManagerImpl::storeDataChunkInCUBE_FileBucket(
       				const CubeInfo& cinfo, //input
       				const CostNode* const costRoot, //input
       				const string& factFile,  //input
       				DirEntry& returnDirEntry  //output
				) const
//precondition:
//	costRoot points at a CostNode corresponding to a DataChunk.The total size of the chunk satisfies
//	the following: BUCKET_THRESHOLD <= size <= DiskBucket::bodysize. The cinfo must at least contain valid
//	info on the following:
//	ssm file id, the maximum chunking depth and the number of facts per cell.
//postcondition:
//	the DataChunk has been stored in a CUBE File and returnDirEntry contains the Bucket id and the chunk slot.
{
        unsigned int maxDepth = cinfo.getmaxDepth();
        unsigned int numFacts = cinfo.getnumFacts();

	//ASSERTION 1: this is a Data Chunk
       	//if(costRoot->getchunkHdrp()->depth != maxDepth)
	if(isDataChunk(costRoot->getchunkHdrp()->depth, costRoot->getchunkHdrp()->localDepth, costRoot->getchunkHdrp()->nextLocalDepth, maxDepth))       	
       		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeDataChunkInCUBE_FileBucket ==> ASSERTION 1: found tree instead of single data chunk in input\n");

	//ASSERTION 2: the cost is within desired limits
       	unsigned int szBytes = 0;
       	CostNode::calcTreeSize(costRoot, szBytes);
       	if(szBytes < DiskBucket::BCKT_THRESHOLD || szBytes > DiskBucket::bodysize)
       		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeDataChunkInCUBE_FileBucket ==> ASSERTION 2: wrong data chunk size\n");

        //  Create a BucketID for the  Bucket that will store the data chunk.
	//  NOTE: no bucket allocation performed yet, just id generation!
	//create a new bucket id
	BucketID bcktID;
	try{
		bcktID = BucketID::createNewID();
	}
       	catch(GeneralError& error) {
       		GeneralError e("AccessManagerImpl::storeDataChunkInCUBE_FileBucket ==> ");
       		error += e;
       		throw error;
       	}

	//read the input file and create a DataChunk instance
	vector<DataChunk>* dataVectp = new vector<DataChunk>;
        vector<DirChunk>* const dirVectp = 0; // no pointer to DirChunk vector passed in this case
	dataVectp->reserve(1);
        try {
		descendDepth1stCostTree( maxDepth,
                                         numFacts,
                                         costRoot,
                                         factFile,
                                         bcktID,
                                         dirVectp,
                                         dataVectp);
        }
        catch(GeneralError& error) {
               GeneralError e("AccessManagerImpl::storeDataChunkInCUBE_FileBucket ==> ");
               error += e;
               delete dataVectp;
               throw error;
        }
        catch(...){
               delete dataVectp;
               throw;
        }

      	//ASSERTION 3: valid returned vectors
       	if(dataVectp->size() != 1) {
       		delete dataVectp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeDataChunkInCUBE_FileBucket ==> ASSERTION 3: error in creating the DataChunk instance from file\n");
	}//end if


	//create a heap allocated DiskBucket instance containing only this chunk
        DiskBucket* dbuckp = 0;
        try {
		createSingleDataChunkDiskBucketInHeap(maxDepth,
					numFacts,
					bcktID,
					dataVectp->front(),//*(dataVectp->begin()),
					dbuckp);
        }
        catch(GeneralError& error) {
               GeneralError e("AccessManagerImpl::storeDataChunkInCUBE_FileBucket ==> ");
               error += e;
               delete dataVectp;
               throw error;
        }
        catch(...){
               delete dataVectp;
               throw;
        }
        //free up memory
        delete dataVectp;
        dataVectp = 0;

       #ifdef DEBUGGING
	//print the contents of the created bucket in a separate file
        try{
                printDiskBucketContents_SingleTreeAndCluster(dbuckp, maxDepth);
        }
      	catch(GeneralError& error) {
      		GeneralError e("AccessManagerImpl::storeTreesInCUBE_FileClusters ==> \n");
      		error += e;
      		throw error;
      	}
       #endif

	//  Store the DiskBucket in a fixed size Bucket in the CUBE File
	try {
		FileManager::storeDiskBucketInCUBE_File(dbuckp, cinfo.get_fid());
	}
      	catch(GeneralError& error) {
      		GeneralError e("AccessManagerImpl::storeSingleTreeInCUBE_FileBucket ==>");
      		error += e;
      		throw error;
      	}

       	// update the DirEntry corresponding to the cell pointing to costRoot and
       	// return it to the caller.
       	returnDirEntry.bcktId = bcktID;
	returnDirEntry.chnkIndex = 0; //root of tree is stored at the first chunk slot

 	// free up memory
 	delete dbuckp;	
}//end AccessManagerImpl::storeDataChunkInCUBE_FileBucket			

void AccessManagerImpl::storeSingleTreeInCUBE_FileBucket(
						const CubeInfo& cinfo, //input
						const CostNode* const costRoot, //input
						const string& factFile,  //input
						DirEntry& returnDirEntry,  //output
						const AccessManager::treeTraversal_t howToTraverse //input
						) const

// precondition:
// 	BUCKET_THRESHOLD <= total size of chunks of tree under costRoot <= DiskBucket::bodysize
// postcondition:
// 	(chunks of tree under costRoot stored in a ssm record (CUBE File bucket) within a
// 	DiskBucket structure)&&
// 	(returnDirEntry.bcktId.rid contains the ssm record id holding this tree)
// 	(returnDirentry.chnkIndex contains the chunk slot that the root chunk-node of this tree lies. 	
{
       	// ASSERTION1: assert that the size of the tree under costRoot is between [Threshold,DiskBucket::bodysize]
       	unsigned int szBytes = 0;
       	CostNode::calcTreeSize(costRoot, szBytes);
       	if(szBytes < DiskBucket::BCKT_THRESHOLD || szBytes > DiskBucket::bodysize)
       		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::storeSingleTreeInCUBE_FileBucket ==> ASSERTION1: wrong tree size\n");       							
	
	//  Create a BucketID for the  Bucket that will store the tree.
	//  NOTE: no bucket allocation performed yet, just id generation!
	//create a new bucket id
	BucketID bcktID;
	try{
		 bcktID = BucketID::createNewID();
	}
       	catch(GeneralError& error) {
       		GeneralError e("AccessManagerImpl::storeSingleTreeInCUBE_FileBucket ==> ");
       		error += e;
       		throw error;
       	}
	
	//create the corresponding dir/data chunks from the input file
	//according to the traversal method	
	//  In order to create a DiskBucket we will need a vector of DirChunks, a vector of DataChunks	
	vector<DirChunk>* dirVectp = 0;
	vector<DataChunk>* dataVectp = 0;
	// traverse the cost-tree and fill these vectors with chunks, loaded from the fact input file
        unsigned int maxDepth = cinfo.getmaxDepth();
        unsigned int numFacts = cinfo.getnumFacts();	
	try{	
                traverseSingleCostTreeCreateChunkVectors(
                       				maxDepth,
						numFacts,
                				costRoot,
                				factFile,
                				bcktID,
                				dirVectp,
                				dataVectp,
                				howToTraverse);
	}
       	catch(GeneralError& error) {
       		GeneralError e("AccessManagerImpl::storeSingleTreeInCUBE_FileBucket==>");
       		error += e;
       		if(dirVectp) delete dirVectp;
       		if(dataVectp) delete dataVectp;       		
       		throw error;
       	}
       	catch(...){
       		if(dirVectp) delete dirVectp;
       		if(dataVectp) delete dataVectp;       		
       		throw;       	
       	}
		
	// Create a DiskBucket in heap and fill it with the chunks of the 2 vectors.
        DiskBucket* dbuckp = 0;
        try{
                createSingleTreeDiskBucketInHeap(
                				maxDepth,
                				numFacts,
                				bcktID,
                				dirVectp,
                				dataVectp,
                				dbuckp,
                				howToTraverse);

        }
       	catch(GeneralError& error) {
       		GeneralError e("AccessManagerImpl::storeSingleTreeInCUBE_FileBucket==>");
       		error += e;
               	//free up memory
         	delete dirVectp;
         	delete dataVectp;
         	if(dbuckp) delete dbuckp;       	       		
       		throw error;
       	}
       	catch(...){
               	//free up memory
         	delete dirVectp;
         	delete dataVectp;
         	if(dbuckp) delete dbuckp;       	       		
       		throw;       	
       	}
       	
       	//free up memory
	delete dirVectp;
	dirVectp = 0;
	delete dataVectp;
	dataVectp = 0;       	

       #ifdef DEBUGGING
	//print the contents of the created bucket in a separate file
        try{
                printDiskBucketContents_SingleTreeAndCluster(dbuckp, maxDepth);
        }
      	catch(GeneralError& error) {
      		GeneralError e("AccessManagerImpl::storeTreesInCUBE_FileClusters ==> \n");
      		error += e;
      		delete dbuckp;
      		throw error;
      	}
      	catch(...){
      		delete dbuckp;
      		throw;      	
      	}
       #endif
       	       	      	      	      			
	//  Store the DiskBucket in a fixed size Bucket in the CUBE File
	try {
		FileManager::storeDiskBucketInCUBE_File(dbuckp, cinfo.get_fid());
	}
      	catch(GeneralError& error) {
      		GeneralError e("AccessManagerImpl::storeSingleTreeInCUBE_FileBucket ==>");
      		error += e;
      		delete dbuckp;
      		throw error;
      	}
      	catch(...){
      		delete dbuckp;
      		throw;      	
      	}
	 	
       	//   update the DirEntry corresponding to the cell pointing to costRoot and
       	//   return it to the caller.
       	returnDirEntry.bcktId = bcktID;
	returnDirEntry.chnkIndex = 0; //root of tree is stored at the first chunk slot

 	// free up memory
 	delete dbuckp;		
}//end of AccessManagerImpl::storeSingleTreeInCUBE_FileBucket

void AccessManagerImpl::traverseSingleCostTreeCreateChunkVectors(
				unsigned int maxDepth,
				unsigned int numFacts,
				const CostNode* const costRoot,
				const string& factFile,
				const BucketID& bcktID,
				vector<DirChunk>* &dirVectp,
				vector<DataChunk>* &dataVectp,
				const AccessManager::treeTraversal_t howToTraverse)const
//precondition:
//	costRoot points at a cost-tree and NOT a single data chunk,
//	dirVectp and dataVectp are NULL vectors
//	bcktID contains a valid bucket id where these tree will be eventually be stored in
//postcondition:
//	dirVectp and dataVectp point at two vectors containing the instantiated dir and Data chunks
//	respectively.
{
	// ASSERTION 1: costRoot does not point at a data chunk
       	//if(costRoot->getchunkHdrp()->depth == maxDepth)
	if(isDataChunk(costRoot->getchunkHdrp()->depth, costRoot->getchunkHdrp()->localDepth, costRoot->getchunkHdrp()->nextLocalDepth, maxDepth))       	       	
       		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::traverseSingleCostTreeCreateChunkVectors ==> ASSERTION 1: found single data chunk in input tree\n");

	// ASSERTION 2: null input pointers
	if(dirVectp || dataVectp)
       		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::traverseSingleCostTreeCreateChunkVectors ==> ASSERTION 2: not null vector pointers\n");

	// ASSERTION 3: not null bucket id
	if(bcktID.isnull())
       		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::traverseSingleCostTreeCreateChunkVectors ==> ASSERTION 3: found null input bucket id\n");

	// Reserve space for the two chunk vectors
	dirVectp = new vector<DirChunk>;
	unsigned int numDirChunks = 0;
	CostNode::countDirChunksOfTree(costRoot, maxDepth, numDirChunks);
	try{
		dirVectp->reserve(numDirChunks);
	}
	catch(std::bad_alloc&){
		GeneralError error(__FILE__, __LINE__, "AccessManagerImpl::traverseSingleCostTreeCreateChunkVectors ==> Memory allocation request failed!\n");
		//log error to error log file
		errorLogStream << error << endl;
		//output to screen
	        outputLogStream << "(File: " << __FILE__ << ", line: " << __LINE__ << "): " << "AccessManagerImpl::traverseSingleCostTreeCreateChunkVectors ==> Memory allocation request failed!\n" << endl;		
		
	        delete  dirVectp;
	        dirVectp = 0;
		throw; //throw bad_alloc, nothing we can do!
	}//catch
	

	dataVectp = new vector<DataChunk>;
	unsigned int numDataChunks = 0;
	CostNode::countDataChunksOfTree(costRoot, maxDepth, numDataChunks);
	try{
		dataVectp->reserve(numDataChunks);
	}
	catch(std::bad_alloc&){
		GeneralError error(__FILE__, __LINE__, "AccessManagerImpl::traverseSingleCostTreeCreateChunkVectors ==> Memory allocation request failed!\n");
		//log error to error log file
		errorLogStream << error << endl;
		//output to screen
	        outputLogStream << "(File: " << __FILE__ << ", line: " << __LINE__ << "): " << "AccessManagerImpl::traverseSingleCostTreeCreateChunkVectors ==> Memory allocation request failed!\n" << endl;		
		
	        delete  dirVectp;
	        dirVectp = 0;
	        delete dataVectp;
	        dataVectp = 0;
		throw; //throw bad_alloc, nothing we can do!
	}//catch
	
	// Fill the chunk vectors by descending the cost tree with the appropriate
	// traversal method.
        switch(howToTraverse){
                case AccessManager::breadthFirst:
                        {
                        	queue<CostNode*> nextToVisit; // breadth-1st queue
                        	try {
                        		descendBreadth1stCostTree(
                        		        maxDepth,
                        		        numFacts,
                        		        costRoot,
                        		        factFile,
                        		        bcktID,
                        		        nextToVisit,
                        		        dirVectp,
                        		        dataVectp);
                        	}
                              	catch(GeneralError& error) {
                              		GeneralError e("AccessManagerImpl::traverseSingleCostTreeCreateChunkVectors ==>");
                              		error += e;
                        	        delete  dirVectp;
                        	        dirVectp = 0;
                        	        delete dataVectp;
                        	        dataVectp = 0;
                              		throw error;
                              	}
                              	catch(...){
                        	        delete  dirVectp;
                        	        dirVectp = 0;
                        	        delete dataVectp;
                        	        dataVectp = 0;
                              		throw;                              	
                              	}
                      	}//end block
                      	break;
                case AccessManager::depthFirst:
                        {
                        	try {
                        		descendDepth1stCostTree(
                        		        maxDepth,
                        		        numFacts,
        				        costRoot,
        				        factFile,
        				        bcktID,
        				        dirVectp,
        				        dataVectp);
                        	}
                              	catch(GeneralError& error) {
                              		GeneralError e("AccessManagerImpl::traverseSingleCostTreeCreateChunkVectors ==>");
                              		error += e;
                        	        delete  dirVectp;
                        	        dirVectp = 0;
                        	        delete dataVectp;
                        	        dataVectp = 0;
                              		throw error;
                              	}
                              	catch(...){
                        	        delete  dirVectp;
                        	        dirVectp = 0;
                        	        delete dataVectp;
                        	        dataVectp = 0;
                              		throw;                              	
                              	}
                      	}//end block
              		break;
                 default:
               	        delete  dirVectp;
               	        dirVectp = 0;
               	        delete dataVectp;
               	        dataVectp = 0;
                        throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::traverseSingleCostTreeCreateChunkVectors ==> Unkown traversal method\n");
                        break;
         }//end switch

      	//ASSERTION 4: valid returned vectors
       	if(dirVectp->empty() || dataVectp->empty()){
               	        delete  dirVectp;
               	        dirVectp = 0;
               	        delete dataVectp;
               	        dataVectp = 0;
       			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::traverseSingleCostTreeCreateChunkVectors ==> ASSERTION 4: empty chunk vector!!\n");
       	}
}//end of AccessManagerImpl::traverseSingleCostTreeCreateChunkVectors

void AccessManagerImpl::createSingleDataChunkDiskBucketInHeap(
					unsigned int maxDepth,
					unsigned int numFacts,
					const BucketID& bcktID,	
					const DataChunk& datachunk,
					DiskBucket* &dbuckp)const
//precondition:
//	dbuckp points to NULL. datachunk contains the DataChunk that we want to
//	store in the DiskBucket.
//postcondition:
//	dbuckp points at a heap allocated DiskBucket where the data chunk has been placed in
//	its body.					
{
	//ASSERTION1: dbuckp points to NULL
	if(dbuckp)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::createSingleDataChunkDiskBucketInHeap ==> ASSERTION1: input pointer to DiskBucket should be null\n");

	// allocate DiskBucket in heap			
	try{
		dbuckp = new DiskBucket;
	}
	catch(std::bad_alloc&){		
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::createSingleDataChunkDiskBucketInHeap ==> cant allocate space for new DiskBucket!\n");		
	}	
	
	// initialize directory pointer to point one beyond last byte of body
	dbuckp->offsetInBucket = reinterpret_cast<DiskBucketHeader::dirent_t*>(&(dbuckp->body[DiskBucket::bodysize]));
	// initialize the DiskBucketHeader
	dbuckp->hdr.id.rid = bcktID.rid; // store the bucket id
	dbuckp->hdr.next.rid = serial_t::null; // Init the links to other buckets with null ids
	dbuckp->hdr.previous.rid = serial_t::null;	
	dbuckp->hdr.no_chunks = 0;  // init chunk counter
	dbuckp->hdr.next_offset = 0; //next free byte offset in the body
	dbuckp->hdr.freespace = DiskBucket::bodysize;
	dbuckp->hdr.no_subtrees = 1; // single "tree" stored
	dbuckp->hdr.subtree_dir_entry[0] = 0; // the root of this "tree" will be stored at chunk slot 0
	dbuckp->hdr.no_ovrfl_next = 0; // no overflow-bucket  chain used
	
	char* nextFreeBytep = dbuckp->body; //init current byte pointer
	// place the data chunk in the bucket's body		
        try{
	        placeSingleDataChunkInDiskBucketBody(maxDepth, numFacts,
		                        datachunk, dbuckp, nextFreeBytep);
        }
       	catch(GeneralError& error) {
       		GeneralError e("AccessManagerImpl::createSingleDataChunkDiskBucketInHeap ==> ");
       		error += e;
       		delete dbuckp;
       		dbuckp = 0;
       		throw error;
       	}
       	catch(...){
       		delete dbuckp;
       		dbuckp = 0;
       		throw;       	
       	}							
}//AccessManagerImpl::createSingleDataChunkDiskBucketInHeap

void AccessManagerImpl::placeSingleDataChunkInDiskBucketBody(
			unsigned int maxDepth,
			unsigned int numFacts,
			const DataChunk& datachunk,
			DiskBucket* const dbuckp,
			char* &nextFreeBytep)const
//precondition:
//	datachunk (input parameter) is a reference to the DataChunk instance that we want to place in the bucket's body.
//	dbuckp (input parameter) is
//	a const pointer to an allocated DiskBucket, where its header members have been initialized.
//	nextFreeBytep is a byte pointer (input+output parameter) that points at the beginning
//	of free space in the DiskBucket's body.
// postcondition:
//      the  datachunk has been placed in the body of the bucket.The nextFreeBytep pointer points at the first free byte in the
//      body of the bucket.
{
	//ASSERTION 1: there is free space to store this data chunk
	// add the size of the datachunk
	size_t szBytes = datachunk.gethdr().size;
	//also add the cost for the corresponding entry in the internal directory of the DiskBucket
	szBytes += sizeof(DiskBucketHeader::dirent_t);
	if(szBytes > dbuckp->hdr.freespace)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeSingleDataChunkInDiskBucketBody ==> ASSERTION 1: DataChunk does not fit in DiskBucket!\n");		

      	// update bucket directory (chunk slots begin from slot 0)
      	dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = dbuckp->hdr.next_offset;
      	dbuckp->hdr.freespace -= sizeof(DiskBucketHeader::dirent_t);
        		
      	// convert DataChunk to DiskDataChunk
      	DiskDataChunk* chnkp = 0;
      	try{
                chnkp = dataChunk2DiskDataChunk(datachunk, numFacts, maxDepth);	      		
      	}
 	catch(GeneralError& error) {
 		GeneralError e("AccessManagerImpl::placeSingleDataChunkInDiskBucketBody ==> ");
 		error += e;
 		throw error;
 	}		
	      	
 	// Now, place the parts of the DiskDataChunk into the body of the DiskBucket
      	size_t chnk_size = 0;
      	size_t hdr_size = 0;
      	try{      		
                placeDiskDataChunkInBcktBody(chnkp, maxDepth, nextFreeBytep, hdr_size, chnk_size);					
	}
 	catch(GeneralError& error) {
 		GeneralError e("AccessManagerImpl::placeSingleDataChunkInDiskBucketBody ==> ");
 		error += e;
		delete chnkp;
 		throw error;
 	}
 	catch(...){
		delete chnkp;
 		throw; 	
 	}		
 	delete chnkp; //free up memory
 	chnkp = 0;
 	
 	#ifdef DEBUGGING
                //ASSERTION 1.2 : no chunk size mismatch         	
                if(datachunk.gethdr().size != chnk_size)
                        throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeSingleDataChunkInDiskBucketBody ==>  ASSERTION 1.2: DataChunk size mismatch!\n");		
 	#endif

        //update next free byte offset indicator
     	dbuckp->hdr.next_offset += chnk_size;        		
     	//update free space indicator
     	dbuckp->hdr.freespace -= chnk_size;
     		
     	// update bucket header: chunk counter        		
       	dbuckp->hdr.no_chunks++;
}// end AccessManagerImpl::placeSingleDataChunkInDiskBucketBody					

void AccessManagerImpl::createSingleTreeDiskBucketInHeap(unsigned int maxDepth, unsigned int numFacts,
		const BucketID& bcktID,	 const vector<DirChunk>*dirVectp,
		const vector<DataChunk>*dataVectp, DiskBucket* &dbuckp,
		const AccessManager::treeTraversal_t howToTraverse)const
// precondition:
//	dbuckp points to NULL && dirVectp and dataVectp contain the chunks of a single tree
//	that can fit in a single DiskBucket. howToTraverse shows how we want to store the chunks
//      in the body of the bucket, i.e., in a depth first or in breadth first manner. The former
//      assumes that the two input vectors have been created with AccessManagerImpl::descendDepth1stCostTree,
//      while the latter assumes creation  with AccessManagerImpl::descendBreadthFirstCostTree.
// postcondition:
//	the chunks of the two vectors have been stored in the chunk slots of a DiskBucket that is allocated
//	in heap and pointed to by dbuckp.
{
	//ASSERTION1: dbuckp points to NULL
	if(dbuckp)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::createSingleTreeDiskBucketInHeap ==> ASSERTION1: input pointer to DiskBucket should be null\n");

	// allocate DiskBucket in heap			
	try{
		dbuckp = new DiskBucket;
	}
	catch(std::bad_alloc&){
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::createSingleTreeDiskBucketInHeap ==> cant allocate space for new DiskBucket!\n");		
	}	

	// initialize directory pointer to point one beyond last byte of body
	dbuckp->offsetInBucket = reinterpret_cast<DiskBucketHeader::dirent_t*>(&(dbuckp->body[DiskBucket::bodysize]));
	// initialize the DiskBucketHeader
	dbuckp->hdr.id.rid = bcktID.rid; // store the bucket id	
	dbuckp->hdr.next.rid = serial_t::null; 	// Init the links to other buckets with null ids
	dbuckp->hdr.previous.rid = serial_t::null;	
	dbuckp->hdr.no_chunks = 0; // init chunk counter
	dbuckp->hdr.next_offset = 0; //next free byte offset in the body
	dbuckp->hdr.freespace = DiskBucket::bodysize;
	dbuckp->hdr.no_subtrees = 1; // single tree stored
	dbuckp->hdr.subtree_dir_entry[0] = 0; // the root of this tree will be stored at chunk slot 0
	dbuckp->hdr.no_ovrfl_next = 0; // no overflow-bucket  chain used
	
	char* nextFreeBytep = dbuckp->body; //init current byte pointer
	//size_t curr_offs = dbuckp->hdr.next_offset;  // init current byte offset
	
        //According to the desired traversal method store the chunks in the body of the bucket	
        try{
	        placeChunksOfSingleTreeInDiskBucketBody(maxDepth, numFacts,
		                        dirVectp, dataVectp, dbuckp, nextFreeBytep,howToTraverse);
        }
       	catch(GeneralError& error) {
       		GeneralError e("AccessManagerImpl::createSingleTreeDiskBucketInHeap ==> ");
       		error += e;
       		delete dbuckp;
       		dbuckp = 0;
       		throw error;
       	}
       	catch(...){
       		delete dbuckp;
       		dbuckp = 0;
       		throw;       	
       	}		
}//end of AccessManagerImpl::createSingleTreeDiskBucketInHeap

void AccessManagerImpl::placeChunksOfSingleTreeInDiskBucketBody(
			unsigned int maxDepth,
			unsigned int numFacts,
			const vector<DirChunk>* const dirVectp,
			const vector<DataChunk>* const dataVectp,
			DiskBucket* const dbuckp,
			char* &nextFreeBytep,
			const AccessManager::treeTraversal_t howToTraverse)const
// precondition:
//      dirVectp and dataVectp (input parameters) contain the chunks of a single tree
//	that can fit in a single DiskBucket. These chunks have been placed in the 2 vectors with a
//      call to AccessManagerImpl::traverseSingleCostTreeCreateChunkVectors. dbuckp (input parameter) is
//	a const pointer to an allocated DiskBucket, where its header members have been initialized.
//	Finally, nextFreeBytep is a byte pointer (input+output parameter) that points at the beginning
//	of free space in the DiskBucket's body.
// postcondition:
//      the chunks of the two vectors have been placed in the body of the bucket with respect to a
//      traversal method of the tree. The nextFreeBytep pointer points at the first free byte in the
//      body of the bucket.
{
        //According to the desired traversal method store the chunks in the body of the bucket
        switch(howToTraverse){
                case AccessManager::breadthFirst:
                        try{
                                _storeBreadth1stInDiskBucket(maxDepth, numFacts,
		                        dirVectp, dataVectp, dbuckp, nextFreeBytep);
                        }
                	catch(GeneralError& error) {
                		GeneralError e("AccessManagerImpl::placeChunksOfSingleTreeInDiskBucketBody ==> ");
                		error += e;
                		throw error;
                	}
                        break;

                case AccessManager::depthFirst:
                        try{
                                _storeDepth1stInDiskBucket(maxDepth, numFacts,
		                        dirVectp, dataVectp, dbuckp, nextFreeBytep);
                        }
                	catch(GeneralError& error) {
                		GeneralError e("AccessManagerImpl::placeChunksOfSingleTreeInDiskBucketBody ==> ");
                		error += e;
                		throw error;
                	}
                        break;
                default:
                        throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeChunksOfSingleTreeInDiskBucketBody ==> unknown traversal method\n");
                        break;
        }//end switch
}// end of AccessManagerImpl::placeChunksOfSingleTreeInDiskBucketBody

				
void AccessManagerImpl::_storeBreadth1stInDiskBucket(unsigned int maxDepth, unsigned int numFacts,
		 const vector<DirChunk>* const dirVectp, const vector<DataChunk>* const dataVectp,
		 DiskBucket* const dbuckp, char* &nextFreeBytep)const
// precondition:
//      dirVectp and dataVectp (input parameters) contain the chunks of a single tree
//	that can fit in a single DiskBucket. These chunks have been placed in the 2 vectors with a
//      call to AccessManagerImpl::descendBreadth1stCostTree. dbuckp (input parameter) is a const pointer to an
//      allocated DiskBucket, where its header members have been initialized. Finally, nextFreeBytep is
//      a byte pointer (input+output parameter) that points at the beginning of the DiskBucket's body.
// postcondition:
//      the chunks of the two vectors have been placed in the body of the bucket with respect to a
//      breadth first traversal of the tree. The nextFreeBytep pointer points at the first free byte in the
//      body of the bucket.
{
        // for each dir chunk of this subtree
        for(vector<DirChunk>::const_iterator dir_i = dirVectp->begin();
            dir_i != dirVectp->end(); dir_i++){
		// loop invariant: a DirChunk is stored in each iteration in the body
		//		   of the DiskBucket

		//ASSERTION 1: there is free space to store this dir chunk
		// add the size of the datachunk
        	size_t szBytes = dir_i->gethdr().size;
        	//also add the cost for the corresponding entry in the internal directory of the DiskBucket
        	szBytes += sizeof(DiskBucketHeader::dirent_t);
		if(szBytes > dbuckp->hdr.freespace)
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::_storeBreadth1stInDiskBucket ==> ASSERTION 1: DirChunk does not fit in DiskBucket!\n");		

      		// update bucket directory (chunk slots begin from slot 0)
      		dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = dbuckp->hdr.next_offset;
      		dbuckp->hdr.freespace -= sizeof(DiskBucketHeader::dirent_t);
        		
      		// convert DirChunk to DiskDirChunk
      		DiskDirChunk* chnkp = 0;
      		try{
                        chnkp = dirChunk2DiskDirChunk(*dir_i, maxDepth);
	      	}
         	catch(GeneralError& error) {
         		GeneralError e("AccessManagerImpl::_storeBreadth1stInDiskBucket==>");
         		error += e;
         		throw error;
         	}		
	      	
         	// Now, place the parts of the DiskDirChunk into the body of the DiskBucket
      		size_t chnk_size = 0;
      		size_t hdr_size = 0;
      		try{      		
                        placeDiskDirChunkInBcktBody(chnkp, maxDepth, nextFreeBytep, hdr_size, chnk_size);							
		}
         	catch(GeneralError& error) {
         		GeneralError e("AccessManagerImpl::_storeBreadth1stInDiskBucket==>");
         		error += e;
         		delete chnkp;
         		throw error;
         	}
         	catch(...){
         		delete chnkp;
         		throw;         	
         	}		
         	#ifdef DEBUGGING
                        //ASSERTION 1.1 : no chunk size mismatch         	
                        if(dir_i->gethdr().size != chnk_size){
                        	delete chnkp;
                                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::_storeBreadth1stInDiskBucket ==> ASSERTION 1.1: DirChunk size mismatch!\n");	
			}//end if
         	#endif
         	
                //update next free byte offset indicator
     		dbuckp->hdr.next_offset += chnk_size;
     		//update free space indicator        		
     		dbuckp->hdr.freespace -= chnk_size;
     		
     		// update bucket header: chunk counter
               	dbuckp->hdr.no_chunks++;

               	#ifdef DEBUGGING                 	
               	cout<<"dirchunk : "<<dir_i->gethdr().id.getcid()<<" just placed in a DiskBucket.\n";
               	cout<<"freespace = "<<dbuckp->hdr.freespace<<endl;
               	cout<<"no chunks = "<<dbuckp->hdr.no_chunks<<endl;
               	cout<<"next offset = "<<dbuckp->hdr.next_offset<<endl;
               	//cout<<"current offset = "<<curr_offs<<endl;
                cout<<"---------------------\n";
                #endif								

                delete chnkp; //free up memory
                chnkp = 0;
        }//end for

        // for each data chunk of this subtree
        for(vector<DataChunk>::const_iterator data_i = dataVectp->begin();
            data_i != dataVectp->end(); data_i++){
		// loop invariant: a DataChunk is stored in each iteration in the body
		//		   of the DiskBucket

		//ASSERTION 2: there is free space to store this data chunk
		// add the size of the datachunk
        	size_t szBytes = data_i->gethdr().size;
        	//also add the cost for the corresponding entry in the internal directory of the DiskBucket
        	szBytes += sizeof(DiskBucketHeader::dirent_t);		
		if(szBytes > dbuckp->hdr.freespace)
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::_storeBreadth1stInDiskBucket ==> ASSERTION 2: DataChunk does not fit in DiskBucket!\n");		

      		// update bucket directory (chunk slots begin from slot 0)
      		dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = dbuckp->hdr.next_offset;
      		dbuckp->hdr.freespace -= sizeof(DiskBucketHeader::dirent_t);
        		
      		// convert DataChunk to DiskDataChunk
      		DiskDataChunk* chnkp = 0;
      		try{
                        chnkp = dataChunk2DiskDataChunk(*data_i, numFacts, maxDepth);	      		
	      	}
         	catch(GeneralError& error) {
         		GeneralError e("AccessManagerImpl::_storeBreadth1stInDiskBucket==>");
         		error += e;
         		throw error;
         	}		
	      	
         	// Now, place the parts of the DiskDataChunk into the body of the DiskBucket
      		size_t chnk_size = 0;
      		size_t hdr_size = 0;
      		try{      		
                        placeDiskDataChunkInBcktBody(chnkp, maxDepth, nextFreeBytep, hdr_size, chnk_size);					
		}
         	catch(GeneralError& error) {
         		GeneralError e("");
         		error += e;
         		delete chnkp;
         		throw error;
         	}
         	catch(...){
         		delete chnkp;
         		throw;         	
         	}
         	delete chnkp; //free up memory		
         	chnkp = 0;
         	
         	#ifdef DEBUGGING
                        //ASSERTION 2.1 : no chunk size mismatch         	
                        if(data_i->gethdr().size != chnk_size)
                                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::_storeBreadth1stInDiskBucket ==> ASSERTION 1.2: DataChunk size mismatch!\n");		
         	#endif

                //update next free byte offset indicator
     		dbuckp->hdr.next_offset += chnk_size;        		
     		//update free space indicator
     		dbuckp->hdr.freespace -= chnk_size;
     		
     		// update bucket header: chunk counter        		
               	dbuckp->hdr.no_chunks++;

               	#ifdef DEBUGGING                 	
               	cout<<"datachunk : "<<data_i->gethdr().id.getcid()<<" just placed in a DiskBucket.\n";
               	cout<<"freespace = "<<dbuckp->hdr.freespace<<endl;
               	cout<<"no chunks = "<<dbuckp->hdr.no_chunks<<endl;
               	cout<<"next offset = "<<dbuckp->hdr.next_offset<<endl;
               	//cout<<"current offset = "<<curr_offs<<endl;
                cout<<"---------------------\n";
                #endif								

        }//end for
}//end of AccessManagerImpl::_storeBreadth1stInDiskBucket

void AccessManagerImpl::_storeDepth1stInDiskBucket(unsigned int maxDepth, unsigned int numFacts,
		 const vector<DirChunk>* const dirVectp, const vector<DataChunk>* const dataVectp,
		 DiskBucket* const dbuckp, char* &nextFreeBytep)const
// precondition:
//      dirVectp and dataVectp (input parameters) contain the chunks of a single tree
//	that can fit in a single DiskBucket. These chunks have been placed in the 2 vectors with a
//      call to AccessManagerImpl::descendDepth1stCostTree. dbuckp (input parameter) is a const pointer to an
//      allocated DiskBucket, where its header members have been initialized. Finally, nextFreeBytep is
//      a byte pointer (input+output parameter) that points at the beginning of the DiskBucket's body.
// postcondition:
//      the chunks of the two vectors have been placed in the body of the bucket with respect to a
//      depth first traversal of the tree. The nextFreeBytep pointer points at the first free byte in the
//      body of the bucket.
{

        //for each dirchunk in the vector, store chunks in the row until
        // you store a (max depth-1) dir chunk. Then you have to continue selecting chunks from the
        // data chunk vector, in order to get the corresponding data chunks (under the same father).
        // After that we continue from the dir chunk vector at the point we were left.
        for(vector<DirChunk>::const_iterator dir_i = dirVectp->begin();
                                                dir_i != dirVectp->end(); dir_i++){
		// loop invariant: a DirChunk is stored in each iteration in the body
		//		   of the DiskBucket. If this DirChunk has a maxDepth-1 depth
		//                 then we also store all the DataChunks that are its children.

		//ASSERTION 1: there is free space to store this dir chunk
		// add the size of the datachunk
        	size_t szBytes = dir_i->gethdr().size;
        	//also add the cost for the corresponding entry in the internal directory of the DiskBucket
        	szBytes += sizeof(DiskBucketHeader::dirent_t);		
		if(szBytes > dbuckp->hdr.freespace)
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::_storeDepth1stInDiskBucket ==> ASSERTION 1: DirChunk does not fit in DiskBucket!\n");		

      		// update bucket directory (chunk slots begin from slot 0)
      		dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = dbuckp->hdr.next_offset;
      		dbuckp->hdr.freespace -= sizeof(DiskBucketHeader::dirent_t);
        		
      		// convert DirChunk to DiskDirChunk
      		DiskDirChunk* chnkp = 0;
      		try{
                        chnkp = dirChunk2DiskDirChunk(*dir_i, maxDepth);
	      	}
         	catch(GeneralError& error) {
         		GeneralError e("AccessManagerImpl::_storeDepth1stInDiskBucket==>");
         		error += e;
         		throw error;
         	}		
	      	
         	// Now, place the parts of the DiskDirChunk into the body of the DiskBucket
      		size_t chnk_size = 0;
      		size_t hdr_size = 0;
      		try{      		
                        placeDiskDirChunkInBcktBody(chnkp, maxDepth, nextFreeBytep, hdr_size, chnk_size);							
		}
         	catch(GeneralError& error) {
         		GeneralError e("AccessManagerImpl::_storeDepth1stInDiskBucket==>");
         		error += e;
         		delete chnkp;
         		throw error;
         	}
         	catch(...){
         		delete chnkp;
         		throw;         	
         	}	
         	#ifdef DEBUGGING
                        //ASSERTION 1.1 : no chunk size mismatch         	
                        if(dir_i->gethdr().size != chnk_size){
                        	delete chnkp;
                                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::_storeDepth1stInDiskBucket ==> ASSERTION 1.1: DirChunk size mismatch!\n");
                        }//end if
         	#endif
         		
                //update next free byte offset indicator
     		dbuckp->hdr.next_offset += chnk_size;
     		//update free space indicator        		
     		dbuckp->hdr.freespace -= chnk_size;
     		
     		// update bucket header: chunk counter
               	dbuckp->hdr.no_chunks++;

               	#ifdef DEBUGGING                 	
               	cout<<"dirchunk : "<<dir_i->gethdr().id.getcid()<<" just placed in a DiskBucket.\n";
               	cout<<"freespace = "<<dbuckp->hdr.freespace<<endl;
               	cout<<"no chunks = "<<dbuckp->hdr.no_chunks<<endl;
               	cout<<"next offset = "<<dbuckp->hdr.next_offset<<endl;
               	//cout<<"current offset = "<<curr_offs<<endl;
                cout<<"---------------------\n";
                #endif								

                delete chnkp; //free up memory
                chnkp = 0;
                //if this is a dir chunk at max depth
                if((*dir_i).gethdr().depth ==  maxDepth-1){
                        //then we have to continue selecting chunks from the DataChunk vector

                        //find all data chunks in data chunk vect with a prefix in their chunk id
                        //equal with *dir_i.gethdr().id
                        //for each datachunk in the vector
                        for(vector<DataChunk>::const_iterator data_i = dataVectp->begin();
                            data_i != dataVectp->end(); data_i++){
                                //if we have a prefix match
	                        if( (*data_i).gethdr().id.getcid().find((*dir_i).gethdr().id.getcid())
	                                 != string::npos ) {

                         		// loop invariant: a DataChunk is stored in each iteration in the body
                         		//		   of the DiskBucket. All these DataChunks have the same
                         		//                 parent DirChunk

                         		//ASSERTION 2: there is free space to store this data chunk
                        		// add the size of the datachunk
                                	size_t szBytes = data_i->gethdr().size;
                                	//also add the cost for the corresponding entry in the internal directory of the DiskBucket
                                	szBytes += sizeof(DiskBucketHeader::dirent_t);		                         		
                         		if(szBytes > dbuckp->hdr.freespace)
                         			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::_storeDepth1stInDiskBucket ==> ASSERTION 2: DataChunk does not fit in DiskBucket!\n");		

                               		// update bucket directory (chunk slots begin from slot 0)
                               		dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = dbuckp->hdr.next_offset;
                               		dbuckp->hdr.freespace -= sizeof(DiskBucketHeader::dirent_t);
                                 		
                               		// convert DataChunk to DiskDataChunk
                               		DiskDataChunk* chnkp = 0;
                               		try{
                                                 chnkp = dataChunk2DiskDataChunk(*data_i, numFacts, maxDepth);	      		
                         	      	}
                                  	catch(GeneralError& error) {
                                  		GeneralError e("AccessManagerImpl::_storeDepth1stInDiskBucket==>");
                                  		error += e;
                                  		throw error;
                                  	}		
                         	      	
                                  	// Now, place the parts of the DiskDataChunk into the body of the DiskBucket
                               		size_t chnk_size = 0;
                               		size_t hdr_size = 0;
                               		try{      		
                                                 placeDiskDataChunkInBcktBody(chnkp, maxDepth,
                                                        nextFreeBytep, hdr_size, chnk_size);					
                         		}
                                  	catch(GeneralError& error) {
                                  		GeneralError e("");
                                  		error += e;
                                  		delete chnkp;
                                  		throw error;
                                  	}
                                  	catch(...){
                                  		delete chnkp;
                                  		throw;                                  	
                                  	}
                                  	delete chnkp; //free up memory
                                  	chnkp = 0;
                                  			
                                	#ifdef DEBUGGING
                                               //ASSERTION 2.1 : no chunk size mismatch         	
                                               if(data_i->gethdr().size != chnk_size)
                                                       throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::_storeDepth1stInDiskBucket ==> ASSERTION 2.1: DataChunk size mismatch!\n");		
                                	#endif

                                         //update next free byte offset indicator
                              		dbuckp->hdr.next_offset += chnk_size;        		
                              		//update free space indicator
                              		dbuckp->hdr.freespace -= chnk_size;
                              		
                              		// update bucket header: chunk counter        		
                                       	dbuckp->hdr.no_chunks++;

                                       	#ifdef DEBUGGING                 	
                                       	cout<<"datachunk : "<<data_i->gethdr().id.getcid()<<" just placed in a DiskBucket.\n";
                                       	cout<<"freespace = "<<dbuckp->hdr.freespace<<endl;
                                       	cout<<"no chunks = "<<dbuckp->hdr.no_chunks<<endl;
                                       	cout<<"next offset = "<<dbuckp->hdr.next_offset<<endl;
                                       	//cout<<"current offset = "<<curr_offs<<endl;
                                        cout<<"---------------------\n";
                                        #endif								
                                }//end if
                        }//end for
                }//end if
        }//end for
}//end of AccessManagerImpl::_storeDepth1stInDiskBucket

DiskDirChunk* AccessManagerImpl::dirChunk2DiskDirChunk(const DirChunk& dirchnk, unsigned int maxDepth) const
// precondition:
//	dirchnk is a DirChunk instance filled with valid entries. maxDepth is the maximum depth of
//      the cube in question.
// postcondition:
//	A DiskDirChunk has been allocated in heap space that contains the values of
//	dirchnk and a pointer to it is returned.	
{

	//ASSERTION: this is a dir chunk
	if(!AccessManagerImpl::isDirChunk(dirchnk.gethdr().depth, dirchnk.gethdr().localDepth, dirchnk.gethdr().nextLocalDepth, maxDepth))
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> wrong chunk type!\n");			
			
	/*		
        //ASSERTION 1.1: this is not the root chunk
        if(dirchnk.gethdr().depth == Chunk::MIN_DEPTH)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> ASSERTION 1.1: this method cannot handle the root chunk\n");	
        //ASSERTION 1.2: this is not a data chunk
        if(dirchnk.gethdr().depth == maxDepth)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> ASSERTION 1.2: this method cannot handle a data chunk\n");			
        //ASSERTION 1.3: valid depth value
        if(dirchnk.gethdr().depth < Chunk::MIN_DEPTH || dirchnk.gethdr().depth > maxDepth)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> ASSERTION 1.3: invalid depth value\n");	
	*/
	
	//Is this the root chunk?
	bool isRootChunk = AccessManagerImpl::isRootChunk(dirchnk.gethdr().depth, dirchnk.gethdr().localDepth, dirchnk.gethdr().nextLocalDepth, maxDepth);

	//Is this an artificially chunked dir chunk?
	bool isArtifChunk = AccessManagerImpl::isArtificialChunk(dirchnk.gethdr().localDepth);	
				
	//allocate new DiskDirChunk
        DiskDirChunk* chnkp=0;
	try{
		chnkp = new DiskDirChunk;
	}
	catch(std::bad_alloc&){
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> cant allocate space for new DiskDirChunk!\n");		
	}	
		
	// 1. first copy the headers
	
	//insert values for non-pointer members
	chnkp->hdr.depth = dirchnk.gethdr().depth;
	chnkp->hdr.local_depth = dirchnk.gethdr().localDepth;
	chnkp->hdr.next_local_depth = dirchnk.gethdr().nextLocalDepth;	
	chnkp->hdr.no_dims = dirchnk.gethdr().numDim;		
	chnkp->hdr.no_measures = 0; // this is a directory chunk
	chnkp->hdr.no_entries = dirchnk.gethdr().totNumCells;
	
	// store the chunk id
	
	//ASSERTION : global depth and chunk-id compatibility
	if(dirchnk.gethdr().depth != dirchnk.gethdr().id.getChunkGlobalDepth()){
		delete chnkp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> depth and chunk-id mismatch\n");	
	}// end if
	
        //if this is not the root chunk
        if(!isRootChunk){
        	//first find the number of domains
        	int noDomains;
                try{
                	noDomains = DiskChunkHeader::getNoOfDomainsFromDepth(int(chnkp->hdr.depth), int(chnkp->hdr.local_depth));
                }
        	catch(GeneralError& error) {
                	GeneralError e("AccessManagerImpl::dirChunk2DiskDirChunk ==> ");
                	error += e;
                	delete chnkp;
                	throw error;
                }
                catch(...){
                	delete chnkp;
                	throw;
                }			
        //        if(chnkp->hdr.local_depth == Chunk::NULL_DEPTH)
        //        	noDomains = chnkp->hdr.depth - Chunk::MIN_DEPTH;                	                	
        //        else {// local_depth >= MIN_DEPTH
                	//ASSERTION 1.4
        //        	if(!(chnkp->hdr.local_depth >= Chunk::MIN_DEPTH)
        //        		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> ASSERTION 1.4: invalid local depth in chunk.\n");
        //        	noDomains = (chnkp->hdr.depth - Chunk::MIN_DEPTH)+(chnkp->hdr.local_depth - Chunk::MIN_DEPTH);
        //        }//end else
          	
        	//allocate space for the domains 	
        	try{	
        		chnkp->hdr.chunk_id = new DiskChunkHeader::Domain_t[noDomains];
        	}
        	catch(std::bad_alloc&){
        		delete chnkp;
        		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> cant allocate space for the domains!\n");		
        	}	
          	
        	//ASSERTION2 : valid no_dims value
        	if(chnkp->hdr.no_dims <= 0) {
        		delete chnkp;
        		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> ASSERTION2: invalid num of dims\n");	
        	}// end if
                //ASSERTION 2.1: num of dims and chunk id compatibility		
                bool isroot = false;		
                if(chnkp->hdr.no_dims != dirchnk.gethdr().id.getChunkNumOfDim(isroot)){
                	delete chnkp;
                        throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> ASSERTION 2.1: num of dims and chunk id mismatch\n");	
                }//end if
                #ifdef DEBUGGING
                if(isroot){
                	// normally it will never enter here, because even if isroot becomes true, the  above !=
                	// will also be true
                	delete chnkp;
                        throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> ASSERTION 2.1: root chunk encountered\n");	
                }// end if
                #endif
          		
        	//allocate space for each domain's order-codes		
        	for(int i = 0; i<noDomains; i++) { //for each domain of the chunk id
        		try{
        			chnkp->hdr.chunk_id[i].ordercodes = new DiskChunkHeader::ordercode_t[chnkp->hdr.no_dims];
        		}
                 	catch(std::bad_alloc&){
                 		delete chnkp;
                 		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> cant allocate space for the ordercodes!\n");		
                 	}			
        	}//end for
          	
        	string cid = dirchnk.gethdr().id.getcid();	
        	string::size_type begin = 0;
        	for(int i=0; i<noDomains; i++) { //for each domain of the chunk id
        		//get the appropriate substring
        		string::size_type end = cid.find(".", begin); // get next "."
        		// if end==npos then no "." found, i.e. this is the last domain
        		// end-begin == the length of the domain substring => substring cid[begin]...cid[begin+(end-begin)-1]		
        		string domain = (end == string::npos) ?
        		                        string(cid, begin, cid.length()-begin) : string(cid, begin, end-begin);				
          		
                        string::size_type b = 0;		
        		for(int j =0; j<chnkp->hdr.no_dims; j++){ //for each order-code of the domain			
        			string::size_type e = domain.find("|", b); // get next "|"
        			string ocstr = (e == string::npos) ?
        			                        string(domain, b, domain.length()-b) : string(domain, b, e-b);
        			chnkp->hdr.chunk_id[i].ordercodes[j] = atoi(ocstr.c_str());			
        			b = e+1;
        		}//end for
        		begin = end+1;			
        	}//end for
	}//end if
	else{ //this is the root chunk
		#ifdef DEBUGGING
		if(chnkp->hdr.chunk_id != 0) {
			delete chnkp;
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> chunkid should be 0 for the root chunk!\n");
		}//end if
		#endif
	}//else
	
	//store the order-code ranges
	//allocate space
	try{
		chnkp->hdr.oc_range = new DiskChunkHeader::OrderCodeRng_t[chnkp->hdr.no_dims];
	}
	catch(std::bad_alloc&){
		delete chnkp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> cant allocate space for the oc ranges!\n");		
	}	
		
	int i=0;
	vector<LevelRange>::const_iterator iter = dirchnk.gethdr().vectRange.begin();
	//ASSERTION3: combatible vector length and no of dimensions
	if(chnkp->hdr.no_dims != dirchnk.gethdr().vectRange.size()){
		delete chnkp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> ASSERTION3: wrong length in vector\n");	
	}//end if
	while(i<chnkp->hdr.no_dims && iter != dirchnk.gethdr().vectRange.end()){
		if(iter->leftEnd != LevelRange::NULL_RANGE || iter->rightEnd != LevelRange::NULL_RANGE) {			
			chnkp->hdr.oc_range[i].left = iter->leftEnd;		
			chnkp->hdr.oc_range[i].right = iter->rightEnd;		
		}//end if
		//else leave the default null ranges (assigned by the constructor)
		i++;
		iter++;
	}//end while	
	
	// if this is an artificial chunk copy the range to order code mappings
	if(isArtifChunk){
		//artificial hierarchy pointer
		vector<map<int, LevelRange> >* const hp = dirchnk.gethdr().artificialHierarchyp;
		
		//ASSERTION
		if(!hp){
			delete chnkp;
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> NULL pointer to artificial hierarchy for an artificial chunk encountered!\n");
		}// end if
			
         	//allocate space for the entries
         	try{
         		chnkp->rng2oc = new DiskDirChunk::Rng2oc_t[chnkp->hdr.no_dims];
         	}
         	catch(std::bad_alloc&){
         		delete chnkp;
         		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> cant allocate space for range to order code entries!\n");		
         	}
         	
         	//Fill in each range_to_oc entry
         	for(int i = 0; i<chnkp->hdr.no_dims; i++){
         		//get mapping for this dimension
         		map<int, LevelRange>& currMap = (*hp)[i];
         		
         		//number of artificial members for this dimension
         		chnkp->rng2oc[i].noMembers = currMap.size();
         		
         		//allocate range_to_oc elements for this dimension
         		try{
         			chnkp->rng2oc[i].rngElemp = new DiskDirChunk::Rng2ocElem_t[chnkp->rng2oc[i].noMembers];
         		}
                 	catch(std::bad_alloc&){
                 		delete chnkp;
                 		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> cant allocate space for range to order code elements!\n");		
                 	}
                 	
                 	//update the elements' left-boundary range values
                 	for(int j =0; j<chnkp->rng2oc[i].noMembers; j++){
				chnkp->rng2oc[i].rngElemp[j].rngLeftBoundary = currMap[j].leftEnd;				                 	
                 	}//end for                 	         		
         	}//end for		
	}//end if
	else{
		#ifdef DEBUGGING
		if(chnkp->rng2oc != 0){
			delete chnkp;
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> rng2oc should be 0 for non-artificial dir chunks!\n");				
		} // end if
		#endif	
	}//end else
	
	// 2. Next copy the entries
	//ASSERTION 3.1: valid total number of cells
	if(dirchnk.gethdr().totNumCells <= 0){
		delete chnkp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> ASSERTION 3.1: total No of cells is <= 0\n");			
	}//end if
	//allocate space for the entries
	try{
		chnkp->entry = new DiskDirChunk::DirEntry_t[dirchnk.gethdr().totNumCells];
	}
	catch(std::bad_alloc&){
		delete chnkp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> cant allocate space for dir entries!\n");		
	}	
	
	i = 0;
	vector<DirEntry>::const_iterator ent_iter = dirchnk.getentry().begin();
	//ASSERTION4: combatible vector length and no of entries
	if(chnkp->hdr.no_entries != dirchnk.getentry().size()) {
		delete chnkp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dirChunk2DiskDirChunk ==> ASSERTION4: wrong length in vector\n");	
	} //end if
	while(i<chnkp->hdr.no_entries && ent_iter != dirchnk.getentry().end()){
	        chnkp->entry[i].bucketid = ent_iter->bcktId;
       	        chnkp->entry[i].chunk_slot = ent_iter->chnkIndex;
		i++;
		ent_iter++;	
	}//end while
	
	return chnkp;
}// end of AccessManagerImpl::dirChunk2DiskDirChunk

DiskDataChunk* AccessManagerImpl::dataChunk2DiskDataChunk(const DataChunk& datachnk, unsigned int numFacts,
						      unsigned int maxDepth)const
// precondition:
//	datachnk is a DataChunk instance filled with valid entries. numFacts is the number of facts per cell
//      and maxDepth is the maximum chunking depth of the cube in question.
// postcondition:
//	A DiskDataChunk has been allocated in heap space that contains the values of
//	datachnk and a pointer to it is returned.	
{
	//ASSERTION1: input chunk is a data chunk
//	if(datachnk.gethdr().depth != maxDepth)
//		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> ASSERTION1: wrong depth for data chunk\n");
	
	//ASSERTION: this is a data chunk
	if(!AccessManagerImpl::isDataChunk(datachnk.gethdr().depth, datachnk.gethdr().localDepth, datachnk.gethdr().nextLocalDepth, maxDepth))
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDirChunk ==> wrong chunk type!\n");			

		
	//allocate new DiskDataChunk
	DiskDataChunk* chnkp = 0;
	try{
		chnkp = new DiskDataChunk;
	}
	catch(std::bad_alloc&){
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> cant allocate space for new DiskDataChunk!\n");		
	}	
		
	// 1. first copy the header
	//insert values for non-pointer members
	chnkp->hdr.depth = datachnk.gethdr().depth;
	chnkp->hdr.local_depth = datachnk.gethdr().localDepth;
	chnkp->hdr.next_local_depth = datachnk.gethdr().nextLocalDepth;	
	chnkp->hdr.no_dims = datachnk.gethdr().numDim;			
	chnkp->hdr.no_measures = numFacts;
	chnkp->hdr.no_entries = datachnk.gethdr().totNumCells;
	
	// store the chunk id
	//ASSERTION 1.1 : global depth and chunk-id compatibility
	if(datachnk.gethdr().depth != datachnk.gethdr().id.getChunkGlobalDepth()){
		delete chnkp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> ASSERTION 1.1: depth and chunk-id mismatch\n");		
	}// end if

	//first find the number of domains
	int noDomains;
        try{
        	noDomains = DiskChunkHeader::getNoOfDomainsFromDepth(int(chnkp->hdr.depth), int(chnkp->hdr.local_depth));
        }
	catch(GeneralError& error) {
        	GeneralError e("AccessManagerImpl::dataChunk2DiskDataChunk ==> ");
        	error += e;
        	delete chnkp;
        	throw error;
        }
        catch(...){
        	delete chnkp;
        	throw;
        }						
//        if(chnkp->hdr.local_depth == Chunk::NULL_DEPTH)
//        	noDomains = chnkp->hdr.depth - Chunk::MIN_DEPTH;                	                	
//        else {// local_depth >= MIN_DEPTH
//        	//ASSERTION 1.2
//        	if(!(chnkp->hdr.local_depth >= Chunk::MIN_DEPTH)
//        		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> ASSERTION 1.2: invalid local depth in chunk.\n");
//        	noDomains = (chnkp->hdr.depth - Chunk::MIN_DEPTH)+(chnkp->hdr.local_depth - Chunk::MIN_DEPTH);
//        }//end else
			
	//allocate space for the domains 	
	try{	
		chnkp->hdr.chunk_id = new DiskChunkHeader::Domain_t[noDomains];
	}
	catch(std::bad_alloc&){
		delete chnkp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> cant allocate space for the domains!\n");
	}	
	
	//ASSERTION2 : valid no_dims value
	if(chnkp->hdr.no_dims <= 0) {
		delete chnkp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> ASSERTION2: invalid num of dims\n");	
	}//end if
        //ASSERTION 2.1: num of dims and chunk id compatibility		
        bool isroot = false;		
        if(chnkp->hdr.no_dims != datachnk.gethdr().id.getChunkNumOfDim(isroot)){
        	delete chnkp;
                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> ASSERTION 2.1: num of dims and chunk id mismatch\n");	
       	}// end if
       	#ifdef DEBUGGING
        if(isroot) {
              	// normally it will never enter here, because even if isroot becomes true, the  above !=
              	// will also be true
              	delete chnkp;
                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> ASSERTION 2.1: root chunk encountered\n");	
	}//end if
	#endif
	//allocate space for each domain's order-codes		
	for(int i = 0; i<noDomains; i++) { //for each domain of the chunk id
		try{
			chnkp->hdr.chunk_id[i].ordercodes = new DiskChunkHeader::ordercode_t[chnkp->hdr.no_dims];
		}
         	catch(std::bad_alloc&){
         		delete chnkp;
         		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> cant allocate space for the ordercodes!\n");		
         	}			
	}//end for
	
	string cid = datachnk.gethdr().id.getcid();	
	string::size_type begin = 0;
	for(int i=0; i<noDomains; i++) { //for each domain of the chunk id
		//get the appropriate substring
		string::size_type end = cid.find(".", begin); // get next "."
		// if end==npos then no "." found, i.e. this is the last domain
		// end-begin == the length of the domain substring => substring cid[begin]...cid[begin+(end-begin)-1]		
		string domain = (end == string::npos) ?
		                        string(cid, begin, cid.length()-begin) : string(cid, begin, end-begin);				
		
                string::size_type b = 0;		
		for(int j =0; j<chnkp->hdr.no_dims; j++){ //for each order-code of the domain			
			string::size_type e = domain.find("|", b); // get next "|"
			string ocstr = (e == string::npos) ?
			                        string(domain, b, domain.length()-b) : string(domain, b, e-b);
			chnkp->hdr.chunk_id[i].ordercodes[j] = atoi(ocstr.c_str());			
			b = e+1;
		}//end for
		begin = end+1;			
	}//end for

	//store the order-code ranges
	//allocate space
	try{
		chnkp->hdr.oc_range = new DiskChunkHeader::OrderCodeRng_t[chnkp->hdr.no_dims];
	}
	catch(std::bad_alloc&){
		delete chnkp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> cant allocate space for the oc ranges!\n");		
	}	
		
	int i=0;
	vector<LevelRange>::const_iterator iter = datachnk.gethdr().vectRange.begin();
	//ASSERTION3: combatible vector length and no of dimensions
	if(chnkp->hdr.no_dims != datachnk.gethdr().vectRange.size()){
		delete chnkp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> ASSERTION3: wrong length in vector\n");	
	}//end if
	while(i<chnkp->hdr.no_dims && iter != datachnk.gethdr().vectRange.end()){
		if(iter->leftEnd != LevelRange::NULL_RANGE || iter->rightEnd != LevelRange::NULL_RANGE) {			
			chnkp->hdr.oc_range[i].left = iter->leftEnd;		
			chnkp->hdr.oc_range[i].right = iter->rightEnd;		
		}//end if
		//else leave the default null ranges (assigned by the constructor)
		i++;
		iter++;
	}//end while			
	
	// 2. Copy number of ace
	//ASSERTION 3.1: valid real and total number of cells
	if(datachnk.gethdr().totNumCells < datachnk.gethdr().rlNumCells){
		delete chnkp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> ASSERTION 3.1: total No of cells is less than real No of cells!\n");		
	}//end if
	chnkp->no_ace = datachnk.gethdr().rlNumCells;
	
	// 3. Next copy the bitmap
	//ASSERTION 4: combatible bitmap length and no of entries
	if(chnkp->hdr.no_entries != datachnk.getcomprBmp().size()){
		delete chnkp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> ASSERTION 4: bitmap size and No of entries mismatch\n");	
	}//end if

	//allocate space for the bitmap
	try{
		chnkp->allocBmp(chnkp->hdr.no_entries);
	}
	catch(std::bad_alloc&){
		delete chnkp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> cant allocate space for the bitmap!\n");		
	}	
	//bitmap initialization
        int bit = 0;
        for(deque<bool>::const_iterator bititer = datachnk.getcomprBmp().begin();
            bititer != datachnk.getcomprBmp().end(); bititer++){
                if(*bititer == true) {
                	chnkp->set_bit(bit);
                }
                else{
                	chnkp->clear_bit(bit);
                }
                bit++;
        }
							
	// 4. Next copy the entries
	//allocate space for the entries
	try{
		chnkp->entry = new DiskDataChunk::DataEntry_t[chnkp->no_ace];
	}
	catch(std::bad_alloc&){
		delete chnkp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> cant allocate space for data entries!\n");		
	}
	
	i = 0;
	vector<DataEntry>::const_iterator ent_iter = datachnk.getentry().begin();
	//ASSERTION5: combatible vector length and no of entries
	if(chnkp->no_ace != datachnk.getentry().size()){
		delete chnkp;
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> ASSERTION4: wrong length in vector\n");	
	}//end if
	while(i<chnkp->no_ace && ent_iter != datachnk.getentry().end()){
		//allocate space for the measures
         	try{
         		chnkp->entry[i].measures = new measure_t[int(chnkp->hdr.no_measures)];
         	}
         	catch(std::bad_alloc&){
         		delete chnkp;
         		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> cant allocate space for data entries!\n");
         	}	
		
         	//store the measures of this entry
       		//ASSERTION6: combatible vector length and no of measures
		if(chnkp->hdr.no_measures != ent_iter->fact.size()){
			delete chnkp;
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::dataChunk2DiskDataChunk ==> ASSERTION6: wrong length in vector\n");	         	
		}//end if
		int j=0;
		vector<measure_t>::const_iterator m_iter = ent_iter->fact.begin();
		while(j<chnkp->hdr.no_measures && m_iter != ent_iter->fact.end()){
			chnkp->entry[i].measures[j] = *m_iter;
			j++;
			m_iter++;		
		}//end while
		i++;
		ent_iter++;
	}//end while
	
	return chnkp;
}// end of AccessManagerImpl::dataChunk2DiskDataChunk

void AccessManagerImpl::placeDiskDirChunkInBcktBody(const DiskDirChunk* const chnkp, unsigned int maxDepth,
					char* &currentp, size_t& hdr_size, size_t& chnk_size) const

// precondition:
//		chnkp points at a DiskDirChunk structure && currentp is a byte pointer pointing in the
//		body of a DiskBucket (or byte vector in general) at the point, where the DiskDirChunk must be placed.maxDepth
//		gives the maximum depth of the cube in question and it is used for confirming that this
//		is a data chunk.
// postcondition:
//		the DiskDirChunk has been placed in the body && currentp points at the next free byte in
//		the body && chnk_size contains the bytes consumed by the placement of the DiskDirChunk &&
//              hdr_size contains the bytes consumed by the DiskChunkHeader.		
{
        // init size counters
        chnk_size = 0;
        hdr_size = 0;

	//ASSERTION1: input pointers are not null
	if(!chnkp || !currentp)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskDirChunkInBcktBody ==> ASSERTION1: null pointer(s)\n");

	//get a const pointer to the DiskChunkHeader
	const DiskChunkHeader* const hdrp = &chnkp->hdr;
	
	//ASSERTIONS following...
	
	//ASSERTION: this is a dir chunk
	if(!AccessManagerImpl::isDirChunk(hdrp->depth, hdrp->local_depth, hdrp->next_local_depth, maxDepth))
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskDirChunkInBcktBody ==> wrong chunk type!\n");			
			
	//Is this the root chunk?
	bool isRootChunk = AccessManagerImpl::isRootChunk(hdrp->depth, hdrp->local_depth, hdrp->next_local_depth, maxDepth);

	//Is this an artificially chunked dir chunk?
	bool isArtifChunk = AccessManagerImpl::isArtificialChunk(hdrp->local_depth);	
		
	/*	
	//ASSERTION 1.1: not out of range for depth
	if(hdrp->depth < Chunk::MIN_DEPTH || hdrp->depth > maxDepth)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskDirChunkInBcktBody ==> ASSERTION 1.1: depth out of range for a dir chunk\n");	
		
	//ASSERTION 1.2: not the root chunk
	if(hdrp->depth == Chunk::MIN_DEPTH)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskDirChunkInBcktBody ==> ASSERTION 1.2: depth denotes the root chunk!\n");	
		
	//ASSERTION 1.3: not a data chunk
	if(hdrp->depth == maxDepth)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskDirChunkInBcktBody ==> ASSERTION 1.3 depth denotes a data chunk!\n");	
	*/
				
	//begin by placing the static part of a DiskDirchunk structure
	memcpy(currentp, reinterpret_cast<char*>(chnkp), sizeof(DiskDirChunk));
	currentp += sizeof(DiskDirChunk); // move on to the next empty position
	chnk_size += sizeof(DiskDirChunk); // this is the size of the static part of a DiskDirChunk
	hdr_size += sizeof(DiskChunkHeader); // this is the size of the static part of a DiskChunkHeader

	//if this is not the root chunk
	if(!isRootChunk){			
        	//continue with placing the chunk id
        	//ASSERTION2: chunkid is not null
        	if(!hdrp->chunk_id)
        		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskDirChunkInBcktBody ==> ASSERTION2: null pointer for chunk id\n");	
        	//first store the domains of the chunk id
        	for (int i = 0; i < hdrp->depth; i++){ //for each domain of the chunk id
        	//loop invariant: a domain of the chunk id will be stored. A domain
        	// is only a pointer to an array of order codes.
        		const DiskChunkHeader::Domain_t* const dmnp = &(hdrp->chunk_id)[i];
        		memcpy(currentp, reinterpret_cast<char*>(dmnp), sizeof(DiskChunkHeader::Domain_t));
        		currentp += sizeof(DiskChunkHeader::Domain_t); // move on to the next empty position
        		hdr_size += sizeof(DiskChunkHeader::Domain_t);
        		chnk_size += sizeof(DiskChunkHeader::Domain_t);
        	}//end for		
        	//Next store the order-codes of the domains
        	for (int i = 0; i < hdrp->depth; i++){ //for each domain of the chunk id	
        		//ASSERTION3: ordercodes pointer is not null
        		if(!(hdrp->chunk_id)[i].ordercodes)
        			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskDirChunkInBcktBody ==> ASSERTION3: null pointer\n");			
        		for(int j = 0; j < hdrp->no_dims; j++) { //fore each order code of this domain
        		//loop invariant: each ordercode of the current domain will stored
        			const DiskChunkHeader::ordercode_t* const op = &((hdrp->chunk_id)[i].ordercodes[j]);
                		memcpy(currentp, reinterpret_cast<const char*>(op), sizeof(DiskChunkHeader::ordercode_t));
                		currentp += sizeof(DiskChunkHeader::ordercode_t); // move on to the next empty position
                		hdr_size += sizeof(DiskChunkHeader::ordercode_t);		
                		chnk_size += sizeof(DiskChunkHeader::ordercode_t);		
        		}//end for	
                }//end for
	}//end if
	#ifdef DEBUGGING	
	else {//this is the root chunk
        	//ASSERTION2: chunkid is null
        	if(hdrp->chunk_id)
        		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskDirChunkInBcktBody ==> ASSERTION2: not null chunk id for root chunk\n");				
	}
	#endif
		
	//next place the orcercode ranges
	//ASSERTION4: oc_range is not null
	if(!hdrp->oc_range)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskDirChunkInBcktBody ==> ASSERTION4: null pointer\n");		
	for(int i = 0; i < hdrp->no_dims; i++) {
	//loop invariant: store an order code range structure
		const DiskChunkHeader::OrderCodeRng_t* const rngp = &(hdrp->oc_range)[i];
       		memcpy(currentp, reinterpret_cast<char*>(rngp), sizeof(DiskChunkHeader::OrderCodeRng_t));
       		currentp += sizeof(DiskChunkHeader::OrderCodeRng_t); // move on to the next empty position
       		hdr_size += sizeof(DiskChunkHeader::OrderCodeRng_t);		
       		chnk_size += sizeof(DiskChunkHeader::OrderCodeRng_t);		       		
	}//end for
	
	//if this is an artificially chunked dir chunk
	if(isArtifChunk){
        	//ASSERTION: rng2oc is not null
        	if(!chnkp->rng2oc)
        		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskDirChunkInBcktBody ==> ASSERTION: null pointer for range to orde-code mapping (for an artificially chunked dir chunk)!\n");	
        	//first store the ranges-to-oc per dimension
        	for (int i = 0; i < hdrp->no_dims; i++){ //for each dimension
        	//loop invariant: a Rng2oc_t instance will be stored. This includes
        	// a pointer to an array of range to order code mappings.
        		const DiskDirChunk::Rng2oc_t* const rp = &(chnkp->rng2oc[i]);
        		memcpy(currentp, reinterpret_cast<char*>(rp), sizeof(DiskDirChunk::Rng2oc_t));
        		currentp += sizeof(DiskDirChunk::Rng2oc_t); // move on to the next empty position
        		chnk_size += sizeof(DiskDirChunk::Rng2oc_t);
        	}//end for
        	
        	//Now store for each range (i.e., per dim) the corresponding range elements
        	for (int i = 0; i < hdrp->no_dims; i++){ //for each dimension
        		//ASSERTION
        		if(!chnkp->rng2oc[i].rngElemp){
	        		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskDirChunkInBcktBody ==> null pointer for range element (for an artificially chunked dir chunk)!\n");		
        		}//end if
        		for(int j=0; j < chnkp->rng2oc[i].noMembers; j++){
        			const DiskDirChunk::Rng2ocElem_t * const relemp = &(chnkp->rng2oc[i].rngElemp[j]);
                		memcpy(currentp, reinterpret_cast<char*>(relemp), sizeof(DiskDirChunk::Rng2ocElem_t));
                		currentp += sizeof(DiskDirChunk::Rng2ocElem_t); // move on to the next empty position
                		chnk_size += sizeof(DiskDirChunk::Rng2ocElem_t);        			
        		}//end for
        	}//end for        					
	}//end if
		
	//finally place the dir entries
       	for(int i =0; i<chnkp->hdr.no_entries; i++) { //for each entry
	       	const DiskDirChunk::DirEntry_t* const ep = &chnkp->entry[i];       		
        	memcpy(currentp, reinterpret_cast<char*>(ep), sizeof(DiskDirChunk::DirEntry_t));
        	currentp += sizeof(DiskDirChunk::DirEntry_t); // move on to the next empty position
        	chnk_size += sizeof(DiskDirChunk::DirEntry_t);		       	
       	}//end for  						
}// end of AccessManagerImpl::placeDiskDirChunkInBcktBody      		

void AccessManagerImpl::placeDiskDataChunkInBcktBody(const DiskDataChunk* const chnkp, int maxDepth,
			char* &currentp, size_t& hdr_size, size_t& chnk_size)const
// precondition:
//		chnkp points at a DiskDataChunk structure && currentp is a byte pointer pointing in the
//		body of a DiskBucket at the point, where the DiskDataChunk must be placed. maxDepth
//		gives the maximum depth of the cube in question and it is used for confirming that this
//		is a data chunk.
// postcondition:
//		the DiskDataChunk has been placed in the body && currentp points at the next free byte in
//		the body && chnk_size contains the bytes consumed by the placement of the DiskDataChunk &&
//              hdr_size contains the bytes consumed by the placement of the DiskChunkHeader.
{
        // init size counters
        chnk_size = 0;
        hdr_size = 0;

	//ASSERTION1: input pointers are not null
	if(!chnkp || !currentp)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskDataChunkInBcktBody ==> ASSERTION1: null pointer\n");

	//get a const pointer to the DiskChunkHeader
	const DiskChunkHeader* const hdrp = &chnkp->hdr;
	
	//ASSERTION 1.1: this is a data chunk
//	if(int(hdrp->depth) != maxDepth)
//		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskDataChunkInBcktBody ==> ASSERTION 1.1: chunk's depth != maxDepth in Data Chunk\n");
	//ASSERTION: this is a data chunk
	if(!AccessManagerImpl::isDataChunk(hdrp->depth, hdrp->local_depth, hdrp->next_local_depth, maxDepth))
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskDataChunkInBcktBody ==> wrong chunk type!\n");			

		
	//begin by placing the static part of a DiskDatachunk structure
	memcpy(currentp, reinterpret_cast<char*>(chnkp), sizeof(DiskDataChunk));
	currentp += sizeof(DiskDataChunk); // move on to the next empty position
	chnk_size += sizeof(DiskDataChunk); // this is the size of the static part of a DiskDataChunk
	hdr_size += sizeof(DiskChunkHeader); // this is the size of the static part of a DiskChunkHeader
			
	//continue with placing the chunk id
	//ASSERTION2: chunkid is not null
	if(!hdrp->chunk_id)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskDataChunkInBcktBody ==> ASSERTION2: null pointer\n");	
	//first store the domains of the chunk id
	for (int i = 0; i < hdrp->depth; i++){ //for each domain of the chunk id
	//loop invariant: a domain of the chunk id will be stored. A domain
	// is only a pointer to an array of order codes.
		DiskChunkHeader::Domain_t* dmnp = &(hdrp->chunk_id)[i];
		memcpy(currentp, reinterpret_cast<char*>(dmnp), sizeof(DiskChunkHeader::Domain_t));
		currentp += sizeof(DiskChunkHeader::Domain_t); // move on to the next empty position
		hdr_size += sizeof(DiskChunkHeader::Domain_t);
		chnk_size += sizeof(DiskChunkHeader::Domain_t);
	}//end for		
	//Next store the order-codes of the domains
	for (int i = 0; i < hdrp->depth; i++){ //for each domain of the chunk id	
		//ASSERTION3: ordercodes pointer is not null
		if(!(hdrp->chunk_id)[i].ordercodes)
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskDataChunkInBcktBody ==> ASSERTION3: null pointer\n");			
		for(int j = 0; j < hdrp->no_dims; j++) { //fore each order code of this domain
		//loop invariant: each ordercode of the current domain will stored
			DiskChunkHeader::ordercode_t* op = &((hdrp->chunk_id)[i].ordercodes[j]);
        		memcpy(currentp, reinterpret_cast<char*>(op), sizeof(DiskChunkHeader::ordercode_t));
        		currentp += sizeof(DiskChunkHeader::ordercode_t); // move on to the next empty position
        		hdr_size += sizeof(DiskChunkHeader::ordercode_t);		
        		chnk_size += sizeof(DiskChunkHeader::ordercode_t);		
		}//end for	
        }//end for
		
	//next place the orcercode ranges
	//ASSERTION4: oc_range is not null
	if(!hdrp->oc_range)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskDataChunkInBcktBody ==> ASSERTION4: null pointer\n");		
	for(int i = 0; i < hdrp->no_dims; i++) {
	//loop invariant: store an order code range structure
		DiskChunkHeader::OrderCodeRng_t* rngp = &(hdrp->oc_range)[i];
       		memcpy(currentp, reinterpret_cast<char*>(rngp), sizeof(DiskChunkHeader::OrderCodeRng_t));
       		currentp += sizeof(DiskChunkHeader::OrderCodeRng_t); // move on to the next empty position
       		hdr_size += sizeof(DiskChunkHeader::OrderCodeRng_t);		
       		chnk_size += sizeof(DiskChunkHeader::OrderCodeRng_t);		       		
	}//end for
	
	//next place the bitmap (i.e., array of WORDS)
  	for(int b=0; b<bmp::numOfWords(hdrp->no_entries); b++){
		bmp::WORD* wp = &chnkp->bitmap[b];
		memcpy(currentp, reinterpret_cast<char*>(wp), sizeof(bmp::WORD));
	       	currentp += sizeof(bmp::WORD); // move on to the next empty position
       		chnk_size += sizeof(bmp::WORD);       		
  	}//end for
  	
       	//Now, place the DataEntry_t structures
       	for(int i=0; i<chnkp->no_ace; i++){
        	DiskDataChunk::DataEntry_t* ep = &chnkp->entry[i];
             	memcpy(currentp, reinterpret_cast<char*>(ep), sizeof(DiskDataChunk::DataEntry_t));
             	currentp += sizeof(DiskDataChunk::DataEntry_t); // move on to the next empty position
             	chnk_size += sizeof(DiskDataChunk::DataEntry_t);		       	
       	}//end for

       	//Finally place the measure values
       	// for each data entry
       	for(int i=0; i<chnkp->no_ace; i++){
       		// for each measure of this entry
               	for(int m=0; m<hdrp->no_measures; m++){
                	measure_t* mp = &chnkp->entry[i].measures[m];
                     	memcpy(currentp, reinterpret_cast<char*>(mp), sizeof(measure_t));
                     	currentp += sizeof(measure_t); // move on to the next empty position
                     	chnk_size += sizeof(measure_t);		       	
                }//end for
       	}//end for       	       	
}// end of AccessManagerImpl::placeDiskDataChunkInBcktBody      		

/*
void AccessManagerImpl::placeDiskChunkHdrInBody(const DiskChunkHeader* const hdrp, char* currentp, size_t& hdr_size)
// precondition:
//		hdrp points at a DiskChunkHeader structure && currentp is a byte pointer pointing in the
//		body of a DiskBucket at the point, where the DiskChunkHeader must be placed.
// postcondition:
//		the DiskChunkHeader has been placed in the body && currentp points at the next free byte in
//		the body && hdr_size contains the bytes consumed by the placement of the header
{
	//ASSERTION1: chnkp points at a DiskDirChunk
	if(!hdrp)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskChunkHdrInBody ==> ASSERTION1: null pointer\n");

	// first store the static-size part		
	memcpy(currentp, reinterpret_cast<char*>(hdrp), sizeof(DiskChunkHeader));
	currentp += sizeof(DiskChunkHeader); // move on to the next empty position
	hdr_size += sizeof(DiskChunkHeader);
	
	// Next, store the chunk id
	//ASSERTION2: chunkid is not null
	if(!hdrp->chunk_id)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskChunkHdrInBody ==> ASSERTION2: null pointer\n");	
	//first store the domains of the chunk id
	for (int i = 0; i < hdrp->depth; i++){ //for each domain of the chunk id
	//loop invariant: a domain of the chunk id will be stored. A domain
	// is only a pointer to an array of order codes.
		DiskChunkHeader::Domain_t* dmnp = &hdrp->chunk_id[i];
		memcpy(currentp, reinterpret_cast<char*>(dmnp), sizeof(DiskChunkHeader::Domain_t));
		currentp += sizeof(DiskChunkHeader::Domain_t); // move on to the next empty position
		hdr_size += sizeof(DiskChunkHeader::Domain_t);
	}//end for		
	//Next store the order-codes of the domains
	for (int i = 0; i < hdrp->depth; i++){ //for each domain of the chunk id	
		//ASSERTION3: ordercodes pointer is not null
		if(!hdrp->chunk_id[i]->ordercodes)
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskChunkHdrInBody ==> ASSERTION3: null pointer\n");			
		for(int j = 0; j < hdrp->no_dims; j++) { //fore each order code of this domain
		//loop invariant: each ordercode of the current domain will stored
			DiskChunkHeader::ordercode_t* op = &(hdrp->chunk_id[i]->ordercodes[j]);
        		memcpy(currentp, reinterpret_cast<char*>(op), sizeof(DiskChunkHeader::ordercode_t));
        		currentp += sizeof(DiskChunkHeader::ordercode_t); // move on to the next empty position
        		hdr_size += sizeof(DiskChunkHeader::ordercode_t);		
		}//end for	
        }//end for
	
	//next store the order code ranges
	//ASSERTION4: oc_range is not null
	if(!hdrp->oc_range)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::placeDiskChunkHdrInBody ==> ASSERTION4: null pointer\n");		
	for(int i = 0; i < hdrp->no_dims;; i++) {
	//loop invariant: store an order code range structure
		DiskChunkHeader::OrderCodeRng_t* rngp = &hdrp->oc_range[i];
       		memcpy(currentp, reinterpret_cast<char*>(rngp), sizeof(DiskChunkHeader::OrderCodeRng_t));
       		currentp += sizeof(DiskChunkHeader::OrderCodeRng_t); // move on to the next empty position
       		hdr_size += sizeof(DiskChunkHeader::OrderCodeRng_t);		
	}//end for
}// end AccessManagerImpl::placeDiskChunkHdrInBody
*/

void AccessManagerImpl::descendBreadth1stCostTree(
				 unsigned int maxDepth,
				 unsigned int numFacts,
				 const CostNode* const costRoot,
				 const string& factFile,
				 const BucketID& bcktID,
				 queue<CostNode*>& nextToVisit,
				 vector<DirChunk>* const dirVectp,				
				 vector<DataChunk>* const dataVectp)const
// precondition:
//	costRoot points at a tree, where BUCKET_THRESHOLD<= tree-size <= DiskBucket::bodysize
// postcondition:
//	The dir chunks and the data chunks of this tree have been filled with values
//	and are stored in heap space in two vectors: dirVectp and dataVectp, in the following manner:
//	We descend in a breadth first manner and we store each node (Chunk) as we first visit it.				
{
        //ASSERTION1: non-empty sub-tree
	if(!costRoot) {
	 	//empty sub-tree
	 	throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendBreadth1stCostTree ==>ASSERTION1: Emtpy sub-tree!!\n");
	}
	
	//case I: this corresponds to a directory chunk
	//if(costRoot->getchunkHdrp()->depth < maxDepth){
	if(isDirChunk(costRoot->getchunkHdrp()->depth, costRoot->getchunkHdrp()->localDepth, costRoot->getchunkHdrp()->nextLocalDepth, maxDepth)){	
                //Target: create the dir chunk corresponding to the current node

         	//NOTE:  an artificially chunked dir chunk cannot appear here
         	//because these are all stored in the root bucket. ONLY artificially chunked DATA chunks may appear.
	        //ASSERTION
	        if(isArtificialChunk(costRoot->getchunkHdrp()->localDepth))
		        throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendBreadth1stCostTree  ==> artificial chunk encountered!\n");

                //allocate entry vector where size = total no of cells
	     	//      NOTE: the default DirEntry constructor should initialize the member BucketID
	     	//	with a BucketID::null constant.
	     	vector<DirEntry> entryVect(costRoot->getchunkHdrp()->totNumCells);
	     	
                //for each child of this node
                int childAscNo = 0;
                //The current chunk will be stored at chunk-slot = dirVectp->size()
                //The current chunk's children will be stored at
                //chunk slot = current node's chunk slot + size of queue +ascending
                //number of child (starting from 1)			
                int indexBase = dirVectp->size() + nextToVisit.size();
                for(vector<CostNode*>::const_iterator ichild = costRoot->getchild().begin();
                    ichild != costRoot->getchild().end(); ichild++) {
                        childAscNo++; //one more child
                        //create corresponding entry in current chunk
                        DirEntry e;
			e.bcktId = bcktID; //store the bucket id where the child chunk will be stored
                        //store the chunk slot in the bucket where the chunk will be stored
                        e.chnkIndex = indexBase + childAscNo;
			
			// insert entry at entryVect in the right offset (calculated from the Chunk Id)
			Coordinates c;
			(*ichild)->getchunkHdrp()->id.extractCoords(c);
			//ASSERTION2
			unsigned int offs = DirChunk::calcCellOffset(c, costRoot->getchunkHdrp()->vectRange);
       			if(offs >= entryVect.size())
       				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendBreadth1stCostTree ==>ASSERTION2: entryVect out of range!\n");
                        //store  entry in the entry vector
			entryVect[offs] = e;

                        //push child node pointer into queue in order to visit it later
                        nextToVisit.push(*ichild);
                }//end for

                //create dir chunk instance
		DirChunk newChunk(*(costRoot->getchunkHdrp()), entryVect);

	     	// store new dir chunk in the corresponding vector
	     	dirVectp->push_back(newChunk);

                //ASSERTION3: assert that queue is not empty at this point
                if(nextToVisit.empty())
                        throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendBreadth1stCostTree ==>ASSERTION3: queue cannot be empty at this point!\n");
                //pop next node from the queue and visit it
                CostNode* next = nextToVisit.front();
                nextToVisit.pop(); // remove pointer from queue
               	try {
               		descendBreadth1stCostTree(
               				maxDepth,
               				numFacts,
               		                next,               				
               				factFile,
               				bcktID,
               				nextToVisit,
               				dirVectp,
               				dataVectp);
               	}
               	catch(GeneralError& error) {
               		GeneralError e("AccessManagerImpl::descendBreadth1stCostTree  ==> ");
               		error += e;
               		throw error;
               	}
               	//ASSERTION4
               	if(dirVectp->empty())
                        throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendBreadth1stCostTree  ==>ASSERTION4: empty chunk vectors (possibly both)!!\n");
        }//end if
	else{	//case II: this corresponds to a data chunk
                //Target: create the data chunk corresponding to the current node

		//ASSERTION5: depth check
		//if(costRoot->getchunkHdrp()->depth != maxDepth)
		if(!isDataChunk(costRoot->getchunkHdrp()->depth, costRoot->getchunkHdrp()->localDepth, costRoot->getchunkHdrp()->nextLocalDepth, maxDepth))
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendBreadth1stCostTree ==>ASSERTION5: Wrong chunk type: data chunk expected!\n");

               // allocate entry vector where size = real number of cells
	     	vector<DataEntry> entryVect(costRoot->getchunkHdrp()->rlNumCells);
               // allocate compression bitmap where size = total number of cells (initialized to 0's).	     	
	     	deque<bool> cmprBmp(costRoot->getchunkHdrp()->totNumCells, false);
	     	
	     	//if this is an artificially chunked data chunk then
	     	vector<ChunkID> dataPointsArtifChunkVect;
	     	if(isArtificialChunk(costRoot->getchunkHdrp()->localDepth)){
	     		if(costRoot->getcMapp()->empty())
		     		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendBreadth1stCostTree ==>ASSERTION: empty cell map in artificial data chunk\n");
	     		//use chunk's cellmap to create a vector with the existing data points. This vector
	     		//will contain the corresponding chunk id with the extra (i.e., artificial) domains REMOVED.
	     		//Because in the input file data points DONT have these extra domains!
	     		dataPointsArtifChunkVect.reserve(costRoot->getcMapp()->getchunkidVectp()->size());
	     		try{
	     			removeArtificialDomainsFromCellChunkIDs(costRoot->getchunkHdrp()->localDepth - Chunk::MIN_DEPTH, *(costRoot->getcMapp()->getchunkidVectp()),dataPointsArtifChunkVect);
	     		}
                	catch(GeneralError& error) {
                		GeneralError e("AccessManagerImpl::descendBreadth1stCostTree  ==> ");
                		error += e;
                		throw error;
                	}	     		
	     	}//end if

	     	// Fill in those entries
        	// open input file for reading
        	ifstream input(factFile.c_str());
        	if(!input)
        		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendDepth1stCostTree ==> Error in creating ifstream obj\n");

        	string buffer;
        	// skip all schema staff and get to the fact values section
        	do{
        		input >> buffer;
        	}while(buffer != "VALUES_START");

        	// read on until you find prefix corresponding to current data chunk
        	string prefix = costRoot->getchunkHdrp()->id.getcid();
        	do {
	        	input >> buffer;
	        	if(input.eof())
	        		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendDepth1stCostTree ==> Can't find prefix in input file\n");
	        }while(buffer.find(prefix) != 0);

	        // now, we 've got a prefix match.
	        // Read the fact values for the non-empty cells of this chunk.
	        unsigned int factsPerCell = numFacts;
	        vector<measure_t> factv(factsPerCell);
		map<ChunkID, DataEntry> helpmap; // for temporary storage of entries
		int numCellsRead = 0;
	        do {
	        // loop invariant: read a line from fact file, containing the values of
		//     		   a single cell. All cells belong to chunk with id prefix
		
			//if this is an artifically chunked data chunk
			if(isArtificialChunk(costRoot->getchunkHdrp()->localDepth)){
				//look if the current cell belongs in the vector of data points of this chunk
				ChunkID currId(buffer);
				vector<ChunkID>::iterator result = find(dataPointsArtifChunkVect.begin(), dataPointsArtifChunkVect.end(), currId);
				// if did not found
				if(result == dataPointsArtifChunkVect.end()){					
					//read next cell id (i.e. chunk  id)
			        	input >> buffer;
					//and continue loop
					continue;
				}//end if
			}//end if
			
			for(int i = 0; i<factsPerCell; ++i){
				input >> factv[i];
			}

			DataEntry e(factsPerCell, factv);
			//Offset in chunk can't be computed until bitmap is created. Store
			// the entry temporarily in this map container, by chunkid
			ChunkID cellid(buffer);

			//ASSERTION6: no such id already exists in the map
			if(helpmap.find(cellid) != helpmap.end())
                                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendBreadth1stCostTree ==>ASSERTION6: double entry for cell in fact load file\n");
			helpmap[cellid] = e;

			// insert entry at cmprBmp in the right offset (calculated from the Chunk Id)
			Coordinates c;
			cellid.extractCoords(c);
			//ASSERTION7
			unsigned int offs = DirChunk::calcCellOffset(c, costRoot->getchunkHdrp()->vectRange);
       			if(offs >= cmprBmp.size()){
       				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendBreadth1stCostTree ==>ASSERTION7: cmprBmp out of range!\n");
       			}
			cmprBmp[offs] = true; //this cell is non-empty

			numCellsRead++;

	        	//read next cell id (i.e. chunk  id)
	        	input >> buffer;
	        } while(buffer.find(prefix) == 0 && !input.eof()); // we are still under the same prefix
	                                                           // (i.e. data chunk)
		input.close();

		//ASSERTION8: number of non-empty cells read
		if(numCellsRead != costRoot->getchunkHdrp()->rlNumCells)
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendBreadth1stCostTree ==>ASSERTION8: Wrong number of non-empty cells\n");

		// now store into the entry vector of the data chunk
		for(map<ChunkID, DataEntry>::const_iterator map_i = helpmap.begin();
		    map_i != helpmap.end(); ++map_i) {
        		// insert entry at entryVect in the right offset (calculated from the Chunk Id)
        		Coordinates c;
        		map_i->first.extractCoords(c);

        		bool emptyCell = false;
        		unsigned int offs = DataChunk::calcCellOffset(c, cmprBmp,
        		                                             costRoot->getchunkHdrp()->vectRange, emptyCell);
        		//ASSERTION9: offset within range
       			if(offs >= entryVect.size())
       				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendBreadth1stCostTree ==>ASSERTION9: (DataChunk) entryVect out of range!\n");
    			
       			//ASSERTION10: non-empty cell
       			if(emptyCell)
               			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendBreadth1stCostTree ==>ASSERTION10: access to empty cell!\n");
        		//store entry in vector
        		entryVect[offs] = map_i->second;		    		
		}//end for		
		
	     	// Now that entryVect and cmprBmp are filled, create dataChunk object
		DataChunk newChunk(*(costRoot->getchunkHdrp()), cmprBmp, entryVect);	     	

	     	// Store new chunk in the corresponding vector
	     	dataVectp->push_back(newChunk);		

                //if the queue is not empty at this point
                if(!nextToVisit.empty()){
                        //pop next node from the queue and visit it
                        CostNode* next = nextToVisit.front();
                        nextToVisit.pop(); // remove pointer from queue
                	try {
                		descendBreadth1stCostTree(
                				maxDepth,
                				numFacts,
                		                next,
                				factFile,
                				bcktID,
                				nextToVisit,
                				dirVectp,
                				dataVectp);
                	}
                	catch(GeneralError& error) {
                		GeneralError e("");
                		error += e;
                		throw error;
                	}
                	//ASSERTION11
                	if(dirVectp->empty() || dataVectp->empty())
                                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendBreadth1stCostTree  ==>ASSERTION11: empty chunk vectors (possibly both)!!\n");
                }//end if
        }//end else
}//AccessManagerImpl::descendBreadth1stCostTree()


void AccessManagerImpl::descendDepth1stCostTree(
				 unsigned int maxDepth,
				 unsigned int numFacts,
				 const CostNode* const costRoot,
				 const string& factFile,
				 const BucketID& bcktID,
				 vector<DirChunk>* const dirVectp,
				 vector<DataChunk>* const dataVectp)const
// precondition:
//	costRoot points at a tree, where BUCKET_THRESHOLD<= tree-size <= DiskBucket::bodysize
// postcondition:
//	The dir chunks and the data chunks of this tree have been filled with values
//	and are stored in heap space in two vectors: dirVectp and dataVectp, in the following manner:
//	We descend in a depth first manner and we store each node (Chunk) as we first visit it.
{
	if(!costRoot) {
	 	//empty sub-tree
	 	throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendDepth1stCostTree  ==> Emtpy sub-tree!!\n");
	}

	// I. If this is not a leaf chunk (i.e., is a dir chunk)
	//if(costRoot->getchunkHdrp()->depth < maxDepth){
	if(isDirChunk(costRoot->getchunkHdrp()->depth, costRoot->getchunkHdrp()->localDepth, costRoot->getchunkHdrp()->nextLocalDepth, maxDepth)){
	
		//NOTE:  an artificially chunked dir chunk cannot appear here
		//because these are all stored in the root bucket. ONLY artificially chunked DATA chunks may appear.		
	        //ASSERTION
	        if(isArtificialChunk(costRoot->getchunkHdrp()->localDepth))
		        throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendDepth1stCostTree  ==> artificial chunk encountered!\n");
	        		
	     	// 1. Create the vector holding the chunk's entries.
	     	//      NOTE: the default DirEntry constructor should initialize the member BucketID
	     	//	with a BucketID::null constant.
	     	vector<DirEntry> entryVect(costRoot->getchunkHdrp()->totNumCells);

	     	// 2. Fill in those entries
	     	//    the current chunk (costRoot) will be stored in
	     	//    chunk slot == dirVectp->size()+dataVectp->size()
	     	//   the 1st child will be stored just after the current chunk
	     	unsigned int index = dirVectp->size() + dataVectp->size() + 1;
		vector<ChunkID>::const_iterator icid = costRoot->getcMapp()->getchunkidVectp()->begin();
		vector<CostNode*>::const_iterator ichild = costRoot->getchild().begin();
		// ASSERTION1: size of CostNode vectors is equal
		if(costRoot->getchild().size() != costRoot->getcMapp()->getchunkidVectp()->size())
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendDepth1stCostTree ==> ASSERTION1: != vector sizes\n");
		while(ichild != costRoot->getchild().end() && icid != costRoot->getcMapp()->getchunkidVectp()->end()){
			// loop invariant: for current entry e holds:
			//		  e.bcktId == the same for all entries
			//		  e.chnkIndex == the next available chunk slot in the bucket
			//			after all the chunks of the sub-trees hanging from
			//			the previously inserted entries.
			DirEntry e;
			e.bcktId = bcktID;
                        e.chnkIndex = index;
			// insert entry at entryVect in the right offset (calculated from the Chunk Id)
			Coordinates c;
			(*icid).extractCoords(c);
			//ASSERTION2
			unsigned int offs = DirChunk::calcCellOffset(c, costRoot->getchunkHdrp()->vectRange);
       			if(offs >= entryVect.size()){
       				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendDepth1stCostTree ==>ASSERTION2: entryVect out of range!\n");
       			}
			entryVect[offs] = e;
                        unsigned int numChunks = 0;
                	CostNode::countChunksOfTree(*ichild, maxDepth, numChunks);
			index = index + numChunks;			
			ichild++;
			icid++;
		} //end while

	     	// 3. Now that entryVect is filled, create dirchunk object
		DirChunk newChunk(*costRoot->getchunkHdrp(), entryVect);

	     	// 4. Store new chunk in the corresponding vector
	     	dirVectp->push_back(newChunk);

	     	// 5. Descend to children
	     	for(vector<CostNode*>::const_iterator ichd = costRoot->getchild().begin();
	     	    ichd != costRoot->getchild().end(); ++ichd) {
                	try {
                		descendDepth1stCostTree(
                				maxDepth,
                				numFacts,
                				(*ichd),
                				factFile,
                				bcktID,
                				dirVectp,
                				dataVectp);
                	}
                      	catch(GeneralError& error) {
                      		GeneralError e("AccessManagerImpl::descendDepth1stCostTree ==> ");
                      		error += e;
                      		throw error;
                      	}
                      	//ASSERTION3
                       	if(dirVectp->empty() || dataVectp->empty()){
                       			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendDepth1stCostTree  ==>ASSERTION3: empty chunk vectors (possibly both)!!\n");
                       	}
	     	} // end for
	} //end if
	else { // II. this is a leaf chunk
		//ASSERTION4: depth check
		//if(costRoot->getchunkHdrp()->depth != maxDepth)
		if(!isDataChunk(costRoot->getchunkHdrp()->depth, costRoot->getchunkHdrp()->localDepth, costRoot->getchunkHdrp()->nextLocalDepth, maxDepth))
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendDepth1stCostTree ==>ASSERTION5: Wrong chunk type: data chunk expected!\n");		
			
	     	//if this is an artificially chunked data chunk then
	     	vector<ChunkID> dataPointsArtifChunkVect;
	     	if(isArtificialChunk(costRoot->getchunkHdrp()->localDepth)){
	     		if(costRoot->getcMapp()->empty())
		     		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendDepth1stCostTree ==>ASSERTION: empty cell map in artificial data chunk\n");
	     		//use chunk's cellmap to create a vector with the existing data points. This vector
	     		//will contain the corresponding chunk id with the extra (i.e., artificial) domains REMOVED.
	     		//Because in the input file data points DONT have these extra domains!
	     		dataPointsArtifChunkVect.reserve(costRoot->getcMapp()->getchunkidVectp()->size());
	     		try{
	     			removeArtificialDomainsFromCellChunkIDs(costRoot->getchunkHdrp()->localDepth - Chunk::MIN_DEPTH, *(costRoot->getcMapp()->getchunkidVectp()),dataPointsArtifChunkVect);
	     		}
                	catch(GeneralError& error) {
                		GeneralError e("AccessManagerImpl::descendDepth1stCostTree  ==> ");
                		error += e;
                		throw error;
                	}	     		
	     	}//end if
			
		// 1. Create the vector holding the chunk's entries and the compression bitmap
		//    (initialized to 0's).
	     	vector<DataEntry> entryVect(costRoot->getchunkHdrp()->rlNumCells);
	     	deque<bool> cmprBmp(costRoot->getchunkHdrp()->totNumCells, false);

	     	// 2. Fill in those entries
        	// open input file for reading
        	ifstream input(factFile.c_str());
        	if(!input)
        		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendDepth1stCostTree ==> Error in creating ifstream obj\n");

        	string buffer;
        	// skip all schema staff and get to the fact values section
        	do{
        		input >> buffer;
        	}while(buffer != "VALUES_START");

        	// read on until you find prefix corresponding to current data chunk
        	string prefix = costRoot->getchunkHdrp()->id.getcid();
        	do {
	        	input >> buffer;
	        	if(input.eof())
	        		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendDepth1stCostTree ==> Can't find prefix in input file\n");
	        }while(buffer.find(prefix) != 0);

	        // now, we 've got a prefix match.
	        // Read the fact values for the non-empty cells of this chunk.
	        unsigned int factsPerCell = numFacts;
	        vector<measure_t> factv(factsPerCell);
		map<ChunkID, DataEntry> helpmap; // for temporary storage of entries
		int numCellsRead = 0;
	        do {
	        // loop invariant: read a line from fact file, containing the values of
		//     		   a single cell. All cells belong to chunk with id prefix
		
			//if this is an artifically chunked data chunk
			if(isArtificialChunk(costRoot->getchunkHdrp()->localDepth)){
				//look if the current cell belongs in the vector of data points of this chunk
				ChunkID currId(buffer);
				vector<ChunkID>::iterator result = find(dataPointsArtifChunkVect.begin(), dataPointsArtifChunkVect.end(), currId);
				// if did not found
				if(result == dataPointsArtifChunkVect.end()){					
					//read next cell id (i.e. chunk  id)
			        	input >> buffer;
					//and continue loop
					continue;
				}//end if
			}//end if
		
			for(int i = 0; i<factsPerCell; ++i){
				input >> factv[i];
			}

			DataEntry e(factsPerCell, factv);
			//Offset in chunk can't be computed until bitmap is created. Store
			// the entry temporarily in this map container, by chunkid
			ChunkID cellid(buffer);

			//ASSERTION5: no such id already exists in the map
			if(helpmap.find(cellid) != helpmap.end())
                                throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendDepth1stCostTree ==>ASSERTION5: double entry for cell in fact load file\n");
			helpmap[cellid] = e;

			// insert entry at cmprBmp in the right offset (calculated from the Chunk Id)
			Coordinates c;
			cellid.extractCoords(c);
			//ASSERTION6
			unsigned int offs = DirChunk::calcCellOffset(c, costRoot->getchunkHdrp()->vectRange);
       			if(offs >= cmprBmp.size()){
       				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendDepth1stCostTree ==>ASSERTION6: cmprBmp out of range!\n");
       			}
			cmprBmp[offs] = true; //this cell is non-empty

			numCellsRead++;

	        	//read next cell id (i.e. chunk  id)
	        	input >> buffer;
	        } while(buffer.find(prefix) == 0 && !input.eof()); // we are still under the same prefix
	                                                           // (i.e. data chunk)
		input.close();

		//ASSERTION7: number of non-empty cells read
		if(numCellsRead != costRoot->getchunkHdrp()->rlNumCells)
			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendDepth1stCostTree ==>ASSERTION7: Wrong number of non-empty cells\n");

		// now store into the entry vector of the data chunk
		for(map<ChunkID, DataEntry>::const_iterator map_i = helpmap.begin();
		    map_i != helpmap.end(); ++map_i) {
        		// insert entry at entryVect in the right offset (calculated from the Chunk Id)
        		Coordinates c;
        		map_i->first.extractCoords(c);

        		bool emptyCell = false;
        		unsigned int offs = DataChunk::calcCellOffset(c, cmprBmp,
        		                                             costRoot->getchunkHdrp()->vectRange, emptyCell);
        		//ASSERTION8: offset within range
       			if(offs >= entryVect.size())
       				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendDepth1stCostTree ==>ASSERTION8: (DataChunk) entryVect out of range!\n");
    			
       			//ASSERTION9: non-empty cell
       			if(emptyCell)
               			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::descendDepth1stCostTree ==>ASSERTION9: access to empty cell!\n");
        		//store entry in vector
        		entryVect[offs] = map_i->second;		    		
		}//end for		
		
	     	// 3. Now that entryVect and cmprBmp are filled, create dataChunk object
		DataChunk newChunk(*(costRoot->getchunkHdrp()), cmprBmp, entryVect);	     	

	     	// 4. Store new chunk in the corresponding vector
	     	dataVectp->push_back(newChunk);		
	} // end else		
}// end of AccessManagerImpl::descendDepth1stCostTree

void AccessManagerImpl::removeArtificialDomainsFromCellChunkIDs(int nodoms, const vector<ChunkID>& inputVect,vector<ChunkID>& outputVect)const
//precondition:
//		nodoms > 0, inputVect non empty
//processing
//		remove domains form chunk ids and insert old ids without the removed part into outputVect
//postcondition:
//		outputVect.size() == inputVect.size()
{
	//ASSERTION: valid nodoms
	if(nodoms <= 0)
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::removeArtificialDomainsFromChunkIDs ==>ASSERTION: requested No of removed domains <= 0!");	

	//for each input chunk id
	for(vector<ChunkID>::const_iterator inpID = inputVect.begin(); inpID != inputVect.end(); inpID++){
      		//ASSERTION: not too many domains will be removed: For a data chunk minimum domains that can be left after removal is 2		
      		if(inpID->getNumDomains() - nodoms < 2)
      			throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::removeArtificialDomainsFromChunkIDs ==>ASSERTION: too many number of domains are to be removed from data chunk id!");

                //get a copy of current chunk id
                string copyid = inpID->getcid();			        			      			      			
        			
      		//get position of last dot
      		string::size_type last_dot_pos = copyid.rfind("."); //find last "."
               	if (last_dot_pos == string::npos) //then no "." found.
               		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::removeArtificialDomainsFromChunkIDs ==>ASSERTION: only one domain found, no interemediate domains to remove!");
      		//get end of substr to remove
      		string::size_type toRemoveEnd = last_dot_pos - 1;         		
               	//get beginning of substr to remove
               	string::size_type toRemoveBegin;
               	string::size_type pos = toRemoveEnd;
               	int counter = nodoms;
               	while(counter){
      			pos = copyid.rfind(".", pos); //get next "." moving backwards from end to begin
      			if (pos == string::npos) //then no "." found.			
      				throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::removeArtificialDomainsFromChunkIDs ==>ASSERTION: cant find domains to remove!!!");
      			counter--;
      			pos -= 1; //go one before current dot         	
               	}//end while
               	// now pos is 2 characters before the beginning of the substr to remove
               	toRemoveBegin = pos + 1; // move only one character ahead in order to also remove one of the two dots        			
         	
		//remove substr
		copyid.erase(toRemoveBegin, toRemoveEnd - toRemoveBegin + 1);
		
		//insert in output vector
		outputVect.push_back(ChunkID(copyid));
	}//end for
	
	//ASSERTION
	if(inputVect.size() != outputVect.size())
		throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::removeArtificialDomainsFromChunkIDs ==>ASSERTION: outputVect has a wrong number of elements!");	
}//AccessManagerImpl::removeArtificialDomainsFromCellChunkIDs()
			
void AccessManagerImpl::test_construction_phaseI(CostNode* costRoot, CubeInfo& cinfo)const
{
	ofstream out("construction_phase_I.dbug");

        if (!out)
        {
            cerr << "creating file \"construction_phase_I.dbug\" failed\n";
            throw GeneralError(__FILE__, __LINE__, "AccessManagerImpl::test_construction_phaseI ==> creating file \"construction_phase_I.dbug\" failed\n");
        }

        out <<"**************************************************"<<endl;	
        out <<"*	    CHUNK TREE INFORMATION              *"<<endl;
        out <<"**************************************************"<<endl;	
        out <<"\n\n";

        // Start traversing the tree..
        CostNode::printTree(costRoot, cinfo.getmaxDepth(), out);

        // Calculate total chunk tree size
        unsigned int szBytes = 0;
        unsigned int szPages = 0;
        CostNode::calcTreeSize(costRoot, szBytes); //, szPages);
        szPages = int(ceil(float(szBytes)/float(PAGESIZE)));

        // calculate FULL cube size
        Dimension_Level level;
        unsigned int szBytesFullCb = 1;
    	for (
		vector<Dimension>::iterator iter_dim = const_cast<vector<Dimension>&>(cinfo.getvectDim()).begin();
		iter_dim != cinfo.getvectDim().end();
		++iter_dim)
	{
		// get most detailed level from each dimension
		level = (*iter_dim).get_vectLevel().back();
                #ifdef DEBUGGING
                      cerr<<"AccessManagerImpl::test_construction_phaseI ==> Most detailed level of dim "<<(*iter_dim).get_name()<<" is "<<level.get_name()<<endl;
                      cerr<<"With #members : "<<level.get_num_of_members()<<endl;
                #endif
         	szBytesFullCb *= level.get_num_of_members(); //cartesian product of grain members!
	}
	int entry_size = cinfo.getnumFacts() * sizeof(measure_t);		
	szBytesFullCb *= entry_size;
	
        out<<"\n\n******************************************"<<endl;
        out<<    " TOTAL SIZE (bytes) : "<<szBytes<<endl;
        out<<    " TOTAL SIZE (MINIMUM #disk pages) : "<<szPages<<endl;
        out<<    " TOTAL SIZE FULL CUBE (bytes) : "<<szBytesFullCb<<endl;
        out<<    "******************************************"<<endl;

} // end of AccessManagerImpl::test_construction_phaseI.

//---------------------------- end of AccessManagerImpl -----------------------------------------//

// --------------------------- class CostNode -----------------------------------------------//
CostNode::~CostNode(){
	if(chunkHdrp) delete chunkHdrp;
	if(cMapp) delete cMapp;
	//delete children pointers
	/*
	for (	vector<CostNode*>::iterator iter = child.begin();
		iter != child.end();
		++iter )
	{
       		delete (*iter);
	}
	*/
	//Done like in Effective STL, item 7
	for_each(child.begin(), child.end(), DeleteObject());
}
/*
CostNode::CostNode(const CostNode& cnode): child(cnode.getchild())
{
	if(cnode.getchunkHdrp())
		chunkHdrp = new ChunkHeader(*(cnode.getchunkHdrp()));
	else
		chunkHdrp = 0;
	if(cnode.getcMapp())
		cMapp = new CellMap (*(cnode.getcMapp()));	
	else
		cMapp = 0;
}
*/
CostNode::CostNode(ChunkHeader* const hdr, CellMap* const map)
{
	if(hdr)
		chunkHdrp = new ChunkHeader(*hdr);
	else
		chunkHdrp = 0;
		
	if(map)
		cMapp = new CellMap (*map);	
	else
		cMapp = 0;
}

CostNode::CostNode(ChunkHeader* const hdr): cMapp(0)
{
	if(hdr)
		chunkHdrp = new ChunkHeader(*hdr);
	else
		chunkHdrp = 0;

}
/*
CostNode & CostNode::operator=(const CostNode & other)
{
	if(this != &other) {
		// deallocate current data
		if(chunkHdrp) delete chunkHdrp;
		if(cMapp) delete cMapp;
		//delete child;
//    		for (
//			vector<CostNode*>::iterator iter = child.begin();
//			iter != child.end();
//			++iter
//		)
//		{
//         		delete (*iter);
//		}
		//Done like in Effective STL, item 7
		for_each(child.begin(), child.end(), DeleteObject());

		// duplicate other's data
         	if(other.getchunkHdrp())
         		chunkHdrp = new ChunkHeader(*(other.getchunkHdrp()));
         	else
         		chunkHdrp = 0;
         	if(other.getcMapp())
         		cMapp = new CellMap(*(other.getcMapp()));
         	else
         		cMapp = 0;
		//child = new vector<CostNode>(*(other.getchild()));
		child = other.getchild();
	}
	return (*this);
}//CostNode::operator=()
*/
void CostNode::printTree(CostNode* root, int maxDepth, ofstream& out)
{
	if(!root){
		return;
	}

       	out << "CHUNK : "<<root->getchunkHdrp()->id.getcid()<<endl;
       	out << "----------------------------------------"<<endl;
       	out <<"\tDepth = "<<root->getchunkHdrp()->depth<<
                ", Local Depth = "<<root->getchunkHdrp()->localDepth<<
                ", NextLocalDepth flag = "<<root->getchunkHdrp()->nextLocalDepth<<
       	        ", NumOfDims = "<<root->getchunkHdrp()->numDim<<
       	        ", totNumCells = "<<root->getchunkHdrp()->totNumCells<<
       	        ", rlNumCells = "<<root->getchunkHdrp()->rlNumCells<<
       	        ", size(bytes) = "<<root->getchunkHdrp()->size<<endl;
       			
       	// print levels	
       	for (	vector<LevelRange>::const_iterator i = root->getchunkHdrp()->vectRange.begin();
  		i != root->getchunkHdrp()->vectRange.end();	++i){
  			out<<"\t"<<(*i).dimName<<"("<<(*i).lvlName<<") : ["<<(*i).leftEnd<<", "<<(*i).rightEnd<<"]"<<endl;
 	}
        // print cell chunk ids
	out<<"\tExisting Cells (Chunk-IDs) : "<<endl;
        //#ifdef DEBUGGING
        //	if(root->getcMapp()->getchunkidVectp()->empty()) {
        //	 	cerr<<"CostNode::printTree ==> CellMap Chunkid vector is empty!!!"<<endl;
        //	}
        //#endif
        if(AccessManager::isDataChunk(root->getchunkHdrp()->depth, root->getchunkHdrp()->localDepth, root->getchunkHdrp()->nextLocalDepth, maxDepth)){
       		//if this is a large data chunk
		if( AccessManager::isLargeChunk(root->getchunkHdrp()->size) ){
                   	for (	vector<ChunkID>::const_iterator iter = root->getcMapp()->getchunkidVectp()->begin();
                		iter != root->getcMapp()->getchunkidVectp()->end();
                		++iter	){

                		out<<"\t"<<(*iter).getcid()<<", "<<endl;			  		   		   		
                	}//for
                }//if
                else {
                	out << "Data points were discarded to save memory\n";
                }//end else
        }//if
        else {
              	for (	vector<ChunkID>::const_iterator iter = root->getcMapp()->getchunkidVectp()->begin();
           		iter != root->getcMapp()->getchunkidVectp()->end();
           		++iter	){

           		out<<"\t"<<(*iter).getcid()<<", "<<endl;			  		   		   		
           	}//for
        }//end else
        #ifdef DEBUGGING
        	cerr<<"CostNode::printTree ==> Testing CostNode children:\n";
	      	for (	vector<CostNode*>::const_iterator j = root->getchild().begin(); j != root->getchild().end(); ++j){
   			cerr<<"\t"<<(*j)->getchunkHdrp()->id.getcid()<<endl;
   		}        	
        #endif	
   	
   	out <<"\n\n";   	
   	// descend to children
      	for (	vector<CostNode*>::const_iterator j = root->getchild().begin();
   		j != root->getchild().end();
   		++j	){
		CostNode::printTree(*j, maxDepth, out);  			
   	}   	
   	
      	/*for (	vector<CostNode>::const_iterator j = root->getchild()->begin();
   		j != root->getchild()->end();
   		++j	){
		CostNode::printTree(const_cast<vector<CostNode>::iterator>(j), out);  			
   	}*/   	
} // end of CostNode::printTree

void CostNode::calcTreeSize(const CostNode* const root, unsigned int& szBytes)//,unsigned int& szPages)
// IMPORTANT precondition:
// the szBytes is an accumulative total, therefore when first called (1st recursion) it should be zero.
{
	if(!root){
		throw GeneralError(__FILE__, __LINE__, "CostNode::calcTreeSize ==> null pointer\n");
	}
	
	// add the size of the chunk pointed by root
	szBytes += root->getchunkHdrp()->size;
	//also add the cost for the corresponding entry in the internal directory of the DiskBucket
	szBytes += sizeof(DiskBucketHeader::dirent_t);
	
	//szPages += int(ceil(float(szBytes)/float(PAGESIZE)));
   	
   	// descend to children
   	// if this is not a data chunk
   	if(!root->getchild().empty()){
              	for (vector<CostNode*>::const_iterator j = root->getchild().begin();
           	     j != root->getchild().end(); j++){
        		
			CostNode::calcTreeSize(*j, szBytes); //, szPages);  			
           	}//end for   		
	}//end if	
} // end of CostNode::calcTreeSize

void CostNode::countDirChunksOfTree(const CostNode* const costRoot,
				 unsigned int maxDepth,
				 unsigned int& total)
// precondition:
//	costRoot points at a CostNode corresponding to a directory or a data chunk. total is either 0 (when first
//	called - 1st recursion), or it contains
//	the number of parent directory chunks all the way up to the root (with which the first call
//	was made).maxdepth contains the maximum depth of the cube in question.
// postcondition:
//	the returned result equals the number of directory chunks under costRoot
{
	//ASSERTION1: not a null pointer
	if(!costRoot)
		throw GeneralError(__FILE__, __LINE__, "ASSERTION1 in CostNode::countDirChunksOfTree ==> null pointer\n");

	//ASSERTION2: it has a valid depth
//	if(costRoot->getchunkHdrp()->depth > maxDepth + 1 ||
//						costRoot->getchunkHdrp()->depth < Chunk::MIN_DEPTH)
//		throw GeneralError(__FILE__, __LINE__, "CostNode::countDirChunksOfTree ==> invalid global depth\n");

	// if this is a data chunk
//	if(costRoot->getchunkHdrp()->depth == maxDepth + 1)
	if(AccessManagerImpl::isDataChunk(costRoot->getchunkHdrp()->depth, costRoot->getchunkHdrp()->localDepth, costRoot->getchunkHdrp()->nextLocalDepth, maxDepth))
		return; //dont add anything

	//This is a dir chunk so add 1 to the total
	//ASSERTION
	if(!AccessManagerImpl::isDirChunk(costRoot->getchunkHdrp()->depth, costRoot->getchunkHdrp()->localDepth, costRoot->getchunkHdrp()->nextLocalDepth, maxDepth))
		throw GeneralError(__FILE__, __LINE__, "CostNode::countDirChunksOfTree ==> invalid chunk type (invalid depth value(s))\n");		
	total += 1;
	
	//visit children nodes
       	//for each non-empty cell
       	for(vector<CostNode*>::const_iterator cnode_i = costRoot->getchild().begin(); cnode_i != costRoot->getchild().end(); cnode_i++)
       		//add the number of dir chunks of each sub-tree hanging from there
       		countDirChunksOfTree(*cnode_i, maxDepth, total);
	
/*	if(AccessManager::isArtificialChunk(costRoot->getchunkHdrp()->localDepth)){
	
	}//end if
	else { // not an artificial chunk
        	//if this is the maximum depth then
        	if(costRoot->getchunkHdrp()->depth == maxDepth - 1){
        		//we have hit the bottom of dir chunks, return the total
        		return;
        	}//end if
        	else{   //there are more dir chunks further down
        		//for each non-empty cell
        		for(vector<CostNode*>::const_iterator cnode_i = costRoot->getchild().begin();
        			cnode_i != costRoot->getchild().end(); cnode_i++){
        			//add the number of dir chunks of each sub-tree hanging from there
        			countDirChunksOfTree(*cnode_i, maxdepth, total);
        		}//end for
        	}//end else
        }//end else
*/
}// CostNode::countDirChunksOfTree


void CostNode::countDataChunksOfTree(
			const CostNode* const costRoot,
			unsigned int maxDepth,
			unsigned int& total)
// precondition:
//	costRoot points at a CostNode corresponding to a directory or a data chunk. total is either 0 (when first
//	called - 1st recursion), or it contains
//	the number of data chunks under costRoot already visited. maxdepth contains the maximum depth of the cube in question.
// postcondition:
//	the returned result equals the number of data chunks under costRoot
{
	//ASSERTION1: not a null pointer
	if(!costRoot)
		throw GeneralError(__FILE__, __LINE__, "ASSERTION1 in CostNode::countDataChunksOfTree ==> null pointer\n");

	//ASSERTION2: it has a valid depth
//	if(costRoot->getchunkHdrp()->depth > maxdepth + 1 ||
//						costRoot->getchunkHdrp()->depth < Chunk::MIN_DEPTH)
//		throw GeneralError(__FILE__, __LINE__, "ASSERTION2 in CostNode::countDataChunksOfTree ==> invalid depth\n");

	// if this is a data chunk
//	if(costRoot->getchunkHdrp()->depth == maxdepth + 1){
	if(AccessManagerImpl::isDataChunk(costRoot->getchunkHdrp()->depth, costRoot->getchunkHdrp()->localDepth, costRoot->getchunkHdrp()->nextLocalDepth, maxDepth)){
		//add one to the total
		total += 1;
		return; //return to the parent node
	}//end if

	//This is a dir chunk so descend to its children nodes
	//ASSERTION
	if(!AccessManagerImpl::isDirChunk(costRoot->getchunkHdrp()->depth, costRoot->getchunkHdrp()->localDepth, costRoot->getchunkHdrp()->nextLocalDepth, maxDepth))
		throw GeneralError(__FILE__, __LINE__, "CostNode::countDataChunksOfTree ==> invalid chunk type (invalid depth value(s))\n");		
	
       	//for each non-empty cell
       	for(vector<CostNode*>::const_iterator cnode_i = costRoot->getchild().begin();
       		cnode_i != costRoot->getchild().end(); cnode_i++){
       		//add the number of dir chunks of each sub-tree hanging from there
       		countDataChunksOfTree(*cnode_i, maxDepth, total);
       	}//end for
}//CostNode::countDataChunksOfTree
	
void CostNode::countChunksOfTree(
			const CostNode* const costRoot,
			unsigned int maxDepth,
			unsigned int& total)
// precondition:
//	costRoot points at a CostNode corresponding to a directory or a data chunk. total is either 0 (when first
//	called - 1st recursion), or it contains
//	the number of chunks (dir or data) under costRoot already visited.
//      maxdepth contains the maximum depth of the cube in question.
// postcondition:
//	the returned result equals the number of chunks (dir or data) under costRoot
{
	//ASSERTION1: not a null pointer
	if(!costRoot)
		throw GeneralError(__FILE__, __LINE__, "ASSERTION1 in CostNode::countChunksOfTree ==> null pointer\n");

	//ASSERTION2: it has a valid depth
//	if(costRoot->getchunkHdrp()->depth > maxdepth  ||
//						costRoot->getchunkHdrp()->depth < Chunk::MIN_DEPTH)
//		throw GeneralError(__FILE__, __LINE__, "ASSERTION2 in CostNode::countChunksOfTree ==> invalid depth\n");

	// if this is a data chunk
//	if(costRoot->getchunkHdrp()->depth == maxdepth){
	if(AccessManagerImpl::isDataChunk(costRoot->getchunkHdrp()->depth, costRoot->getchunkHdrp()->localDepth, costRoot->getchunkHdrp()->nextLocalDepth, maxDepth)){
		//add one to the total
		total += 1;
		return; //return to the parent node
	}//end if

	//This is a dir chunk so add one to the total and descend to its children nodes
	//ASSERTION
	if(!AccessManagerImpl::isDirChunk(costRoot->getchunkHdrp()->depth, costRoot->getchunkHdrp()->localDepth, costRoot->getchunkHdrp()->nextLocalDepth, maxDepth))
		throw GeneralError(__FILE__, __LINE__, "CostNode::countChunksOfTree ==> invalid chunk type (invalid depth value(s))\n");			
		
	total += 1;
       	//for each non-empty cell
       	for(vector<CostNode*>::const_iterator cnode_i = costRoot->getchild().begin();
       		cnode_i != costRoot->getchild().end(); cnode_i++){
       		//add the number of chunks of each sub-tree hanging from there
       		countChunksOfTree(*cnode_i, maxDepth, total);
       	}//end for
}//CostNode::countChunksOfTree

void CostNode::removeChildFromVector(const ChunkID& id)
{
	//find iterator pointing at child to remove
	vector<CostNode*>::iterator child_iter;
	for(vector<CostNode*>::iterator iter = child.begin(); iter != child.end(); iter++) {
		if((*iter)->getchunkHdrp()->id == id)
			child_iter = iter;
	}//end for
		
	//free up node, propagate to children through ~CostNode()
	delete *child_iter;
	
	//remove entry from vector
	child.erase(child_iter);
}//CostNode::removeChildFromVector

// -------------------------------- end of CostNode --------------------------------------------

// -------------------------------- class CellMap ----------------------------------------------
/*CellMap::CellMap() : chunkidVect(vector<ChunkID>())
{
}

CellMap::~CellMap()
{
}*/

/*CellMap::CellMap(CellMap const & map) : chunkidVect(map)
{
}*/


//reference version
/*void CellMap::setchunkidVect(const vector<ChunkID>& chv)
{
	chunkidVect = chv;
}*/

CellMap& CellMap::operator=(CellMap const& other)
{
	if(this != &other) {
		// deallocate current data
		if(chunkidVectp) delete chunkidVectp;
		// duplicate other's data
		if(other.getchunkidVectp())
			chunkidVectp = new vector<ChunkID>(*(other.getchunkidVectp()));
		else
			chunkidVectp = 0;
	}
	return (*this);
}

// pointer version
bool CellMap::insert(const string& id)
{
	if(id.empty())
		throw GeneralError(__FILE__, __LINE__, "Error inside CellMap::insert : empty chunk id!\n");
	// check if the chunk id already exists
	ChunkID newId(id);
	vector<ChunkID>::iterator result = find(chunkidVectp->begin(), chunkidVectp->end(), newId);
	/*#ifdef DEBUGGING
		cerr<<"CellMap::insert ==> result =  "<<result<<", chunk id to insert = "<<newId.getcid()<<endl;
	#endif*/		
	if(result == chunkidVectp->end()){
         	// OK its a new one
         	chunkidVectp->push_back(newId);
         	#ifdef DEBUGGING
         		cerr<<"CellMap::insert ==> Just inserted into Cellmap id : "<<chunkidVectp->back().getcid()<<endl;
         	#endif
	
		return true;
	}
	return false;
}//end CellMap::insert

// reference versions
/*bool CellMap::insert(string& id)
{
	if(id.empty())
		throw GeneralError(__FILE__, __LINE__, "Error inside CellMap::insert : empty chunk id!\n");
	// check if the chunk id already exists
	ChunkID newId(id);
	vector<ChunkID>::iterator result = find(chunkidVect.begin(), chunkidVect.end(), newId);
	if(result == chunkidVect.end())
		return false;
	// OK its a new one
	chunkidVect.push_back(newId);
	return true;
}*/

CellMap* CellMap::searchMapForDataPoints(const vector<LevelRange>& qbox, const ChunkID& prefix)const
// precondition:
//      *this is an non-empty CellMap and qbox a non-empty query box and prefix a non-empty Chunk id
// processing:
//      iterate through all data points of Cell Map and check wether they fall into the query box
//postcondition:
//      return pointer to new CellMap with retrieved data points. If no data points found return NULL (i.e.,0)
{
	//assert that CellMap is not empty
	if(empty())
		throw GeneralError(__FILE__, __LINE__, "CellMap::searchMapForDataPoints ==> CellMap is empty!\n");
	
	// if qbox is empty
	if(qbox.empty())
	        throw GeneralError(__FILE__, __LINE__, "CellMap::searchMapForDataPoints ==> query box is empty!\n");

	//create new cell map
	CellMap* newmapp = new CellMap;

	//for each chunk id stored in the cell map
	for(vector<ChunkID>::const_iterator id_iter = chunkidVectp->begin(); id_iter != chunkidVectp->end(); id_iter++){
		//get suffix domain
		string suffix = id_iter->get_suffix_domain();
		//turn domain to coordinates
		Coordinates c;
		ChunkID::domain2coords(suffix, c);
		//create corresponding cell
		Cell dataPoint(c, qbox);
		//if (cell is within qbox)
		if(dataPoint.cellWithinBoundaries()){ //then this is a data point of interest
			//create new id: add suffix domain to input prefix
			ChunkID newid(prefix);
       			//and add the new domain as suffix
       			try{
       				newid.addSuffixDomain(suffix);
       			}
         		catch(GeneralError& error){
         			GeneralError e("CellMap::searchMapForDataPoints ==> ");
         			error += e;
         			delete newmapp;
                          	throw error;
         		}
         		catch(...){
         			delete newmapp;
                          	throw;         		
         		}
			//insert new id into new cell map
                	if(!newmapp->insert(newid.getcid())) {
                	        string msg = string("CellMap::searchMapForDataPoints ==> double entry in cell map: ") + newid.getcid();             	
                	        delete newmapp;
                		throw GeneralError(__FILE__, __LINE__, msg.c_str());
                	}// end if
		}//end if
	}//end for

	//if new cellmap is empty: no data points where found within qbox
	if(newmapp->empty()){
		delete newmapp;//free up space
		return 0;
	}//end if
	else{
		return newmapp;//return pointer to new cell map
	}//end else
	
}// end CellMap::searchMapForDataPoints
// -------------------------------- end of CellMap ---------------------------------------------