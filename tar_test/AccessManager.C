/***************************************************************************
                          AccessManager.C  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


#include "definitions.h"
#include "AccessManager.h"
#include "Cube.h"
#include "SystemManager.h"
#include "FileManager.h"
#include "CatalogManager.h"
#include "Chunk.h"

#include <strstream>
#include <fstream>
#include <sm_vas.h>
#include <algorithm>
#include <math.h>

#include <stdio.h>

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
    {load_cmd,  3, "load_cube",  "name dim_file data_file",       "load cube <name> with the data in <data_flie>"},
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

AccessManager::AccessManager() {}

AccessManager::~AccessManager() {}

void AccessManager::parseCommand(char* line, bool& quit)
{
    istrstream  s(line);

    const int   max_params = 5;
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
            err = load_cube(name, dimFile, factFile);
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
            cerr << "Error: " << cmd->name << " command failed." << endl;
        }
    } // end else

} // end AccessManager::commandParse

cmd_err_t AccessManager::create_cube(string& name)
{

	W_COERCE(ss_m::begin_xct());

	FileID file_id;
        // create the cube file
	try {
		FileManager::createCubeFile(file_id);
	}
	catch(const char* message){
		string msg("Exception while creating Cube file: ");
		msg += message;
		cmd_err_t err =  (char*)msg.c_str();
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
	catch(const char* message){
		string msg("Exception while registering new Cube in the catalog: ");
		msg += message;
		cmd_err_t err =  (char*)msg.c_str();
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());
		return err;
	}

 	W_COERCE(ss_m::commit_xct());  // commit the cube creation
	return 0;
}

cmd_err_t AccessManager::drop_cube(string& name)
{
	W_COERCE(ss_m::begin_xct());

	// first get information about the cube
	CubeInfo info;
	try{
		CatalogManager::getCubeInfo(name, info);

	}
	catch(const char* message) {
		string msg("Ex.from CatalogManager::getCubeInfo in AccessManager::drop_cube(): ");
		msg += message;
		cmd_err_t err =  (char*)msg.c_str();
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());
		return err;
	}

	// delete cube file
	try {
		FileManager::destroyCubeFile(info.get_fid());

	}
	catch(const char* message) {
		string msg("Ex. from FileManager::destroyFile in  AccessManager::drop_cube(): ");
		msg += message;
		cmd_err_t err =  (char*)msg.c_str();
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());
		return err;
	}

	// update catalog
	try{
		CatalogManager::unregisterCube(info);

	}
	catch(const char* message) {
		string msg("Ex. from CatalogManager::unregisterCube in AccessManager::drop_cube(): ");
		msg += message;
		cmd_err_t err =  (char*)msg.c_str();
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());
		return err;
	}

 	W_COERCE(ss_m::commit_xct());  // commit the cube destroying

	return 0;
}

cmd_err_t AccessManager::load_cube (string& name, string& dimFile, string& factFile)
{
	// Execute the whole loading (i.e. CUBE File creation) process
	// as one big transaction (i.e. all or nothing).
	W_COERCE(ss_m::begin_xct());

	// first get information about the cube from the catalog
	CubeInfo info;
	try{
		CatalogManager::getCubeInfo(name, info);
	}
	catch(const char* message) {
		string msg("Ex. from CatalogManager::getCubeInfo in AccessManager::load_cube(): ");
		msg += message;
		cmd_err_t err =  (char*)msg.c_str();
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());
		return err;
	}

	// get information about the dimensions from the dimFile
	try {
		info.Get_dimension_information(dimFile);
	}
	catch(const char* message) {
		string msg("Ex. from CubeInfo::Get_dimension_information in AccessManager::load_cube(): ");
		msg += message;
		cmd_err_t err =  (char*)msg.c_str();
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
			for (vector<Level_Member>::iterator iter_mem = (*iter_lev).get_vectMember().begin();
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
	catch(const char* message) {
		string msg("Ex. from CubeInfo::getFactInfo in AccessManager::load_cube(): ");
		msg += message;
		cmd_err_t err =  (char*)msg.c_str();
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());		
		return err;
	}
#ifdef DEBUGGING
      cerr << "Starting the Cube File construction algorithm ..." <<endl;
#endif	
	// Construct CUBE File
	try{
		constructCubeFile(info, factFile);
	}
	catch(const char* message) {
		string msg("");//"Ex. from AccessManager::constructCubeFile in AccessManager::load_cube : ");
		msg += message;
		cmd_err_t err =  (char*)msg.c_str();
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());		
		return err;
	}

	// store updated CubeInfo obj back on disk
	try{
		//CatalogManager::updateCubeInfo(name, info);
	}
	catch(const char* message) {
		string msg("Ex. from CatalogManager::updateCubeInfo in AccessManager::load_cube(): ");
		msg += message;
		cmd_err_t err =  (char*)msg.c_str();
		// Abort current transaction
		W_COERCE(ss_m::abort_xct());
		return err;
	}

 	W_COERCE(ss_m::commit_xct()); // commit Cube loading

	return 0;
}

cmd_err_t AccessManager::print_cube (string& name)
{
	return 0;

}

/*
Chunk_cell_data* AccessManager::Create_root_chunk(CubeInfo& info)
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
                         PAGE_SIZE,    // size of record = 8KBytes
                         vec_t(),      // empty body
                         record_ID);   // bucket ID
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"Error in ss_m::create_rec in AccessManager::Create_root_chunk "<< err <<endl;
		// throw an exeption
		throw error.str();
	}

	//Store the ID of the root bucket in the CubeInfo object
	info.set_rootBucket(record_ID); //***I may store again the CubeInfo object***

	return root_chunk;
}
*/

void AccessManager::constructCubeFile(CubeInfo& cinfo, string& factFile)
{
	// 1. Estimate the storage cost for the components of the chunk hierarchy tree

	// In this phase we will use only chunk headers.
	// 1.1 construct root chunk header.
	ChunkHeader* rootHdrp = new ChunkHeader;
	Chunk::createRootChunkHeader(rootHdrp, cinfo);

	// 1.2 create CostNode Tree
	CostNode* costRoot = 0;
	try {
		costRoot = Chunk::expandChunk(rootHdrp, cinfo, factFile);
	}
	catch(const char* message) {
		//string msg("");//"Ex. from Chunk::expandChunk in AccessManager::constructCubeFile : ");
		//msg += message;
		throw; //msg.c_str();
	}
	
        #ifdef DEBUGGING
        	try{
        	      test_construction_phaseI(costRoot, cinfo);
        	      delete costRoot;
        	      return;
        	}
        	catch(const char* message) {
        		string msg("");//"Ex. from AccessManager::test_construction_phaseI in AccessManager::constructCubeFile : ");
        		msg += message;
        		throw msg.c_str();
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
		error <<"AccessManager::ConstructCubeFile  ==> Error in ss_m::create_id"<< err <<endl;
		// throw an exeption
		throw error.str();
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
	catch(const char* message) {
		string msg("AccessManager::ConstructCubeFile  ==> ");
		msg += message;
		throw msg.c_str();
	}
	// assertion following:
	if((rootEntry.bcktId != cinfo.get_rootBucketID()) || (rootEntry.chnkIndex != cinfo.get_rootChnkIndex())) {
		throw "AccessManager::ConstructCubeFile  ==> Error in returned DirEntry value from AccessManager::putIntoBuckets";
	}
	
	// 2.3. Now, we are ready to create and store the root bucket.
	
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
	size_t recordBdySz = datachunks_offset_in_body + 0;
		
	BucketHeader* rtBcktHdrp = new BucketHeader(rootBcktID, hdrSz, dirSz, dataSz, bodySz, datachunks_offset_in_body, recordBdySz);
	
	//2.3.2. Now, create the bucket instance
	Bucket rootBucket(rtBcktHdrp, rtBcktEntriesVectp, 0);	
	try {
		rootBucket.checkSize();		
	}
	catch(ExceptionBcktOverflow& e1) {
		string m("AccessManager::ConstructCubeFile  ==> ");//"Ex. from Bucket constructor in AccessManager::constructCubeFile : ");
		m += e1.msg;
		throw m.c_str();
	}
	catch(ExceptionBcktHdOverflow& e2) {
		string m("AccessManager::ConstructCubeFile  ==> ");//"Ex. from Bucket constructor in AccessManager::constructCubeFile : ");
		m += e2.msg;
		throw m.c_str();
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
                         hdr,       /* header  */
                         PAGE_SIZE,  /* length hint          */
                         body_data, /* body    */
                         record_ID);      /* rec id           */
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"AccessManager::ConstructCubeFile  ==> Error in ss_m::create_rec_id"<< err <<endl;
		// throw an exeption
		throw error.str();
	}
	
	delete costRoot; // free up the whole tree space!
}

void AccessManager::putIntoBuckets(CubeInfo& cinfo,
				CostNode* costRoot,
				unsigned int where2store,
				string& factFile,
				vector<DirChunk>* rtDirDataVectp,
				DirEntry& returnDirEntry)
				
{
/*
	I. if this is NOT a leaf chunk   	
	
	  1. define a vector costVect
	
	  2. For each cell in the costRoot->cMapp member, call routine CostNode::getSubTreeTotCost()
	     and save the result in costVect.
	
	  3. The costRoot represents the current chunk, i.e. the one that corresponds to the costRoot node.
	     We want to store this chunk in the rtDirDataVectp.In order
	     to do that we need to create a DirChunk and fill appropriatelly its entries, then insert it in
	     the vector at position where2store.
	     	3.1. vector<DirEntry> entryVect(costRoot->getchunkHdrp()->totNumCells);
	     	     NOTE: the default DirEntry constructor should initialize
	     	     the member BucketID with a BucketID::null constant.
	     	
	  4. We have 3 cases  (a,b,c) that we have to deal with, arising from the contents of costVect:
			case a: T<=costVect[i]<=B
			case b: costVect[i] < T
			case c: costVect[i] > B
			case d: EMPTY cell
			
	   	4.1. Traverse costVect and insert in three different vectors the chunk ids and the
	   	     total costs of each case: caseAvect, caseBvect, caseCvect
	   	
	 5. Process each list separately:
	 	
	 	5.1. For each entry of caseAvect:
	 		call storeIntoBucket to store sub-tree in depth-first mode:
	 		this should return the appropriate DirEntry. I use
	 		the chunk-id to find the offset in entryVect, where I store this DirEntry	   	
	 		
	 	5.2. Call find_best_clusters(caseBVect) : this should return a vector of
	 	     DirEntries with 1-1 correspondance with caseBVect. I use this to store
	 	     in the appropriate positions in entryVect.
	 	
		5.3. rtDirDataVectp->reserve(1),
		     then recursively call putIntoBuckets, with where2store = rtDirDataVectp->capacity().
		     Store the returned DirEntry at the corresponding position of entryVect.
	
	6. Now that entryVect is filled, create the chunk
		6.1.  DirChunk newChunk((*costRoot->getchunkHdrp()), entryVect);
		
		6.2. insert newChunk in the rtDirDataVectp at position where2store.
		
	7. return the DirEntry corresponding to the newChunk

	II. Else if this a leaf chunk
	
	  1. call storeIntoBucketChain()	     				     			     		 		  		
*/	
} // end of AccessManager::putIntoBuckets

void AccessManager::test_construction_phaseI(CostNode* costRoot, CubeInfo& cinfo)
{
	ofstream out("dbg_construction_I");

        if (!out)
        {
            cerr << "creating file \"dbg_log_construction_I\" failed\n";
            throw "AccessManager::test_construction_phaseI ==> creating file \"dbg_log_construction_I\" failed\n";
        }

        out <<"**************************************************"<<endl;	
        out <<"*	    CHUNK TREE INFORMATION              *"<<endl;
        out <<"**************************************************"<<endl;	
        out <<"\n\n";

        // Start traversing the tree..
        CostNode::printTree(costRoot, out);

        // Calculate total chunk tree size
        unsigned int szBytes = 0;
        unsigned int szPages = 0;
        CostNode::calcTreeSize(costRoot, szBytes); //, szPages);
        szPages = int(ceil(float(szBytes)/float(PAGE_SIZE)));

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
                      cerr<<"AccessManager::test_construction_phaseI ==> Most detailed level of dim "<<(*iter_dim).get_name()<<" is "<<level.get_name()<<endl;
                      cerr<<"With #members : "<<level.get_num_of_members()<<endl;
                #endif
         	szBytesFullCb *= level.get_num_of_members(); //cartesian product of grain members!
	}
	int entry_size = cinfo.getnumFacts() * sizeof(float);		
	szBytesFullCb *= entry_size;
	
        out<<"\n\n******************************************"<<endl;
        out<<    " TOTAL SIZE (bytes) : "<<szBytes<<endl;
        out<<    " TOTAL SIZE (MINIMUM #disk pages) : "<<szPages<<endl;
        out<<    " TOTAL SIZE FULL CUBE (bytes) : "<<szBytesFullCb<<endl;
        out<<    "******************************************"<<endl;

} // end of AccessManager::test_construction_phaseI.


/* Loukas Sinos

int* AccessManager::expandChunk(CubeInfo& info, ChunkID& cID, string& datafile)
{
	// First, allocate space in memory to store the cell map
	int number_of_cells = 1; // the number of cells of the map

	if (cID.get_chunk_id() == "0") // if this is the root chunk
	{
		vector<Dimension_Level>::iterator iter_lev;
    	for (vector<Dimension>::iterator iter_dim = info.get_vectDim().begin();
    								 iter_dim != info.get_vectDim().end();
    								 ++iter_dim)
    	{
			iter_lev = (*iter_dim).get_vectLevel().begin(); //pointer to upper level
			number_of_cells *= (*iter_lev).get_num_of_members(); //number of members of the upper level
		}
	}
	else // the chunk is not the root chunk
	{
		//get the suffix of the chunk ID
		//if chunkID = X1|X2|X3.Y1|Y2|Y3.Z1|Z2|Z3 then the suffix is Z1|Z2|Z3
  		string suffix = cID.get_suffix_domain();
  		string::size_type pos1, pos2;
 		pos1 = 0;
		int order_code; //stores the order code of the member (Z1, Z2 and Z3)
		vector<Dimension_Level>::iterator iter_lev;
		vector<Level_Member>::iterator iter_mem;
    	for (vector<Dimension>::iterator iter_dim = info.get_vectDim().begin();
    								 iter_dim != info.get_vectDim().end();
    								 ++iter_dim)
		{
  			pos2 = suffix.find("|"); //position of first "|", then of second "|" while the loop is executed
									 //if no "|" is found (this happens in the last loop) pos2 takes the value <string::npos>
			order_code = atoi(suffix.substr(pos1, pos2).c_str()); //order code takes in turns the values Z1, Z2, Z3
			if (pos2 != string::npos) //if this is not the last loop
			{
				suffix = suffix.substr(pos2+1, string::npos); //suffix takes in turn the values "Z2|Z3" and Z3
				pos1 = pos2+1; //position next to the character "|"
			}
			//go to the level that the member with (order code = order_code) belongs to
			//the level is relative to the chunking depth, for example if chunking depth D = 1 then level number is 0
			//in general, level number = D - 1
			iter_lev = (*iter_dim).get_vectLevel().begin();
			for (int i=0; i<(cID.get_chunk_depth())-1; i++)
				iter_lev++;

			//since we went to the right level, we try to find the correct member
			iter_mem = (*iter_lev).get_vectMember().begin();
			while ((*iter_mem).get_order_code() != order_code)
				iter_mem++;

			//in order to calculate the dimensions of the cel map, we calculate the number of children of each member
			//that takes part in the suffix of the chunk ID and we multiply all
			//if Z1 has 5 members, Z2 has 2 members and Z3 has 3 members then number_of_cells = 5 * 2 * 3 = 30cells
			number_of_cells *= ((*iter_mem).get_last_child_order_code()) - ((*iter_mem).get_first_child_order_code()) + 1;
		}
	}
	try {
		int* cell_map = new int[number_of_cells];
	}
	catch(...) {
		throw "Cannot allocate memory for the construction of the cell map";
	}
	//The allocation of memory space was completed


	//Now, we must fill in the cell map with the cost values
	//First create a string vector of the cells' ids
	vector<string> vecID;
}

int AccessManager::directoryCost(CubeInfo& info, ChunkID& cID, string& datafile)
{
	int cost = 0;

	vector<Dimension>::iterator iter_dim = info.get_vectDim().begin();
	if (cID.get_chunk_depth() == ((*iter_dim).get_num_of_levels() - 1)) //if this is a data chunk
		return cost;
	//Check if there are data cells with prefix the cID
	if (check_chunkID(cID, datafile)) ;

}
*/

/*bool AccessManager::check_chunkID(ChunkID& cID, string& datafile)
{
	ifstream ifstrm(datafile.c_str());
	if (!ifstrm)
    {
    	throw "Creating ifstream object failed. Cannot open datafile";
    }

    string buffer; //stores a word from the input file
    ifstrm >> buffer;
    bool found_data = false;
    int length = strlen(cID.get_chunk_id().c_str());
    //while (cID.get_chunk_id().compare(0,cID.get_chunk_id().length(),buffer) < 0)
    while (strncmp(cID.get_chunk_id().c_str(), buffer.c_str(), length) < 0)
    {
		ifstrm >> buffer;
		ifstrm >> buffer;
	}
	//if (cID.get_chunk_id().compare(0,cID.get_chunk_id().length(),buffer) == 0)
    if (strncmp(cID.get_chunk_id().c_str(), buffer.c_str(), length) == 0)
		found_data = true;

	ifstrm.close(); //close file stream

	return found_data;
}
*/

/* Loukas Sinos
void AccessManager::create_vector_of_chunk_ids( string& cID, //the chunk id of the chunk for which we want to create the cells' IDs
												string& suffix, //the suffix of the chunk id or part of the suffix
												string& newID, //the new chunk id for a cell or part of it
												vector<Dimension>::iterator iter_dim, //points to the current dimension
												vector<string>& vec) //stores the ids of the cells
{
	vector<Dimension_Level>::iterator iter_lev;
	vector<Level_Member>::iterator iter_mem;
	string::size_type pos1;
	int order_code;
	if (cID == "0")
	{
		iter_lev = (*iter_dim).get_vectLevel().begin();



		pos1 = suffix.find("|");
		order_code = atoi(suffix.substr(0, pos1).c_str());
	}

}
*/

//---------------------------- end of AccessManager -----------------------------------------//

// --------------------------- class CostNode -----------------------------------------------//
CostNode::~CostNode(){
	delete chunkHdrp;
	delete cMapp;
	//delete child;
	for (	vector<CostNode*>::iterator iter = child.begin();
		iter != child.end();
		++iter )
	{
       		delete (*iter);
	}
}

const CostNode & CostNode::operator=(const CostNode & other)
{
	if(this != &other) {
		// deallocate current data
		delete chunkHdrp;
		delete cMapp;
		//delete child;
    		for (
			vector<CostNode*>::iterator iter = child.begin();
			iter != child.end();
			++iter
		)
		{
         		delete (*iter);
		}

		// duplicate other's data
		chunkHdrp = new ChunkHeader(*(other.getchunkHdrp()));
		cMapp = new CellMap(*(other.getcMapp()));
		//child = new vector<CostNode>(*(other.getchild()));
		child = other.getchild();
	}
	return (*this);
}

void CostNode::printTree(CostNode* root, ofstream& out)
{
        /*#ifdef DEBUGGING
              cerr<<"CostNode::printTree ==> Inside CostNode::printTree" << endl;
        #endif*/
	if(!root){
		return;
	}

       	out << "CHUNK : "<<root->getchunkHdrp()->id.getcid()<<endl;
       	out << "----------------------------------------"<<endl;
       	out <<"\tDepth = "<<root->getchunkHdrp()->depth<<", NumOfDims = "
       		<<root->getchunkHdrp()->numDim<<", totNumCells = "
       		<<root->getchunkHdrp()->totNumCells<<", rlNumCells = "
       		<<root->getchunkHdrp()->rlNumCells<<", size(bytes) = "
       		<<root->getchunkHdrp()->size<<endl;
	
       	// print levels	
       	for (	vector<LevelRange>::const_iterator i = root->getchunkHdrp()->vectRange.begin();
  		i != root->getchunkHdrp()->vectRange.end();	++i){
  			out<<"\t"<<(*i).dimName<<"("<<(*i).lvlName<<") : ["<<(*i).leftEnd<<", "<<(*i).rightEnd<<"]"<<endl;
 	}
        // print cell chunk ids
	out<<"\tExisting Cells (Chunk-IDs) : "<<endl;
        #ifdef DEBUGGING
        	if(root->getcMapp()->getchunkidVectp()->empty()) {
        	 	cerr<<"CostNode::printTree ==> CellMap Chunkid vector is empty!!!"<<endl;
        	}
        #endif	
      	for (	vector<ChunkID>::const_iterator iter = root->getcMapp()->getchunkidVectp()->begin();
   		iter != root->getcMapp()->getchunkidVectp()->end();
   		++iter	){

                /*#ifdef DEBUGGING
                      cerr<<"CostNode::printTree ==> printing cell chunk-ids..." << endl;
                #endif*/
   		
   		out<<"\t"<<(*iter).getcid()<<", "<<endl;			  		   		   		
   	}
        #ifdef DEBUGGING
        	cerr<<"CostNode::printTree ==> Testing CostNode children:\n";
	      	for (	vector<CostNode*>::const_iterator j = root->getchild().begin();
   		j != root->getchild().end();
   		++j	){
   			cerr<<"\t"<<(*j)->getchunkHdrp()->id.getcid()<<endl;
   		}        	
        #endif	
        /*#ifdef DEBUGGING
        	cerr<<"CostNode::printTree ==> Testing CostNode children:\n";
	      	for (	vector<CostNode>::const_iterator j = root->getchild()->begin();
   		j != root->getchild()->end();
   		++j	){
   			cerr<<"\t"<<(*j).getchunkHdrp()->id.getcid()<<endl;
   		}        	
        #endif	*/

   	
   	out <<"\n\n";   	
   	// descend to children
      	for (	vector<CostNode*>::const_iterator j = root->getchild().begin();
   		j != root->getchild().end();
   		++j	){
		CostNode::printTree(*j, out);  			
   	}   	
   	
      	/*for (	vector<CostNode>::const_iterator j = root->getchild()->begin();
   		j != root->getchild()->end();
   		++j	){
		CostNode::printTree(const_cast<vector<CostNode>::iterator>(j), out);  			
   	}*/   	
} // end of CostNode::printTree

void CostNode::calcTreeSize(CostNode* root, unsigned int& szBytes) //,unsigned int& szPages)
{
	if(!root){
		return;
	}
	szBytes += root->getchunkHdrp()->size;
	//szPages += int(ceil(float(szBytes)/float(PAGE_SIZE)));
   	// descend to children
      	for (	vector<CostNode*>::const_iterator j = root->getchild().begin();
   		j != root->getchild().end();
   		++j	){
		CostNode::calcTreeSize(*j, szBytes); //, szPages);  			
   	}   		
} // end of CostNode::calcTreeSize

// -------------------------------- end of CostNode --------------------------------------------

// -------------------------------- class CellMap ----------------------------------------------
/*CellMap::CellMap() : chunkidVect(vector<ChunkID>())
{
}

CellMap::~CellMap()
{
}*/
CellMap::CellMap() : chunkidVectp(new vector<ChunkID>) {}
CellMap::~CellMap() { delete chunkidVectp; }

/*CellMap::CellMap(CellMap const & map) : chunkidVect(map)
{
}*/

CellMap::CellMap(CellMap const & map) {
        // copy the data
	chunkidVectp = new vector<ChunkID>(*(map.getchunkidVectp()));
}

//reference version
/*void CellMap::setchunkidVect(const vector<ChunkID>& chv)
{
	chunkidVect = chv;
}*/
// pointer version
void CellMap::setchunkidVectp(vector<ChunkID>* const chv)
{
	chunkidVectp = chv;
}

CellMap const& CellMap::operator=(CellMap const& other)
{
	if(this != &other) {
		// deallocate current data
		delete chunkidVectp;
		// duplicate other's data
		chunkidVectp = new vector<ChunkID>(*(other.getchunkidVectp()));
	}
	return (*this);
}

// pointer version
bool CellMap::insert(const string& id)
{
	if(id.empty())
		throw "Error inside CellMap::insert : empty chunk id!\n";
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
}
// reference versions
/*bool CellMap::insert(string& id)
{
	if(id.empty())
		throw "Error inside CellMap::insert : empty chunk id!\n";
	// check if the chunk id already exists
	ChunkID newId(id);
	vector<ChunkID>::iterator result = find(chunkidVect.begin(), chunkidVect.end(), newId);
	if(result == chunkidVect.end())
		return false;
	// OK its a new one
	chunkidVect.push_back(newId);
	return true;
}*/

// -------------------------------- end of CellMap ---------------------------------------------