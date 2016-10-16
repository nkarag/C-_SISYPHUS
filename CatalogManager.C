/***************************************************************************
                          CatalogManager.C  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


#include <sm_vas.h>
#include "CatalogManager.h"
#include "SystemManager.h"
#include "Cube.h"
#include "Exceptions.h"
#include <strstream>

typedef w_rc_t rc_t; // shorter version of Shore return code

// static member initialization
serial_t CatalogManager::rootIndexID = serial_t::null;
/* OLD
const char * CatalogManager::catalogName = "DBcatalog";
serial_t CatalogManager::btreeCatalogID = serial_t::null;
END OLD */
const char * CatalogManager::MDcatalogName = "MD-DB-Catalog";
serial_t CatalogManager::MDcatalogID = serial_t::null;

const char * CatalogManager::RELcatalogName = "REL-DB-Catalog";
serial_t CatalogManager::RELcatalogID = serial_t::null;

const char * CatalogManager::cbNmIndexName = "CubeIndexByName";
serial_t CatalogManager::cbNmIndexID = serial_t::null;

const char * CatalogManager::cbIDIndexName = "CubeIndexByID";
serial_t CatalogManager::cbIDIndexID = serial_t::null;

const char * CatalogManager::dimNmIndexName = "DimIndexByName";
serial_t CatalogManager::dimNmIndexID = serial_t::null;

const char * CatalogManager::dimIDIndexName = "DimIndexByID";
serial_t CatalogManager::dimIDIndexID = serial_t::null;

const char * CatalogManager::cbInfoFileName = "CubeInfoFile";
serial_t CatalogManager::cbInfoFileID = serial_t::null;

const char * CatalogManager::dimInfoFileName = "DimInfoFile";
serial_t CatalogManager::dimInfoFileID = serial_t::null;

//constant member
const cubeID_t CatalogManager::MAXKEY = -1; // key to access the current max cube id from the catalog
					    // **NOTE** MAXKEY must be != from CubeInfo::null_id !!!

CatalogManager::CatalogManager(ostream& outputLogStream = cout,
                            ostream& errorLogStream = cerr)
			: outputLogStream(outputLogStream),
	          errorLogStream(errorLogStream)
{

	lvid_t volumeId = SystemManager::getDevVolInfo()->volumeID;

    	/*
     	 * Begin transaction is necessary for functions which require to be
     	 * executed within the scope of a transaction. Such a function is vol_root_index.
     	 */
    	W_COERCE(ss_m::begin_xct());

	// get the root index ID of the volume
	W_COERCE(ss_m::vol_root_index(volumeId, rootIndexID));

	// Try to locate the catalogs for the md database and the relational database

	// MD first
    	smsize_t info_len = sizeof(serial_t);
    	bool found = false;
    	W_COERCE(ss_m::find_assoc(volumeId, rootIndexID,
                              vec_t(MDcatalogName, strlen(MDcatalogName)),
                              &MDcatalogID,
                              info_len, found));
    	if (found) {
        	assert(info_len == sizeof(MDcatalogID));
        	this->outputLogStream << "Using already existing multidimensional database catalog" << endl;
		try {
			// Read the MD catalog and store its information in the corresponding members
			initMDcatalogMembers();
		}
		catch(GeneralError& error) {
			//this->errorLogStream << "Exception in initializing MD catalog members: "<<message << endl;
			GeneralError e("CatalogManager::CatalogManager ==> Exception in initializing MD catalog members: ");
			error += e;
			ss_m::abort_xct();
			throw error; // throw it to the caller (SsmStartupThread.run())
		}
    	} else {
        	this->outputLogStream << "Creating a new multidimensional database catalog" << endl;
		try {
			createCatalogMD();
		}
		catch(GeneralError& error) {
			//this->errorLogStream << "Exception in creating MD database catalog: "<<message << endl;
			GeneralError e("CatalogManager::CatalogManager ==> Exception in creating MD database catalog: ");
			error += e;			
			ss_m::abort_xct();
			throw error; // throw it to the caller (SsmStartupThread.run())
		}
	}

	// Relational second
    	info_len = sizeof(serial_t);
    	found = false;
    	W_COERCE(ss_m::find_assoc(volumeId, rootIndexID,
                              vec_t(RELcatalogName, strlen(RELcatalogName)),
                              &RELcatalogID,
                              info_len, found));
    	if (found) {
        	assert(info_len == sizeof(RELcatalogID));
        	this->outputLogStream << "Using already existing relational database catalog" << endl;

		// CODE_TODO when relational dbs will be supported
    	} else {
        	this->outputLogStream << "Creating a new relational database catalog" << endl;
		try {
			createCatalogREL();
		}
		catch(GeneralError& error) {
			//this->errorLogStream << "Exception in creating REL database catalog: "<<message << endl;
			GeneralError e("CatalogManager::CatalogManager ==> Exception in creating REL database catalog: ");
			error += e;						
			ss_m::abort_xct();
			throw error; // throw it to the caller (SsmStartupThread.run())
		}
	}
	W_COERCE(ss_m::commit_xct());

} // end CatalogManager

CatalogManager::~CatalogManager() {}

void CatalogManager::initMDcatalogMembers()
{

	// get Shore ID of cube index by name
	smsize_t id_len = sizeof(cbNmIndexID);
    	bool found = false;
    	rc_t err = ss_m::find_assoc(SystemManager::getDevVolInfo()->volumeID, MDcatalogID,
                              vec_t(cbNmIndexName, strlen(cbNmIndexName)),
                              &cbNmIndexID,
                              id_len, found);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::initMDcatalogMembers ==> "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}
	if(!found) {
		throw GeneralError(__FILE__, __LINE__, "CatalogManager::initMDcatalogMembers ==> Cannot find stored cube index by name: unable to use existing catalog");
	}

	// get Shore ID of cube index by id
	id_len = sizeof(cbIDIndexID);
    	found = false;
    	err = ss_m::find_assoc(SystemManager::getDevVolInfo()->volumeID, MDcatalogID,
                              vec_t(cbIDIndexName, strlen(cbIDIndexName)),
                              &cbIDIndexID,
                              id_len, found);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::initMDcatalogMembers ==> "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}
	if(!found) {
		throw GeneralError(__FILE__, __LINE__, "CatalogManager::initMDcatalogMembers ==>Cannot find stored cube index by id: unable to use existing catalog");
	}


	// get Shore ID of dim index by name
	id_len = sizeof(dimNmIndexID);
    	found = false;
    	err = ss_m::find_assoc(SystemManager::getDevVolInfo()->volumeID, MDcatalogID,
                              vec_t(dimNmIndexName, strlen(dimNmIndexName)),
                              &dimNmIndexID,
                              id_len, found);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::initMDcatalogMembers ==>"<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}
	if(!found) {
		throw GeneralError(__FILE__, __LINE__, "CatalogManager::initMDcatalogMembers ==> Cannot find stored dimension index by name: unable to use existing catalog");
	}


	// get Shore ID of  dim index by id
	id_len = sizeof(dimIDIndexID);
    	found = false;
    	err = ss_m::find_assoc(SystemManager::getDevVolInfo()->volumeID, MDcatalogID,
                              vec_t(dimIDIndexName, strlen(dimIDIndexName)),
                              &dimIDIndexID,
                              id_len, found);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::initMDcatalogMembers ==>"<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}
	if(!found) {
		throw GeneralError(__FILE__, __LINE__, "CatalogManager::initMDcatalogMembers ==> Cannot find stored dimension index by id : unable to use existing catalog");
	}

	// get Shore ID of cube info file
	id_len = sizeof(cbInfoFileID);
    	found = false;
    	err = ss_m::find_assoc(SystemManager::getDevVolInfo()->volumeID, MDcatalogID,
                              vec_t(cbInfoFileName, strlen(cbInfoFileName)),
                              &cbInfoFileID,
                              id_len, found);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::initMDcatalogMembers ==>"<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}
	if(!found) {
		throw GeneralError(__FILE__, __LINE__, "CatalogManager::initMDcatalogMembers ==> Cannot find file with CubeInfo instances  : unable to use existing catalog");
	}

	// get Shore ID of dim info file
	id_len = sizeof(dimInfoFileID);
    	found = false;
    	err = ss_m::find_assoc(SystemManager::getDevVolInfo()->volumeID, MDcatalogID,
                              vec_t(dimInfoFileName, strlen(dimInfoFileName)),
                              &dimInfoFileID,
                              id_len, found);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::initMDcatalogMembers ==>"<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}
	if(!found) {
		throw GeneralError(__FILE__, __LINE__, "CatalogManager::initMDcatalogMembers ==> Cannot find file with DimInfo instances  : unable to use existing catalog");
	}

} // end initMDCatalogMembers

void CatalogManager::createCatalogMD()
{
	// 1.1 create md database catalog

        // create the btree index on index name
        // the "b*100" indicates the key type is a variable
        // length byte string with maximum length of 100
        rc_t err = ss_m::create_index(SystemManager::getDevVolInfo()->volumeID, ss_m::t_uni_btree, ss_m::t_regular,
                                "b*100", 0, MDcatalogID);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error << "CatalogManager::createCatalogMD() ==> Error in creating md database catalog index "<<err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	// 1.2. store md db catalog OID in root index
	err = ss_m::create_assoc(SystemManager::getDevVolInfo()->volumeID, rootIndexID,
                                vec_t(MDcatalogName, strlen(MDcatalogName)),
                                vec_t(&MDcatalogID, sizeof(MDcatalogID)));
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::createCatalogMD() ==> Error in creating association in root index "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}


	// 2.1 create cube index by name

        // create the btree index on cube name
        // the "b*100" indicates the key type is a variable
        // length byte string with maximum length of 100
        err = ss_m::create_index(SystemManager::getDevVolInfo()->volumeID, ss_m::t_uni_btree, ss_m::t_regular,
                                "b*100", 0, cbNmIndexID);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error << "CatalogManager::createCatalogMD() ==> Error in creating cube index by name: "<<err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	// 2.2 store cube index by name OID in md catalog
	err = ss_m::create_assoc(SystemManager::getDevVolInfo()->volumeID, MDcatalogID,
                                vec_t(cbNmIndexName, strlen(cbNmIndexName)),
                                vec_t(&cbNmIndexID, sizeof(cbNmIndexID)));
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::createCatalogMD() ==> Error in creating association in md catalog index: "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	// 3.1 create cube index by id

        // create the btree index on cube name
        // the "i4" indicates the key type is a 4 byte integer
        err = ss_m::create_index(SystemManager::getDevVolInfo()->volumeID, ss_m::t_uni_btree, ss_m::t_regular,
                                "i4", 0, cbIDIndexID);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error << "CatalogManager::createCatalogMD() ==> Error in creating cube index by id: "<<err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	// 3.2 store cube index by id OID in md catalog
	err = ss_m::create_assoc(SystemManager::getDevVolInfo()->volumeID, MDcatalogID,
                                vec_t(cbIDIndexName, strlen(cbIDIndexName)),
                                vec_t(&cbIDIndexID, sizeof(cbIDIndexID)));
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::createCatalogMD() ==> Error in creating association in md catalog indexa #2: "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	// 3.3 store the current maximum cube id
	cubeID_t key = MAXKEY;
	int max_curr_code = -1; // begin with -1, so that the first call to createNewCode will return 0.
	err = ss_m::create_assoc(SystemManager::getDevVolInfo()->volumeID,cbIDIndexID,
                                vec_t(&key, sizeof(cubeID_t)),
                                vec_t(&max_curr_code, sizeof(max_curr_code)));
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::createCatalogMD() ==> Error in ss_m::create_assoc #3 "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}



	// 4.1 create dim index by name

        // create the btree index on dimension name
        // the "b*100" indicates the key type is a variable
        // length byte string with maximum length of 100
        err = ss_m::create_index(SystemManager::getDevVolInfo()->volumeID, ss_m::t_uni_btree, ss_m::t_regular,
                                "b*100", 0, dimNmIndexID);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error << "CatalogManager::createCatalogMD() ==> Error in creating dimension index by name: "<<err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	// 4.2 store dim index by name OID in md catalog
	err = ss_m::create_assoc(SystemManager::getDevVolInfo()->volumeID, MDcatalogID,
                                vec_t(dimNmIndexName, strlen(dimNmIndexName)),
                                vec_t(&dimNmIndexID, sizeof(dimNmIndexID)));
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::createCatalogMD() ==> Error in creating association in md catalog index #4: "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	// 5.1 create dim index by id

        // create the btree index on cube name
        // the "i4" indicates the key type is a 4 byte integer
        err = ss_m::create_index(SystemManager::getDevVolInfo()->volumeID, ss_m::t_uni_btree, ss_m::t_regular,
                                "i4", 0, dimIDIndexID);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error << "CatalogManager::createCatalogMD() ==> Error in creating dimension index by id: "<<err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}


	// 5.2 store dim index by id OID in md catalog
	err = ss_m::create_assoc(SystemManager::getDevVolInfo()->volumeID, MDcatalogID,
                                vec_t(dimIDIndexName, strlen(dimIDIndexName)),
                                vec_t(&dimIDIndexID, sizeof(dimIDIndexID)));
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::createCatalogMD() ==> Error in creating association in md catalog indexa #5: "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	// 6.1 create CubeInfo file

	// create shore file
	err = ss_m::create_file(SystemManager::getDevVolInfo()->volumeID, cbInfoFileID, ss_m::t_regular);
	if(err) {
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::createCatalogMD() ==> Error in creating Shore file for CubeInfos: " << err <<endl<<ends;
		// throw an exception
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	// 6.2 store CubeInfo file OID in md catalog
	err = ss_m::create_assoc(SystemManager::getDevVolInfo()->volumeID, MDcatalogID,
                                vec_t(cbInfoFileName, strlen(cbInfoFileName)),
                                vec_t(&cbInfoFileID, sizeof(cbInfoFileID)));
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::createCatalogMD() ==> Error in creating association in md catalog index #6: "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	// 7.1 create DimInfo file
	err = ss_m::create_file(SystemManager::getDevVolInfo()->volumeID, dimInfoFileID, ss_m::t_regular);
	if(err) {
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::createCatalogMD() ==> Error in creating Shore file for DimInfos: " << err <<endl<<ends;
		// throw an exception
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	// 7.2 store DimInfo file OID in md catalog
	err = ss_m::create_assoc(SystemManager::getDevVolInfo()->volumeID, MDcatalogID,
                                vec_t(dimInfoFileName, strlen(dimInfoFileName)),
                                vec_t(&dimInfoFileID, sizeof(dimInfoFileID)));
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::createCatalogMD() ==> Error in creating association in md catalog index #7: "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

} //end createCatalogMD()

void CatalogManager::createCatalogREL()
{
	// 1.1. create rel database catalog

        // create the btree index on index name
        // the "b*100" indicates the key type is a variable
        // length byte string with maximum length of 100
        rc_t err = ss_m::create_index(SystemManager::getDevVolInfo()->volumeID, ss_m::t_uni_btree, ss_m::t_regular,
                                "b*100", 0, RELcatalogID);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error << "CatalogManager::createCatalogREL() ==> Error in creating rel database catalog index "<<err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	// 1.2 store rel db catalog OID in root index
	err = ss_m::create_assoc(SystemManager::getDevVolInfo()->volumeID, rootIndexID,
                                vec_t(RELcatalogName, strlen(RELcatalogName)),
                                vec_t(&RELcatalogID, sizeof(RELcatalogID)));
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::createCatalogREL() ==> Error in creating association in root index #2 "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

} // end createCatalogREL

void CatalogManager::registerNewCube(CubeInfo& cbinfo)
{
	//1. Assert that this cube is a new one
	if(cbinfo.get_cbID() != CubeInfo::null_id) {
		throw GeneralError(__FILE__, __LINE__, "CatalogManager::registerNewCube ==> Tried to register a new cube that its id is not null!");
	}

	if(cubeExists(cbinfo)) {
		throw GeneralError(__FILE__, __LINE__, "CatalogManager::registerNewCube ==> Can't create cube. The cube already exists.");
	}

	//2. Create new cube code
	cbinfo.set_cbID(createCubeCode());

	// Create new CubeInfo record in CubeInfo File
        //
        // note: the use of anonymous vectors since none of the data
        //       to store is scattered in memory
        // note: the ugly parameter comments are used because of a gcc bug
	serial_t record_ID;
        rc_t err = ss_m::create_rec(SystemManager::getDevVolInfo()->volumeID , cbInfoFileID,
                         vec_t(),       /* empty record header  */
                         sizeof(cbinfo),  /* length hint          */
                         vec_t(&cbinfo, sizeof(cbinfo)), /* body    */
                         record_ID);      /* new rec id           */
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::registerNewCube ==> Error in ss_m::create_rec in CatalogManager::registerNewCube "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	//3. Update cube index by name
	char* key = (char*)cbinfo.get_name().c_str();
	err = ss_m::create_assoc(SystemManager::getDevVolInfo()->volumeID, cbNmIndexID,
                                vec_t(key, strlen(key)),
                                vec_t(&record_ID, sizeof(record_ID)));
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::registerNewCube ==> Error in ss_m::create_assoc in CatalogManager::registerNewCube #1 "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	//4. Update cube index by id
	cubeID_t key2 = cbinfo.get_cbID();
	err = ss_m::create_assoc(SystemManager::getDevVolInfo()->volumeID, cbIDIndexID,
                                vec_t(&key2, sizeof(key2)),
                                vec_t(&record_ID, sizeof(record_ID)));
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::registerNewCube ==> Error in ss_m::create_assoc in CatalogManager::registerNewCube #2 "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	// 5. Update maximum cube-code in cube index by id
	// Destroy old max value
	cubeID_t oldvalue = cbinfo.get_cbID() - 1;
	key2 = MAXKEY;
	err = ss_m::destroy_assoc(SystemManager::getDevVolInfo()->volumeID, cbIDIndexID,
                                     vec_t(&key2 , sizeof(cubeID_t)),
                                     vec_t(&oldvalue, sizeof(cubeID_t)));
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error<<"CatalogManager::registerNewCube ==> Error in ss_m::destroy_assoc in CatalogManager::registerNewCube" << err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}
	// insert new max value
	key2 = MAXKEY;
	cubeID_t newvalue = cbinfo.get_cbID();
	err = ss_m::create_assoc(SystemManager::getDevVolInfo()->volumeID, cbIDIndexID,
                                vec_t(&key2, sizeof(cubeID_t)),
                                vec_t(&newvalue, sizeof(cubeID_t)));
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::registerNewCube ==> Error in ss_m::create_assoc #3 "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

} // end registerNewCube

cubeID_t CatalogManager::createCubeCode()
{
	// Find current maximum code from catalog
	smsize_t code_len = sizeof(cubeID_t);
	cubeID_t key = MAXKEY;
	cubeID_t maxCode;
    	bool found = false;
    	rc_t err = ss_m::find_assoc(SystemManager::getDevVolInfo()->volumeID, cbIDIndexID ,
                              vec_t(&key, sizeof(cubeID_t)),
                              &maxCode,
                              code_len, found);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error << err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}
	if(!found) {
		throw GeneralError(__FILE__, __LINE__, "CatalogManager::createCubeCode ==> Cannot find stored max cube-code: unable to create new cube code");
	}

	// create new code, simply increase by 1
	return (maxCode+1);

} // end createCubeCode

bool CatalogManager::cubeExists(CubeInfo& cbinfo)
{
	// 1. Look in the index by id
	cubeID_t key = cbinfo.get_cbID();
	smsize_t length_to_write = 0;  // we dont need the associated CubeInfo to return, just to check the existence

/*
	// This also works, but it returns the result into el.
	CubeInfo el;
	smsize_t length = sizeof(CubeInfo);

    	bool found = false;
    	rc_t err = ss_m::find_assoc(SystemManager::getDevVolInfo()->volumeID, btreeCatalogID,
                              vec_t(&key, sizeof(cubeID_t)),
                              &el,
                              length, // we dont need the associated CubeInfo to return, just to check the existence
			      found);
*/
	bool found = false;
    	rc_t err = ss_m::find_assoc(SystemManager::getDevVolInfo()->volumeID, cbIDIndexID,
                              vec_t(&key, sizeof(cubeID_t)),
                              NULL,
                              length_to_write, // we dont need the associated CubeInfo to return, just to check the existence
			      found);

	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::cubeExists ==> Error in ss_m::find_assoc in CatalogManager::cubeExists #1 "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	// 2. Look in the index by name
	char* key2 = (char*)cbinfo.get_name().c_str();
	length_to_write = 0;  // we dont need the associated CubeInfo to return, just to check the existence
	bool found2 = false;
    	err = ss_m::find_assoc(SystemManager::getDevVolInfo()->volumeID, cbNmIndexID,
                              vec_t(key2, strlen(key2)),
                              NULL,
                              length_to_write, // we dont need the associated CubeInfo to return, just to check the existence
			      found2);

	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::cubeExists ==> Error in ss_m::find_assoc in CatalogManager::cubeExists #2 "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}
	return (found || found2);
} // cubeExists

void CatalogManager::getCubeInfo(const string& name, CubeInfo& info)
{
	// Search in the cube index by name to get the the record id
	const char* key = name.c_str();
	serial_t rec_id;
	smsize_t length_to_write = sizeof(serial_t);
	bool found = false;
    	rc_t err = ss_m::find_assoc(SystemManager::getDevVolInfo()->volumeID, cbNmIndexID,
                              vec_t(key, strlen(key)),
                              &rec_id,
                              length_to_write, // we dont need the associated CubeInfo to return, just to check the existence
			      found);

	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::getCubeInfo ==> Error in ss_m::find_assoc in CatalogManager::getCubeInfo "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}
	if(!found) {

		throw GeneralError(__FILE__, __LINE__, "CatalogManager::getCubeInfo ==> specified cube does not exist! ");
	}

    	// pin the CubeInfo record in the buffer pool
    	pin_i handle;
    	err = handle.pin(SystemManager::getDevVolInfo()->volumeID, rec_id, 0);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::getCubeInfo ==> Error in pin_i::pin in CatalogManager::getCubeInfo "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}


    	const CubeInfo& item = *((const CubeInfo*)handle.body());
    	assert(strcmp(item.get_name().c_str(), name.c_str()) == 0);

	// update info
	info.set_name(item.get_name());
	info.set_fid(item.get_fid());
	info.set_cbID(item.get_cbID());

} // end getCubeInfo

void CatalogManager::unregisterCube(const CubeInfo& cbinfo)
{
	// first search in the cube index by name to get the the record id
	const char* key = cbinfo.get_name().c_str();

	serial_t rec_id;
	smsize_t length_to_write = sizeof(serial_t);
	bool found = false;
    	rc_t err = ss_m::find_assoc(SystemManager::getDevVolInfo()->volumeID, cbNmIndexID,
                              vec_t(key, strlen(key)),
                              &rec_id,
                              length_to_write, // we dont need the associated CubeInfo to return, just to check the existence
			      found);

	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"CatalogManager::unregisterCube ==> Error in ss_m::find_assoc in CatalogManager::unregisterCube "<< err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}
	if(!found) {

		throw GeneralError(__FILE__, __LINE__, "CatalogManager::unregisterCube ==> Exception in CatalogManager::unregisterCube : specified cube does not exist! ");
	}

	// Now, remove the association from the cube index by name.
	err = ss_m::destroy_assoc(SystemManager::getDevVolInfo()->volumeID, cbNmIndexID,
                                     vec_t(key , strlen(key)),
                                     vec_t(&rec_id, sizeof(rec_id)));
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error<<"Error in ss_m::destroy_assoc in CatalogManager::unregisterCube #1" << err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

	// Last, remove it also form the cube index by id
	const cubeID_t key2 = cbinfo.get_cbID();
	err = ss_m::destroy_assoc(SystemManager::getDevVolInfo()->volumeID, cbIDIndexID,
                                     vec_t(&key2 , sizeof(cubeID_t)),
                                     vec_t(&rec_id, sizeof(rec_id)));
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error<<"Error in ss_m::destroy_assoc in CatalogManager::unregisterCube #2" << err <<endl<<ends;
		// throw an exeption
		throw GeneralError(__FILE__, __LINE__, error.str());
	}

} // end unregisterCube



