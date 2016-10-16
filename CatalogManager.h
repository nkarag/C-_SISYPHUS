/***************************************************************************
                          CatalogManager.h  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


#ifndef CATALOG_MANAGER_H
#define CATALOG_MANAGER_H

#include <sm_vas.h>
#include "Cube.h"

/**
 * The CatalogManager class keeps database catalog information and
 * exposes it through its methods to other objects.
 * Specifically, it stores the volume root index (Shore) and
 * maintains a special B-tree index called "catalog", which can
 * be found from the root index at construction.
 *
 * @see vol_root_index(SSM), btree(SSM)
 * @author: Nikos Karayannidis
 */
class CatalogManager {

private:

	/**
	 * The key in the catalog, corresponding to the current maximum cube code
	 * this constant is defined in CatalogManager.C
	 */
	static const cubeID_t MAXKEY;
	/**
     	 * The output stream used for logging system messages.
     	 */
      	ostream& outputLogStream;

        /**
         * The output stream used for logging error messages.
         */
    	ostream& errorLogStream;

	/**
	 * The root index ID of the Shore volume. In the root index we store the
	 * following associations: 
	 * 	(MD-Catalog name, MD-Catalog OID)
	 *	(Rel-Catalog name, Rel-Catalog OID)
	 */
	static serial_t rootIndexID;

	/**
	 * The multidimensional database catalog name, i.e. key in order to look for its ID in the volume root index at construction.	
	 */
	static const char * MDcatalogName; 

	/**
	 * Shore logical id of the B-tree index serving as the catalog of the multidimensional database.
	 * In this index we store the following associations:
	 * 	(Cube_name_index name, OID)
	 * 	(Cube_id_index name, OID)
	 *	(Dimension_name_index name, OID)
	 * 	(Dimension_id_index name, OID)
	 * 	(CubeInfo File name, OID)
	 * 	(DimInfo File name, OID)
	 */
	static serial_t MDcatalogID;

	/**
	 * The relational database catalog name, i.e. key in order to look for its ID in the volume root index at construction.	
	 */
	static const char * RELcatalogName; 

	/**
	 * Shore logical id of the B-tree index serving as the catalog of the relational database
	 * In this idex we store the following associations:
	 * 	... Future Work ...
	 */
	static serial_t RELcatalogID;

	// Contents of md database catalog follow	

	/**
	 * The name of cube name index, i.e. key in order to look for its ID in the MD catalog index.
	 */
	static const char* cbNmIndexName;

	/**
	 * Shore logical id of the cube name index. In this index we store the following associations:
	 * 	(Cube name, Shore record ID of CubeInfo Instance)
	 */
	static serial_t cbNmIndexID;

	/**
	 * The name of cube ID index, i.e. key in order to look for its ID in the MD catalog index.
	 */
	static const char* cbIDIndexName;

	/**
	 * Shore logical id of the cube id index. In this index we store the following associations:
	 * 	(Cube id, Shore record ID of CubeInfo Instance)
	 */
	static serial_t cbIDIndexID;

	/**
	 * The name of dimension name index, i.e. key in order to look for its ID in the MD catalog index.
	 */
	static const char* dimNmIndexName;

	/**
	 * Shore logical id of the dimension name index. In this index we store the following associations:
	 * 	(dimension name, Shore record ID of DimInfo Instance)
	 */
	static serial_t dimNmIndexID;

	/**
	 * The name of dimension id index, i.e. key in order to look for its ID in the MD catalog index.
	 */
	static const char* dimIDIndexName;

	/**
	 * Shore logical id of the dimension ID index. In this index we store the following associations:
	 * 	(dimension id, Shore record ID of DimInfo Instance)
	 */
	static serial_t dimIDIndexID;

	/**
	 * The name of the shore file to store CubeInfo instances, i.e. key in order to look for its ID in the MD catalog index.
	 */
	static const char* cbInfoFileName;

	/**
	 * Shore logical id of the Shore File to store cubeInfo instances. 
	 */
	static serial_t cbInfoFileID;

	/**
	 * The name of the shore file to store DimInfo instances, i.e. key in order to look for its ID in the MD catalog index.
	 */
	static const char* dimInfoFileName;

	/**
	 * Shore logical id of the Shore File to store DimInfo instances. 
	 */
	static serial_t dimInfoFileID;

	// end of md database catalog contents

	/**
	 * This method is called when the id of the MD db catalog is located (at construction)
	 * in order to read the catalogs contents and initialize the corresponding members
	 */
	 void initMDcatalogMembers();

	/**
	 * This method id used in order to create from scratch the catalog
	 * for the MD database.
	 */
	 void createCatalogMD();

	/**
	 * This method id used in order to create from scratch the catalog
	 * for the REL database. 
	 */
	 void createCatalogREL();
	

	/**
	 * This method reads the maximum cube code, which is also stored cubeID_t catalog
	 * and returns a new code by increasing the max by one. Then it updates the
	 * stored maximum code in the catalog.
	 */
	static cubeID_t createCubeCode();


public:
	/**
	 * Constructor, main tasks: find catalog of md database and relational database in root-index and if
	 * it does not exist, create them. In the latter case continue  with the creation of cb name index,
	 * cb id index, dim name index, dim id index, CubeInfo file, DimInfo file.
	 */
	CatalogManager(ostream& outputLogStream = cout,
                            ostream& errorLogStream = cerr);
	~CatalogManager();

	/**
	 * This method is used whenever we wish to register a new cube in the
	 * database catalog. Each cube has a unique (per catalog) integer code. This
	 * code is used as the key with which we search for a cube in the catalog. For
	 * each such code we store a CubeInfo instance, corresponding to a specific cube.
	 * registerNewCube will create a new code for the new cube, which will be stored
	 * in the CubeInfo instance.
	 * 
	 * @param cbinfo	the CubeInfo object that will be stored in the catalog.  
	 */
	static void registerNewCube(CubeInfo& cbinfo);

	/**
	 * This method is used in order to remove a cube entry from the cube
	 * index by name and the cube index by id. Usually it is called when
	 * the AccessManager::drop_cube() command is executed.
	 *
	 * @param cbinfo	the CubeInfo of the cube 
	 */
	static void unregisterCube(const CubeInfo& cbinfo);

	/**
	 * This function checks for the existence of a cube in the catalog. 
	 * It searches in the cube indexes by id and by name for the cube-id and name and returns true if it
	 * finds it, otherwise it returns false.
	 * 
	 * @param cbinfo	the CubeInfo of the cube
	 */
	static bool cubeExists(CubeInfo& cbinfo);

	/**
	 * This method retrieves from the catalog the appropriate CubeInfo record and
	 * returns a CubeInfo instance, for the specified cube. On error throws a char* exception. 
	 */
	static void getCubeInfo(const string& name, CubeInfo& info);
	
};

#endif // CATALOG_MANAGER_H
